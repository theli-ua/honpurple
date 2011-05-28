#include "honprpl.h"
#include "hon.h"
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>


#include "account.h"
#include "accountopt.h"
#include "blist.h"
#include "cmds.h"
#include "conversation.h"
#include "connection.h"
#include "debug.h"
#include "notify.h"
#include "privacy.h"
#include "prpl.h"
#include "roomlist.h"
#include "status.h"
#include "util.h"
#include "version.h"
#include "packet_id.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

static void honpurple_nick2id_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message){
	nick2id_cb_data* cb_data = user_data;
	deserialized_element* data = NULL;
	deserialized_element* data2 = NULL;

	if (url_text)
	{
		gchar* ltext,*lname,*foundname;
		guint len,i;
		ltext = g_ascii_strdown ((gchar*)url_text,-1);
		lname = g_ascii_strdown ((gchar*)cb_data->buddy->name,-1);
		foundname = g_strrstr (ltext,lname);
		if (foundname)
		{
			i = foundname - ltext;
			len = strlen(lname);
			memcpy(cb_data->buddy->name,url_text + i , len);
		}
		g_free(ltext);
		g_free(lname);
	}
	

	if(
		!url_text
		|| (data = deserialize_php(&url_text,strlen(url_text)))->type != PHP_ARRAY
		|| ((data2 = g_hash_table_lookup(data->u.array,cb_data->buddy->name)) == 0)
		|| data2->type != PHP_STRING
		){
			if (cb_data->error_cb)
				(cb_data->error_cb)(cb_data->buddy);
	}
	else
	{
		cb_data->buddy->proto_data = GINT_TO_POINTER(atoi(data2->u.string->str));
		if (cb_data->error_cb)
			(cb_data->cb)(cb_data->buddy);
	}
	if (data)
		destroy_php_element(data);
	data = NULL;

	g_free(cb_data);
}
static void honprpl_nick2id(PurpleBuddy* buddy,nick2idCallback cb,nick2idCallback error_cb){
	nick2id_cb_data* cb_data = g_new0(nick2id_cb_data,1);
	gchar* url;
	cb_data->buddy = buddy;
	cb_data->cb = cb;
	cb_data->error_cb = error_cb;
	url = g_strdup_printf(HON_NICK2ID_REQUEST,
		purple_account_get_string(buddy->account, "masterserver", HON_DEFAULT_MASTER_SERVER),
		HON_CLIENT_REQUESTER,buddy->name
		);
	purple_util_fetch_url_request_len_with_account(buddy->account,url,TRUE,NULL,FALSE,NULL,FALSE,-1,honpurple_nick2id_cb,cb_data);
	g_free(url);
}
static void honprpl_cleanup_blist(PurpleConnection* gc){
	GSList* buddylist = purple_find_buddies	(gc->account,NULL);
	GSList* buddy = buddylist;
	
	while (buddy != NULL)
	{
		purple_blist_remove_buddy((PurpleBuddy*)buddy->data);
		buddy = buddy->next;
	}
	g_slist_free(buddylist);
}
static void honprpl_update_buddies(PurpleConnection* gc){
	hon_account* hon = gc->proto_data;
	PurpleGroup* buddies = purple_find_group(HON_BUDDIES_GROUP);
	deserialized_element* buddy_data;

	GHashTableIter iter;
	
	if (!hon->buddies)
		return;

	if (!buddies){
		buddies = purple_group_new(HON_BUDDIES_GROUP);
		purple_blist_add_group(buddies, NULL);
	}
	g_hash_table_iter_init(&iter,hon->buddies);
	while (g_hash_table_iter_next(&iter,NULL,(gpointer*)&buddy_data))
	{
		deserialized_element* buddyname = g_hash_table_lookup(buddy_data->u.array,"nickname");
		if (buddyname)
		{
			PurpleBuddy* buddy;
			guint32 id = atoi(((deserialized_element*)(g_hash_table_lookup(buddy_data->u.array,"buddy_id")))->u.string->str);
			if (!g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(id)))
			{
				g_hash_table_insert(hon->id2nick,GINT_TO_POINTER(id),g_strdup(buddyname->u.string->str));
			}
			
			buddy = purple_find_buddy(gc->account,buddyname->u.string->str);
			if (!buddy)
			{
				deserialized_element* clan_tag = g_hash_table_lookup(buddy_data->u.array,"clan_tag");
				if (!clan_tag || clan_tag->type != PHP_STRING)
				{
					buddy = purple_buddy_new(gc->account,buddyname->u.string->str,NULL);
				}
				else
				{
					clan_tag->u.string = g_string_prepend_c(clan_tag->u.string,'[');
					clan_tag->u.string = g_string_append_c(clan_tag->u.string,']');
					clan_tag->u.string = g_string_append(clan_tag->u.string,buddyname->u.string->str);
					buddy = purple_buddy_new(gc->account,buddyname->u.string->str,clan_tag->u.string->str);
				}
				purple_blist_add_buddy(buddy,NULL,buddies,NULL);
			}
			buddy->proto_data = GINT_TO_POINTER(id);
		}
		
						
	}
}
static void honprpl_update_clanmates(PurpleConnection* gc){
 	hon_account* hon = gc->proto_data;
	PurpleGroup* clanmates;
	deserialized_element* buddy_data;
	GHashTableIter iter;
	gchar* clanname;
	gchar* key;
	deserialized_element* clan_tag = g_hash_table_lookup(hon->clan_info,"tag");

	if (!hon->clanmates || !hon->clan_info)
		return;

	clanname =  ((deserialized_element*)(g_hash_table_lookup(hon->clan_info,"name")))->u.string->str;

	clanmates = purple_find_group(clanname);

	if (!clanmates){
		clanmates = purple_group_new(clanname);
		purple_blist_add_group(clanmates, NULL);
	}
	g_hash_table_iter_init(&iter,hon->clanmates);
	while (g_hash_table_iter_next(&iter,(gpointer*)&key,(gpointer*)&buddy_data))
	{
		deserialized_element* buddyname = g_hash_table_lookup(buddy_data->u.array,"nickname");
		if (buddyname)
		{
			PurpleBuddy* buddy;
			guint32 id = atoi(key);
			if (!g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(id)))
			{
				g_hash_table_insert(hon->id2nick,GINT_TO_POINTER(id),g_strdup(buddyname->u.string->str));
			}

			buddy = purple_find_buddy(gc->account,buddyname->u.string->str);
			if (!buddy)
			{
				if (!clan_tag || clan_tag->type != PHP_STRING)
				{
					buddy = purple_buddy_new(gc->account,buddyname->u.string->str,NULL);
				}
				else
				{
					GString* alias = g_string_new(clan_tag->u.string->str);
					alias = g_string_prepend_c(alias,'[');
					alias = g_string_append_c(alias,']');
					alias = g_string_append(alias,buddyname->u.string->str);
					buddy = purple_buddy_new(gc->account,buddyname->u.string->str,alias->str);
					g_string_free(alias,TRUE);
				}
				purple_blist_add_buddy(buddy,NULL,clanmates,NULL);
			}
			buddy->proto_data = GINT_TO_POINTER(id);
		}


	}

}
/* 
 * UI callbacks
 */
static void honprpl_input_user_info(PurplePluginAction *action)
{

}

/* this is set to the actions member of the PurplePluginInfo struct at the
 * bottom.
 */
static GList *honprpl_actions(PurplePlugin *plugin, gpointer context)
{
	PurplePluginAction *action = purple_plugin_action_new(
		_("Set User Info..."), honprpl_input_user_info);
	return g_list_append(NULL, action);
}


/*
 * prpl functions
 */
static const char *honprpl_list_icon(PurpleAccount *acct, PurpleBuddy *buddy)
{
	PurplePresence *presence;
	PurpleStatus *purple_status;
	guint32 status;
	guint32 flags;
	
	if (!buddy)
		return HON_PROTOCOL_ICON;
	presence = purple_buddy_get_presence(buddy);
	purple_status = purple_presence_get_active_status(presence);

	status = purple_status_get_attr_int(purple_status,HON_STATUS_ATTR);
	flags = purple_status_get_attr_int(purple_status,HON_FLAGS_ATTR);
	
	if (status == HON_STATUS_INGAME || status == HON_STATUS_INLOBBY)
	{
		purple_debug_info(HON_DEBUG_PREFIX, "%s icon: %s\n",
		buddy->name,HON_INGAME_EMBLEM);
		return HON_INGAME_EMBLEM;
	}

	return HON_PROTOCOL_ICON;
	
}

static char *honprpl_status_text(PurpleBuddy *buddy) {
	PurplePresence *presence = purple_buddy_get_presence(buddy);
	PurpleStatus *status = purple_presence_get_active_status(presence);
	//const gchar* server = purple_status_get_attr_string( status, HON_SERVER_ATTR);
	const gchar* gamename = purple_status_get_attr_string(  status, HON_GAME_ATTR);
	GString* info = g_string_new(NULL);

	if (gamename)
	{
		/*info = g_string_append(info,"game: ");*/
		info = g_string_append(info,gamename);
		/*info = g_string_append_c(info,'\n');*/
	}
	/*
	if (server)
	{
		info = g_string_append(info,"server: ");
		info = g_string_append(info,server);
		info = g_string_append_c(info,'\n');
	}
	*/



	return g_string_free(info,FALSE);	
}
static const char* honprpl_list_emblem(PurpleBuddy *b)
{
	PurplePresence *presence = purple_buddy_get_presence(b);
	PurpleStatus *purple_status = purple_presence_get_active_status(presence);

	//PurpleConnection *gc = purple_account_get_connection(purple_buddy_get_account(b));
	//guint32 status = purple_status_get_attr_int(purple_status,HON_STATUS_ATTR);
	guint32 flags = purple_status_get_attr_int(purple_status,HON_FLAGS_ATTR);
	
	/*if(status == HON_STATUS_INGAME || status == HON_STATUS_INLOBBY)
		return HON_INGAME_EMBLEM;
	else*/ if (flags & HON_FLAGS_PREPURCHASED)
	{
		return HON_PREMIUM_EMBLEM;
	}



	return NULL;
}
static void honprpl_tooltip_text(PurpleBuddy *buddy,
								 PurpleNotifyUserInfo *info,
								 gboolean full) 
{
	PurplePresence *presence = purple_buddy_get_presence(buddy);
	PurpleConnection* gc = buddy->account->gc;
	hon_account* hon = NULL;
	PurpleStatus *status = purple_presence_get_active_status(presence);
	const gchar* server = purple_status_get_attr_string(status, HON_SERVER_ATTR);
	const gchar* gamename = purple_status_get_attr_string(status, HON_GAME_ATTR);
	guint32 matchid = purple_status_get_attr_int(status, HON_MATCHID_ATTR);
	guint32 buddy_id = purple_status_get_attr_int(status, HON_BUDDYID_ATTR);
	
	if (gc)
		hon = gc->proto_data;
	if (gamename)
	{
		purple_notify_user_info_add_pair(info, _("Game"), gamename);
	}
	if (server)
	{
		purple_notify_user_info_add_pair(info, _("Server"), server);
	}
	if (matchid > 0)
	{
		gchar* matchstring = g_strdup_printf("%d",matchid);
		purple_notify_user_info_add_pair(info, _("Match ID"), matchstring);
		g_free(matchstring);
	}
	if (hon && buddy_id)
	{
		deserialized_element* data;
		gchar* buddy_string_id = g_strdup_printf("%d",buddy_id);
		deserialized_element* clanmate = g_hash_table_lookup(hon->clanmates,buddy_string_id);
		if (clanmate && ((data = g_hash_table_lookup(clanmate->u.array,"rank")) != 0))
		{
			purple_notify_user_info_add_pair(info, _("Rank"),data->u.string->str);			
		}
		if (clanmate && ((data = g_hash_table_lookup(clanmate->u.array,"join_date")) != 0))
		{
			purple_notify_user_info_add_pair(info, _("Join date"),data->u.string->str);			
		}
		if (clanmate && ((data = g_hash_table_lookup(clanmate->u.array,"message")) != 0) && data->type != PHP_NULL)
		{
			purple_notify_user_info_add_pair(info, _("Message"),data->u.string->str);			
		}
		g_free(buddy_string_id);
	}


	purple_debug_info(HON_DEBUG_PREFIX, "game status: %s\n",
		gamename);

}






static GList *honprpl_status_types(PurpleAccount *acct)
{
	GList *types = NULL;
	PurpleStatusType *type;

	purple_debug_info(HON_DEBUG_PREFIX, "returning status types for %s: %s, %s,%s\n",
		acct->username,
		HON_STATUS_ONLINE_S, HON_STATUS_INGAME_S,HON_STATUS_OFFLINE_S);

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE,
		HON_STATUS_ONLINE_S, NULL, TRUE, TRUE, FALSE,
		HON_STATUS_ATTR, _("Status"), purple_value_new(PURPLE_TYPE_INT),
		HON_FLAGS_ATTR, _("Flags"), purple_value_new(PURPLE_TYPE_INT),
		HON_BUDDYID_ATTR, _("Buddy ID"), purple_value_new(PURPLE_TYPE_INT),
		NULL);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_UNAVAILABLE,
		HON_STATUS_INGAME_S, NULL, TRUE, FALSE, FALSE,
		HON_GAME_ATTR, _("Game"), purple_value_new(PURPLE_TYPE_STRING),
		HON_SERVER_ATTR, _("Server"), purple_value_new(PURPLE_TYPE_STRING),
		HON_MATCHID_ATTR, _("Match ID"), purple_value_new(PURPLE_TYPE_INT),
		HON_STATUS_ATTR, _("Status"), purple_value_new(PURPLE_TYPE_INT),
		HON_FLAGS_ATTR, _("Flags"), purple_value_new(PURPLE_TYPE_INT),
		HON_BUDDYID_ATTR, _("Buddy ID"), purple_value_new(PURPLE_TYPE_INT),
		NULL);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_OFFLINE,
		HON_STATUS_OFFLINE_S, NULL, TRUE, TRUE, FALSE,
		"message", _("Message"), purple_value_new(PURPLE_TYPE_STRING),
		//NULL,NULL, //this produces crash :(
		NULL);
	types = g_list_prepend(types, type);

	return g_list_reverse(types);
}


static void honprpl_blist_example_menu_item(PurpleBlistNode *node, gpointer userdata) {
	purple_debug_info(HON_DEBUG_PREFIX, "example menu item clicked on user %s\n",
		((PurpleBuddy *)node)->name);
#if 0
	purple_notify_info(NULL,  /* plugin handle or PurpleConnection */
		_("Primary title"),
		_("Secondary title"),
		_("This is the callback for the honprpl menu item."));
#endif
}

static GList *honprpl_blist_node_menu(PurpleBlistNode *node) {
	return NULL;

	purple_debug_info(HON_DEBUG_PREFIX, "providing buddy list context menu item\n");

	if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
		PurpleMenuAction *action = purple_menu_action_new(
			_("honprpl example menu item"),
			PURPLE_CALLBACK(honprpl_blist_example_menu_item),
			NULL,   /* userdata passed to the callback */
			NULL);  /* child menu items */
		return g_list_append(NULL, action);
	} else {
		return NULL;
	}
}

static GList *honprpl_chat_info(PurpleConnection *gc) {
	struct proto_chat_entry *pce; /* defined in prpl.h */
	GList* m;

	purple_debug_info(HON_DEBUG_PREFIX, "returning chat setting 'room'\n");

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = _("Chat _room");
	pce->identifier = "room";
	pce->required = TRUE;
	m = g_list_append(NULL, pce);

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = _("Password");
	pce->identifier = "password";
	pce->required = FALSE;
	pce->secret = TRUE;

	return g_list_append(m, pce);
}

static GHashTable *honprpl_chat_info_defaults(PurpleConnection *gc,
											  const char *room) 
{
	GHashTable *defaults;

	purple_debug_info(HON_DEBUG_PREFIX, "returning chat default setting "
		"'room' = 'HoN'\n");

	defaults = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	g_hash_table_insert(defaults, "room", g_strdup(room));
	return defaults;
}





/*
	called from read_cb, attempts to read needed data from sock and
    pass it to the parser, passing back the return code from the read
    call for handling in read_cb 
*/
static int honprpl_read_recv(PurpleConnection *gc, int sock) {
	guint16 packet_length = 0,len = 0;
	hon_account* hon = gc->proto_data;
	gchar* buff = NULL;
	if (hon->got_length == 0)
	{
		len += read(sock,&packet_length,2);
		packet_length = ((packet_length & 0xFF) << 8) | ((packet_length & 0xFF00) >> 8);
		hon->databuff = g_byte_array_new();
		hon->got_length = packet_length;
	}
	packet_length = hon->got_length - hon->databuff->len;
	buff = g_malloc0(packet_length);

	packet_length = recv(sock,buff,packet_length,0);
	len += packet_length;

	hon->databuff = g_byte_array_append(hon->databuff,(const guint8 *)buff,packet_length);

	if (hon->databuff->len == hon->got_length && len > 0)
	{
		hon_parse_packet(gc,(gchar*)hon->databuff->data,hon->databuff->len);
		g_byte_array_free(hon->databuff,TRUE);
		hon->databuff = NULL;
		hon->got_length = 0;
	}

	//if (recv(sock, &packet_length,sizeof(&packet_length) ,MSG_PEEK) > 0)
	//	len += honprpl_read_recv(gc,sock);
	return len;
}


/** callback triggered from purple_input_add, watches the socked for
    available data to be processed by the session */
static void honprpl_read_callback(gpointer data, gint source, PurpleInputCondition cond) {
	PurpleConnection *gc = data;
	hon_account *hon = gc->proto_data;

	int ret = 0, err = 0;

	g_return_if_fail(hon != NULL);

	ret = honprpl_read_recv(gc, hon->fd);

	/* normal operation ends here */
	if(ret > 0) return;

	/* fetch the global error value */
	err = errno;

	/* read problem occurred if we're here, so we'll need to take care of
	it and clean up internal state */

	if(hon->fd) {
		close(hon->fd);
		hon->fd = 0;
	}

	if(gc->inpa) {
		purple_input_remove(gc->inpa);
		gc->inpa = 0;
	}

	if(! ret) {
#ifdef MINBIF
		purple_connection_error_reason(gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Server closed the connection\n that, probably, means that you connected from hon"));
#else
		purple_connection_error_reason(gc,
			PURPLE_CONNECTION_ERROR_NAME_IN_USE,
			_("Server closed the connection\n that, probably, means that you connected from hon"));
#endif
	} else if(ret < 0) {
		const gchar *err_str = g_strerror(err);
		char *msg = NULL;



		msg = g_strdup_printf(_("Lost connection with server: %s"), err_str);
		purple_connection_error_reason(gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			msg);
		g_free(msg);
	}
}

gboolean timeout_handler(gpointer data){
	PurpleConnection *gc = data;
	hon_account *hon = gc->proto_data;
	gchar *msg;
	if (!hon->gotPacket)
	{
		msg = g_strdup_printf(_("Lost connection with server: did not receive any packet in %d seconds"), HON_NETWORK_TIMEOUT);
		purple_connection_error_reason(gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			msg);
		g_free(msg);
		return FALSE;
	}
	hon->gotPacket = FALSE;
	return TRUE;		
}
static void honprpl_login_callback(gpointer data, gint source, const gchar *error_message)
{
    PurpleConnection *gc = data;
	PurpleAccount* acct = purple_connection_get_account(gc);
    hon_account *hon = gc->proto_data;
	int on = 1;
	if (source < 0) {
		gchar *tmp = g_strdup_printf(_("Unable to connect: %s"),
			error_message);
		purple_connection_error_reason (gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		g_free(tmp);
		return;
	}

	hon->fd = source;

	setsockopt(source,IPPROTO_TCP, TCP_NODELAY, &on, sizeof (on));

	if(hon_send_login(gc,hon->cookie,hon->ip,hon->auth,
		purple_account_get_int(acct,PROT_VER_STRING,HON_PROTOCOL_VERSION)))
	{
		purple_connection_update_progress(gc, _("Authenticating"),
			2,   /* which connection step this is */
			4);  /* total number of steps */
		gc->inpa = purple_input_add(source, PURPLE_INPUT_READ,
			honprpl_read_callback, gc);
		hon->gotPacket = FALSE;
		hon->timeout_handle = purple_timeout_add_seconds(HON_NETWORK_TIMEOUT,timeout_handler,(gpointer)gc);
	}
	else
	{
		gchar *tmp = g_strdup_printf(_("Unable to connect: %s"),
			error_message);
		purple_connection_error_reason (gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		g_free(tmp);
	}
}

static void start_hon_session_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message){
	PurpleConnection *gc = user_data;
	hon_account* hon = gc->proto_data;
	deserialized_element* account_data= hon->account_data;
	if (account_data)
	{
		destroy_php_element(account_data);
		hon->account_data = NULL;
	}

	if(!(url_text)){
		purple_connection_error_reason(gc,PURPLE_CONNECTION_ERROR_NETWORK_ERROR,error_message);
		//purple_connection_set_state(gc, PURPLE_DISCONNECTED);
	}
	else{
		purple_debug_info(HON_DEBUG_PREFIX, "data from masterserver: \n%s\n",
		url_text);

		account_data = deserialize_php(&url_text,strlen(url_text));
		if (account_data->type != PHP_ARRAY)
		{
			purple_connection_error_reason(gc,PURPLE_CONNECTION_ERROR_OTHER_ERROR,_("Bad data received from server"));
			//purple_connection_set_state(gc, PURPLE_DISCONNECTED);
		}
		else{
			deserialized_element* res = g_hash_table_lookup(account_data->u.array,"0");
			if (!res || !res->u.int_val)
			{
				res = g_hash_table_lookup(account_data->u.array,"auth");
				if (res && res->type == PHP_STRING)
				{
					purple_connection_error_reason(gc,PURPLE_CONNECTION_ERROR_OTHER_ERROR,res->u.string->str);
				}
				else
					purple_connection_error_reason(gc,PURPLE_CONNECTION_ERROR_OTHER_ERROR,_("Unknown error"));
				//purple_connection_set_state(gc, PURPLE_DISCONNECTED);
			}
			else{
				deserialized_element* tmp;
				gchar* account_id = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"account_id")))->u.string->str;
				/* TODO: check for errors */
				hon->cookie = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"cookie")))->u.string->str;
				hon->ip = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"ip")))->u.string->str;
				hon->auth = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"auth_hash")))->u.string->str;
				hon->self.nickname = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"nickname")))->u.string->str;
				hon->self.account_id = atoi(((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"account_id")))->u.string->str);
				
				tmp = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"buddy_list")));
				if (tmp){
					tmp = g_hash_table_lookup(tmp->u.array,account_id);
					hon->buddies = tmp ? tmp->u.array : NULL;
				}

				tmp = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"banned_list")));
				if (tmp){
					tmp = g_hash_table_lookup(tmp->u.array,account_id);
					hon->banned = tmp ? tmp->u.array : NULL;
				}

				tmp = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"ignored_list")));
				if (tmp){
					tmp = g_hash_table_lookup(tmp->u.array,account_id);
					hon->ignores = tmp ? tmp->u.array : NULL;
				}

				hon->clanmates = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"clan_roster")))->u.array;
				if (g_hash_table_lookup(hon->clanmates,"error"))
				{
					hon->clanmates = NULL;
				}

				hon->clan_info = ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"clan_member_info")))->u.array;
				if (g_hash_table_lookup(hon->clan_info,"error"))
					hon->clan_info = NULL;
				if (hon->clan_info)
				{
					hon->self.clan_tag = ((deserialized_element*)g_hash_table_lookup(hon->clan_info,"tag"))->u.string->str;
					hon->self.clan_name = ((deserialized_element*)g_hash_table_lookup(hon->clan_info,"name"))->u.string->str;
				}
				
				
				
				honprpl_cleanup_blist(gc);
				honprpl_update_buddies(gc);
				honprpl_update_clanmates(gc);


				purple_connection_update_progress(gc, _("Connecting"),
					1,   /* which connection step this is */
					4);  /* total number of steps */


 				if (purple_proxy_connect(gc, gc->account, 
 				                         ((deserialized_element*)(g_hash_table_lookup(account_data->u.array,"chat_url")))->u.string->str,
 				                         HON_CHAT_PORT,
 					honprpl_login_callback, gc) == NULL)
				{
					purple_connection_error_reason (gc,
						PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
						_("Unable to connect"));
					return;
				}
			}


		}		


	}
	hon->account_data = account_data;
}

static void honprpl_login(PurpleAccount *acct)
{
	PurpleUtilFetchUrlData *url_data;
	PurpleConnection *gc = purple_account_get_connection(acct);
	hon_account* honacc;
	GString* request_url = g_string_new(NULL);
	GString* password_md5;

	if ( !purple_account_get_bool(acct,IS_MD5_OPTION,TRUE)){
		password_md5 = get_md5_string(acct->password);
	}
	else
		password_md5 = g_string_new(acct->password);


	g_string_printf(request_url,"%s%s?f=auth&login=%s&password=%s",
		purple_account_get_string(acct, "masterserver", HON_DEFAULT_MASTER_SERVER),
		HON_CLIENT_REQUESTER,
		acct->username,password_md5->str);


	purple_debug_info(HON_DEBUG_PREFIX, "logging in %s\n", acct->username);

	purple_connection_update_progress(gc, _("Getting COOKIE"),
		0,   /* which connection step this is */
		2);  /* total number of steps */

	gc->proto_data = honacc = g_new0(hon_account, 1);

	url_data = purple_util_fetch_url_request_len_with_account(gc->account,request_url->str,TRUE,NULL,FALSE,NULL,FALSE,-1,start_hon_session_cb,gc);

	g_string_free(password_md5,TRUE);
	g_string_free(request_url,TRUE);  

	honacc->account_data = NULL;
	honacc->got_length = 0;
	honacc->gotPacket = FALSE;
	honacc->timeout_handle = 0;
	honacc->id2nick = g_hash_table_new_full(g_direct_hash,g_direct_equal,NULL,g_free);
}

static void honprpl_close(PurpleConnection *gc)
{
	hon_account* hon = gc->proto_data;
	close(hon->fd);

	if (gc->inpa)
		purple_input_remove(gc->inpa);
	g_hash_table_destroy(hon->id2nick);
	if (hon->account_data)
		destroy_php_element(hon->account_data);
	hon->account_data = NULL;
	if (hon->timeout_handle)
		purple_timeout_remove(hon->timeout_handle);
	g_free(hon);
	gc->proto_data = NULL;
}

static int honprpl_send_im(PurpleConnection *gc, const char *who,
						   const char *message, PurpleMessageFlags flags)
{
	char *plain;
	int res;
	purple_markup_html_to_xhtml(message, NULL, &plain);

#ifdef _DEBUG
	purple_debug_info(HON_DEBUG_PREFIX, "sending message to %s: %s\n",
		who, message);
#endif
	res = hon_send_pm(gc,who,plain);
	g_free(plain);
	return res;
}
#define fetch_info_row(x,y) 	\
			if ((info_row = g_hash_table_lookup(needed_data->u.array,x)) != NULL\
				&& info_row->type == PHP_STRING\
				)\
				purple_notify_user_info_add_pair(info, _(y), info_row->u.string->str);


static void honpurple_info_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message){
	PurpleBuddy* buddy = user_data;
	PurpleConnection *gc = buddy->account->gc;
	deserialized_element* data = NULL;
	deserialized_element* needed_data = NULL;
	deserialized_element* info_row = NULL;
	PurpleNotifyUserInfo* info = purple_notify_user_info_new();
	gchar* account_id = g_strdup_printf("%d",GPOINTER_TO_INT(buddy->proto_data));
	if(
		!url_text
		|| (data = deserialize_php(&url_text,strlen(url_text)))->type != PHP_ARRAY
		)
	{
		purple_notify_user_info_add_pair(info, _("Error"), _("error retrieving account stats"));
	}
	else
	{
		if (
			(needed_data = g_hash_table_lookup(data->u.array,"clan_info"))
			&& needed_data->type == PHP_ARRAY
			&& (needed_data = g_hash_table_lookup(needed_data->u.array,account_id))
			&& needed_data->type == PHP_ARRAY
			)
		{
			fetch_info_row("name","Clan Name")
			fetch_info_row("tag","Clan Tag")
			fetch_info_row("rank","Clan Rank")
		}

		if (
			(needed_data = g_hash_table_lookup(data->u.array,"all_stats"))
			&& needed_data->type == PHP_ARRAY
			&& (needed_data = g_hash_table_lookup(needed_data->u.array,account_id))
			&& needed_data->type == PHP_ARRAY
			)
		{
			fetch_info_row("nickname","Nickname")
			fetch_info_row("level","Level")
			fetch_info_row("acc_exp","Current XP")
			fetch_info_row("acc_wins","Wins")
			fetch_info_row("acc_losses","Losses")
			fetch_info_row("acc_games_played","Games Played")
			fetch_info_row("acc_concedes","Concedes")
			fetch_info_row("acc_concedevotes","Concede Votes")
			fetch_info_row("acc_kicked","Kicks")
			fetch_info_row("acc_discos","Disconnects")
			fetch_info_row("acc_pub_skill","Public Skill Rating (PSR)")
			fetch_info_row("acc_herokills","Kills")
			fetch_info_row("acc_deaths","Deaths")
			fetch_info_row("acc_buybacks","Hero Buy Backs")
			fetch_info_row("acc_herodmg","Damage to Heroes")
			fetch_info_row("acc_heroexp","XP From Hero Kills")
			fetch_info_row("acc_heroassists","Hero Kill Assists")
			fetch_info_row("acc_denies","Total Kills Denied")
			fetch_info_row("acc_exp_denied","Total XP Denied")
			fetch_info_row("acc_teamcreepkills","Enemy Creep Kills")
			fetch_info_row("acc_teamcreepdmg","Enemy Creep Damage")
			fetch_info_row("acc_teamcreepexp","Enemy Creep XP")
			fetch_info_row("acc_neutralcreepkills","Neutral Creep Kills")
			fetch_info_row("acc_neutralcreepdmg","Neutral Creep Damage")
			fetch_info_row("acc_neutralcreepexp","Neutral Creep XP")
			fetch_info_row("acc_razed","Buildings Razed")
			fetch_info_row("acc_bdmg","Total Building Damage")
			fetch_info_row("acc_bdmgexp","XP from buidings")
			fetch_info_row("acc_gold","Total Gold Farmed")
			fetch_info_row("acc_herokillsgold","Gold From Hero Kills")
			fetch_info_row("acc_teamcreepgold","Gold From Enemy Creeps")
			fetch_info_row("acc_neutralcreepgold","Gold From Neutral Creeps")
			fetch_info_row("acc_bgold","Gold From Buildings")
			fetch_info_row("acc_goldlost2death","Gold Lost To Death")
			fetch_info_row("acc_gold_spent","Total Gold Spent")
			fetch_info_row("acc_secs","Total Seconds Played")
			fetch_info_row("acc_secs_dead","Total Seconds Dead")
			fetch_info_row("acc_actions","Total Actions performed")
			fetch_info_row("acc_consumables","Total Consumables Used")
			fetch_info_row("acc_wards","Total Wards Used")
			fetch_info_row("create_date","Account Created")
			fetch_info_row("last_login","Last Login")
			fetch_info_row("last_activity","Last Activity")
	
			/* unused:
			minXP: 1741932
			nickname: dancercod
			APEM: 0
			maxXP: 2177414
			account_id: 78959
			AP: 0
			acc_avg_score: 0.00
			AR: 0
			AREM: 0
			*/
		}

	}
	purple_notify_userinfo(gc,        /* connection the buddy info came through */
		buddy->name,  /* buddy's username */
		info,      /* body */
		(PurpleNotifyCloseCallback)destroy_php_element,      /* callback called when dialog closed */
		data);     /* userdata for callback */

	purple_notify_user_info_destroy(info);
	purple_buddy_destroy(buddy);
}
#undef fetch_info_row
static void honprpl_info_nick2id_callback(PurpleBuddy* buddy){
	gchar* url = g_strdup_printf(HON_STATS_REQUEST,
		purple_account_get_string(buddy->account, "masterserver", HON_DEFAULT_MASTER_SERVER),
		HON_CLIENT_REQUESTER,GPOINTER_TO_INT(buddy->proto_data)
		);
	purple_util_fetch_url_request_len_with_account(buddy->account,url,TRUE,NULL,FALSE,NULL,FALSE,-1,honpurple_info_cb,buddy);
	g_free(url);
}
static void honprpl_info_nick2id_error_callback(PurpleBuddy* buddy){
	PurpleNotifyUserInfo* info = purple_notify_user_info_new();
	purple_notify_user_info_add_pair(info, _("Error"), _("Error retrieving account id"));
	purple_notify_userinfo(buddy->account->gc,        /* connection the buddy info came through */
		buddy->name,  /* buddy's username */
		info,      /* body */
		NULL,      /* callback called when dialog closed */
		NULL);     /* userdata for callback */
	purple_buddy_destroy(buddy);
	purple_notify_user_info_destroy(info);
}

static void honprpl_get_info(PurpleConnection *gc, const char *username) {
	PurpleBuddy* OrigBuddy = purple_find_buddy(gc->account,username);
	PurpleBuddy* buddy = purple_buddy_new(gc->account,username,NULL);

	if(OrigBuddy)
	{
		buddy->proto_data = OrigBuddy->proto_data;
		honprpl_info_nick2id_callback(buddy);
	}
	else
	{
		honprpl_nick2id(buddy,honprpl_info_nick2id_callback,honprpl_info_nick2id_error_callback);
	}
}
static void honpurple_add_buddy_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message){
	PurpleBuddy *buddy = user_data;
	deserialized_element* data = NULL;
	deserialized_element* data2 = NULL;

	if(
		!url_text
		|| (data = deserialize_php(&url_text,strlen(url_text)))->type != PHP_ARRAY
		|| !(data2 = g_hash_table_lookup(data->u.array,"0"))
		|| data2->u.int_val == 0
		)
	{
		purple_notify_error(NULL,_("Add buddy error"),_("Got bad data from masterserver"),NULL);
		purple_blist_remove_buddy(buddy);
	}
	if (data)
		destroy_php_element(data);

}
static void honpurple_add_buddy_nick2id_error_cb(PurpleBuddy* buddy){
	purple_notify_error(NULL,_("Add buddy error"),_("Could not get account id"),NULL);
	purple_blist_remove_buddy(buddy);
}
static void honpurple_add_buddy_nick2id_cb(PurpleBuddy* buddy){
	PurpleConnection *gc = buddy->account->gc;
	hon_account* hon = gc->proto_data;
	gchar* url;
	url = g_strdup_printf(HON_ADD_BUDDY_REQUEST,purple_account_get_string(gc->account, "masterserver", HON_DEFAULT_MASTER_SERVER),
		HON_CLIENT_REQUESTER,hon->self.account_id,GPOINTER_TO_INT(buddy->proto_data),hon->cookie);
	purple_util_fetch_url_request_len_with_account(gc->account,url,TRUE,NULL,FALSE,NULL,FALSE,-1,honpurple_add_buddy_cb,buddy);
	g_free(url);
}
static void honprpl_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy,
							  PurpleGroup *group)
{
	honprpl_nick2id(buddy,honpurple_add_buddy_nick2id_cb,honpurple_add_buddy_nick2id_error_cb);
}
static void honprpl_add_buddies(PurpleConnection *gc, GList *buddies,
								GList *groups)
{
	GList *buddy = buddies;
	GList *group = groups;

	purple_debug_info(HON_DEBUG_PREFIX, "adding multiple buddies\n");

	while (buddy && group) {
		honprpl_add_buddy(gc, (PurpleBuddy *)buddy->data, (PurpleGroup *)group->data);
		buddy = g_list_next(buddy);
		group = g_list_next(group);
	}
}
static void honpurple_remove_buddy_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message){
	PurpleBuddy *buddy = user_data;
	deserialized_element* data = NULL;
	deserialized_element* data2 = NULL;

	if(
		!url_text
		|| (data = deserialize_php(&url_text,strlen(url_text)))->type != PHP_ARRAY
		|| !(data2 = g_hash_table_lookup(data->u.array,"0"))
		|| data2->u.int_val == 0
		)
	{
		purple_notify_error(NULL,_("Remove buddy error"),_("Got bad data from masterserver"),NULL);
	}
	if (data)
		destroy_php_element(data);
	data = NULL;
	purple_buddy_destroy(buddy);

}
static void honprpl_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy,
								 PurpleGroup *group)
{
	guint32 buddy_id;
	PurpleBuddy *tmpbuddy;
	hon_account* hon = gc->proto_data;
	gchar* url;
	purple_debug_info(HON_DEBUG_PREFIX, "removing %s from %s's buddy list\n",
		buddy->name, gc->account->username);
	buddy_id = GPOINTER_TO_INT(buddy->proto_data);
	if (buddy_id == 0)
	{
		/*purple_blist_remove_buddy(buddy);*/
		return;
	}
	
	tmpbuddy = purple_buddy_new(gc->account,buddy->name,NULL);
	tmpbuddy->proto_data = GINT_TO_POINTER(buddy_id);
	url = g_strdup_printf(HON_REMOVE_BUDDY_REQUEST,purple_account_get_string(gc->account, "masterserver", HON_DEFAULT_MASTER_SERVER),
		HON_CLIENT_REQUESTER,hon->self.account_id,buddy_id,hon->cookie);
	purple_util_fetch_url_request_len_with_account(gc->account,url,TRUE,NULL,FALSE,NULL,FALSE,-1,honpurple_remove_buddy_cb,tmpbuddy);
	g_free(url);
}

static void honprpl_remove_buddies(PurpleConnection *gc, GList *buddies,
								   GList *groups) 
{
	GList *buddy = buddies;
	GList *group = groups;

	purple_debug_info(HON_DEBUG_PREFIX, "removing multiple buddies\n");

	while (buddy && group) {
		honprpl_remove_buddy(gc, (PurpleBuddy *)buddy->data,
			(PurpleGroup *)group->data);
		buddy = g_list_next(buddy);
		group = g_list_next(group);
	}
}

static void honprpl_join_chat(PurpleConnection *gc, GHashTable *components) {
	gchar* password = g_hash_table_lookup(components, "password");
	if (password && strlen(password) > 0)
		hon_send_join_chat_password(gc,g_hash_table_lookup(components, "room"),password);
	else
		hon_send_join_chat(gc,g_hash_table_lookup(components, "room"));
}

static char *honprpl_get_chat_name(GHashTable *components) {
	const char *room = g_hash_table_lookup(components, "room");
	purple_debug_info(HON_DEBUG_PREFIX, "reporting chat room name '%s'\n", room);
	return g_strdup(room);
}

static void honprpl_chat_leave(PurpleConnection *gc, int id) {
	PurpleConversation* conv = purple_find_chat(gc,id);
	hon_send_leave_chat(gc,conv->name);
}


static int honprpl_chat_send(PurpleConnection *gc, int id, const char *message,
							 PurpleMessageFlags flags) 
{
	hon_account* hon = gc->proto_data;
	gchar* coloredmessage;
	char *plain;
	int res;
	purple_markup_html_to_xhtml(message, NULL, &plain);
	coloredmessage = hon2html(plain);
	res = hon_send_chat_message(gc,id,plain);
	serv_got_chat_in(gc,id,hon->self.nickname,PURPLE_MESSAGE_SEND,coloredmessage,time(NULL));
	g_free(coloredmessage);
	g_free(plain);
	return 0;
}



static void honprpl_set_chat_topic(PurpleConnection *gc, int id,
								   const char *topic)
{
	hon_send_chat_topic(gc,id,topic);
}
static PurpleCmdRet honprpl_send_whisper(PurpleConversation *conv, const gchar *cmd,
								 gchar **args, gchar **error, void *userdata) 
{
	const char *to_username;
	const char *message;
	gchar* colored_msg;
	hon_account* hon = conv->account->gc->proto_data;
	guint32 whisper_type = GPOINTER_TO_INT(userdata);

	/* parse args */
	if (whisper_type == 0)
	{
		to_username = args[0];
		message = args[1];
	}
	else
	{
		to_username = conv->name;
		message = args[0];
	}
	

	if (!to_username || strlen(to_username) == 0) {
		*error = g_strdup(_("Whisper is missing recipient."));
		return PURPLE_CMD_RET_FAILED;
	} else if (!message || strlen(message) == 0) {
		*error = g_strdup(_("Whisper is missing message."));
		return PURPLE_CMD_RET_FAILED;
	}
	purple_debug_info("honprpl", "%s whispers to %s in chat room %s: %s\n",
		hon->self.nickname, to_username, conv->name, message);
	
	hon_send_whisper(conv->account->gc,to_username,message);

	colored_msg = hon2html(message);
	purple_conversation_write(conv,hon->self.nickname, colored_msg, PURPLE_MESSAGE_WHISPER|PURPLE_MESSAGE_SEND, time(NULL));
	g_free(colored_msg);

	return PURPLE_CMD_RET_OK;
	
}
static PurpleCmdRet honprpl_manage_buddies(PurpleConversation *conv, const gchar *cmd,
										   gchar **args, gchar **error, void *userdata) 
{
	const char *command = args[0];
	PurpleConnection* gc = conv->account->gc;
	PurpleBuddy* buddy = NULL;
	if (!g_strcmp0(command,"add"))
	{
		buddy = purple_buddy_new(gc->account,args[1],NULL);
		honprpl_add_buddy(gc,buddy,NULL);
	}
	else if (!g_strcmp0(command,"del"))
	{
		buddy = purple_find_buddy(gc->account,args[1]);
		if (!buddy)
		{
			*error = g_strdup(_("Couldn't find buddy"));
			return PURPLE_CMD_RET_FAILED;
		}
		honprpl_remove_buddy(gc,buddy,NULL);
	}
	else {
		*error = g_strdup(_("Unknown buddy command"));
		return PURPLE_CMD_RET_FAILED;
	}
	return PURPLE_CMD_RET_OK;	
}

static PurpleCmdRet honprpl_channel_auth(PurpleConversation *conv, const gchar *cmd,
										 gchar **args, gchar **error, void *userdata) 
{
	const char *command = args[0];
	PurpleConnection* gc = conv->account->gc;
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	if (!g_strcmp0(command,"enable"))
	{
		hon_send_channel_auth_enable(gc,chat->id);
	}
	else if (!g_strcmp0(command,"disable")) 
	{
		hon_send_channel_auth_disable(gc,chat->id);
	}
	else if (!g_strcmp0(command,"add") || !g_strcmp0(command,"a")) 
	{
		if (!args[1] || strlen(args[1]) < 1)
		{
			*error = g_strdup(_("Command missing username"));
			return PURPLE_CMD_RET_FAILED;
		}
		hon_send_channel_auth_add(gc,chat->id,args[1]);
	}
	else if (!g_strcmp0(command,"delete") || !g_strcmp0(command,"d")) 
	{
		if (!args[1] || strlen(args[1]) < 1)
		{
			*error = g_strdup(_("Command missing username"));
			return PURPLE_CMD_RET_FAILED;
		}
		hon_send_channel_auth_delete(gc,chat->id,args[1]);
	}
	else if (!g_strcmp0(command,"list") || !g_strcmp0(command,"l")) 
	{
		hon_send_channel_auth_list(gc,chat->id);
	}
	else {
		*error = g_strdup(_("Unknown auth command"));
		return PURPLE_CMD_RET_FAILED;
	}
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_clan_commands(PurpleConversation *conv, const gchar *cmd,
								 gchar **args, gchar **error, void *userdata) 
{
	
	const char *command = args[0];
	PurpleConnection* gc = conv->account->gc;
	hon_account* hon = conv->account->gc->proto_data;
	GString* msg;

	if (!hon->clan_info)
	{
		*error = g_strdup(_("Not in clan"));
		return PURPLE_CMD_RET_FAILED;
	}

	if (!g_strcmp0(command,"invite"))
	{
		deserialized_element* rank = g_hash_table_lookup(hon->clan_info,"rank");
		if (rank && !g_ascii_strncasecmp(rank->u.string->str,"member",6))
		{
			*error = g_strdup(_("Only clan founder or officer can invite"));
			return PURPLE_CMD_RET_FAILED;
		}
		hon_send_clan_invite(gc,args[1]);
		msg = g_string_new(NULL);
		g_string_printf(msg,_("Invited %s to clan"),args[1]);
		purple_conversation_write(conv, "",msg->str , PURPLE_MESSAGE_SYSTEM|PURPLE_MESSAGE_NO_LOG, time(NULL));
		g_string_free(msg,TRUE);
		return PURPLE_CMD_RET_OK;
	}
	else if (!g_strcmp0(command,"m") || !g_strcmp0(command,"message"))
	{
		PurpleConversation* clanConv;
		gchar* message = hon2html(args[1]);
		GString* clan_chat_name = g_string_new("Clan ");
		clan_chat_name = g_string_append(clan_chat_name,hon->self.clan_name);
		clanConv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT,clan_chat_name->str,conv->account);
		g_string_free(clan_chat_name,TRUE);
		if (clanConv)
		{
			purple_conv_chat_write(PURPLE_CONV_CHAT(clanConv), hon->self.nickname,message, PURPLE_MESSAGE_WHISPER, time(NULL));
		}
		g_free(message);
		hon_send_clan_message(gc,args[1]);
		return PURPLE_CMD_RET_OK;
	}

	*error = g_strdup(_("Unknown clan command"));
	return PURPLE_CMD_RET_FAILED;
}

static PurpleCmdRet honprpl_who(PurpleConversation *conv, const gchar *cmd,
								  gchar **args, gchar **error, void *userdata) 
{
	const char *user = args[0];
	hon_account* hon = conv->account->gc->proto_data;

	if (!user || strlen(user) == 0) {
		*error = g_strdup(_("Request is missing nickname."));
		return PURPLE_CMD_RET_FAILED;
	} 
	hon->whois_conv = conv;
	hon_send_whois(conv->account->gc,user);
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_join(PurpleConversation *conv, const gchar *cmd,
								gchar **args, gchar **error, void *userdata)
{
	const char *room;
	const char *password;
	GHashTable* table;

	guint32 join_type = GPOINTER_TO_INT(userdata);

	if (join_type == 0)
	{
		room = args[0];
		password = "";
	}
	else
	{
		room = args[0];
		password = args[1];
	}
	table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
	g_hash_table_insert(table, "room", g_strdup(room));
	g_hash_table_insert(table, "password", g_strdup(password));

	honprpl_join_chat(conv->account->gc, table);

	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_leave(PurpleConversation *conv, const gchar *cmd,
								gchar **args, gchar **error, void *userdata)
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	honprpl_chat_leave(conv->account->gc, chat->id);
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_topic(PurpleConversation *conv, const gchar *cmd,
								gchar **args, gchar **error, void *userdata) 
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	honprpl_set_chat_topic(conv->account->gc,chat->id,args[0]);
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_channel_emote(PurpleConversation *conv, const gchar *cmd,
								  gchar **args, gchar **error, void *userdata) 
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	hon_account* hon = conv->account->gc->proto_data;
	hon_send_emote(conv->account->gc,chat->id,args[0]);
	purple_conv_chat_write(chat,hon->self.nickname,args[0],PURPLE_MESSAGE_SEND,time(NULL));
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_whisper_buddies(PurpleConversation *conv, const gchar *cmd,
										  gchar **args, gchar **error, void *userdata) 
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	hon_account* hon = conv->account->gc->proto_data;
	hon_send_whisper_buddies(conv->account->gc,args[0]);
	purple_conv_chat_write(chat,hon->self.nickname,args[0],PURPLE_MESSAGE_SEND|PURPLE_MESSAGE_WHISPER,time(NULL));
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_join_game(PurpleConversation *conv, const gchar *cmd,
										  gchar **args, gchar **error, void *userdata) 
{
	hon_send_join_game(conv->account->gc,args[2],atoi(args[1]),args[0]);
	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet honprpl_password(PurpleConversation *conv, const gchar *cmd,
								  gchar **args, gchar **error, void *userdata) 
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	gchar* password = "";
	if (args[0])
		password = args[0];
	hon_send_channel_password(conv->account->gc,chat->id,password);
	return PURPLE_CMD_RET_OK;
}

static PurpleCmdRet honprpl_silence(PurpleConversation *conv, const gchar *cmd,
								 gchar **args, gchar **error, void *userdata) 
{
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	guint32 duration = atoi(args[1]) * 1000;
	hon_send_channel_silence(conv->account->gc,chat->id,args[0],duration);
	return PURPLE_CMD_RET_OK;
}
static PurpleCmdRet honprpl_kick(PurpleConversation *conv, const gchar *cmd,
								  gchar **args, gchar **error, void *userdata) 
{
	const char* user = args[0];
	hon_account* hon = conv->account->gc->proto_data;
	guint32 kicked_id = 0,id = 0;
	guint32 packetId = GPOINTER_TO_INT(userdata);
	gchar* name;
	GHashTableIter iter;
	PurpleConvChat* chat = PURPLE_CONV_CHAT(conv);
	if (!user || strlen(user) == 0) {
		*error = g_strdup(_("Command is missing nickname."));
		return PURPLE_CMD_RET_FAILED;
	} 
	if (!purple_conv_chat_cb_find(chat,user)
		&& packetId != HON_CS_CHANNEL_BAN && packetId != HON_CS_CHANNEL_UNBAN
		) {
		*error = g_strdup(_("There is no such user in a channel"));
		return PURPLE_CMD_RET_FAILED;
	}
	if (!purple_conv_chat_cb_find(chat,hon->self.nickname) || !(purple_conv_chat_cb_find(chat,hon->self.nickname)->flags & (PURPLE_CBFLAGS_FOUNDER | PURPLE_CBFLAGS_OP | PURPLE_CBFLAGS_HALFOP))) {
		*error = g_strdup(_("You need to be OP or Founder for this command"));
		return PURPLE_CMD_RET_FAILED;
	}
	g_hash_table_iter_init(&iter,hon->id2nick);
	while (kicked_id == 0 && g_hash_table_iter_next(&iter,(gpointer *)&id,(gpointer *)&name))
	{
		if (strcmp(name,user)==0)
		{
			kicked_id = id;
		}
	}
	if (kicked_id != 0 || packetId == HON_CS_CHANNEL_BAN || packetId == HON_CS_CHANNEL_UNBAN)
		switch (packetId){
			case HON_CS_CHANNEL_KICK:
				hon_send_channel_kick(conv->account->gc,chat->id,kicked_id);
				break;
			case HON_CS_CHANNEL_PROMOTE:
				hon_send_channel_promote(conv->account->gc,chat->id,kicked_id);
				break;
			case HON_CS_CHANNEL_DEMOTE:
				hon_send_channel_demote(conv->account->gc,chat->id,kicked_id);
				break;
			case HON_CS_CHANNEL_BAN:
				hon_send_channel_ban(conv->account->gc,chat->id,user);
				break;
			case HON_CS_CHANNEL_UNBAN:
				hon_send_channel_unban(conv->account->gc,chat->id,user);
				break;
			default:
				//*error = g_strdup(_("Was unable to find users account id"));
				return PURPLE_CMD_RET_FAILED;

		}
	else{
		*error = g_strdup(_("Was unable to find users account id"));
		return PURPLE_CMD_RET_FAILED;
	}
	return PURPLE_CMD_RET_OK;
}


/*
* prpl stuff. see prpl.h for more information.
*/

static PurplePluginProtocolInfo prpl_info =
{
	OPT_PROTO_CHAT_TOPIC | OPT_PROTO_UNIQUE_CHATNAME,  /* options */
	NULL,               /* user_splits, initialized in honprpl_init() */
	NULL,               /* protocol_options, initialized in honprpl_init() */
	{   /* icon_spec, a PurpleBuddyIconSpec */
		"",                   /* format */
			0,                               /* min_width */
			0,                               /* min_height */
			0,                             /* max_width */
			0,                             /* max_height */
			0,                           /* max_filesize */
			0,       /* scale_rules */
	},
	honprpl_list_icon,                  /* list_icon */
	honprpl_list_emblem,                                /* list_emblem */
	honprpl_status_text,                /* status_text */
	honprpl_tooltip_text,               /* tooltip_text */
	honprpl_status_types,               /* status_types */
	honprpl_blist_node_menu,            /* blist_node_menu */
	honprpl_chat_info,                  /* chat_info */
	honprpl_chat_info_defaults,         /* chat_info_defaults */
	honprpl_login,                      /* login */
	honprpl_close,                      /* close */
	honprpl_send_im,                    /* send_im */
	NULL,                   /* set_info */
	NULL,                /* send_typing */
	honprpl_get_info,                   /* get_info */
	NULL,                 /* set_status */
	NULL,                   /* set_idle */
	NULL,              /* change_passwd */
	honprpl_add_buddy,                  /* add_buddy */
	honprpl_add_buddies,                /* add_buddies */
	honprpl_remove_buddy,               /* remove_buddy */
	honprpl_remove_buddies,             /* remove_buddies */
	NULL,                 /* add_permit */
	NULL,                   /* add_deny */
	NULL,                 /* rem_permit */
	NULL,                   /* rem_deny */
	NULL,            /* set_permit_deny */
	honprpl_join_chat,                  /* join_chat */
	NULL,                /* reject_chat */
	honprpl_get_chat_name,              /* get_chat_name */
	NULL,                /* chat_invite */
	honprpl_chat_leave,                 /* chat_leave */
	NULL,               /* chat_whisper */
	honprpl_chat_send,                  /* chat_send */
	NULL,                                /* keepalive */
	NULL,              /* register_user */
	NULL,                /* get_cb_info */
	NULL,                                /* get_cb_away */
	NULL,                /* alias_buddy */
	NULL,                /* group_buddy */
	NULL,               /* rename_group */
	NULL,                                /* buddy_free */
	NULL,               /* convo_closed */
	hon_normalize_nick,                  /* normalize */
	NULL,             /* set_buddy_icon */
	NULL,               /* remove_group */
	NULL,                                /* get_cb_real_name */
	honprpl_set_chat_topic,             /* set_chat_topic */
	NULL,                                /* find_blist_chat */
	NULL,          /* roomlist_get_list */
	NULL,            /* roomlist_cancel */
	NULL,   /* roomlist_expand_category */
	NULL,           /* can_receive_file */
	NULL,                                /* send_file */
	NULL,                                /* new_xfer */
	NULL,            /* offline_message */
	NULL,                                /* whiteboard_prpl_ops */
	NULL,                                /* send_raw */
	NULL,                                /* roomlist_room_serialize */
	NULL,                                /* unregister_user */
	NULL,                                /* send_attention */
	NULL,                                /* get_attention_types */
	sizeof(PurplePluginProtocolInfo),    /* struct_size */
	NULL,
	NULL,                                 /* initiate_media */
	NULL                                  /* can_do_media */	
};
static void honprpl_init(PurplePlugin *plugin)
{
	PurpleAccountOption *option_store_md5,*protocol_version;
	PurpleAccountOption *option = purple_account_option_string_new(
		_("HoN masterserver URL"),      /* text shown to user */
		"masterserver",                /* pref name */
		HON_DEFAULT_MASTER_SERVER);               /* default value */

	purple_debug_info(HON_DEBUG_PREFIX, "starting up\n");

	option_store_md5 = purple_account_option_bool_new(
		_("Password is a MD5 hash"),      /* text shown to user */
		IS_MD5_OPTION,                /* pref name */
		FALSE
		);
    protocol_version = purple_account_option_int_new(_("Protocol version"),
            PROT_VER_STRING,HON_PROTOCOL_VERSION);
	prpl_info.protocol_options = g_list_append(NULL, option);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option_store_md5);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,protocol_version);

	/* register whisper chat command */
	purple_cmd_register("whisper",
		"ws",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT  ,
		"prpl-hon",
		honprpl_send_whisper,
		"whisper &lt;username&gt; &lt;message&gt;: send a whisper message",
		GINT_TO_POINTER(0));                 /* userdata */
	purple_cmd_register("whisper",
		"s",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_send_whisper,
		"whisper &lt;message&gt;: send a whisper message",
		GINT_TO_POINTER(1));                 /* userdata */
	purple_cmd_register("w",
		"ws",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT  ,
		"prpl-hon",
		honprpl_send_whisper,
		"whisper &lt;username&gt; &lt;message&gt;: send a whisper message",
		GINT_TO_POINTER(0));                 /* userdata */
	purple_cmd_register("w",
		"s",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_send_whisper,
		"whisper &lt;message&gt;: send a whisper message",
		GINT_TO_POINTER(1));                 /* userdata */

	
	/* clan commands */
	purple_cmd_register("clan",
		"ws",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_clan_commands,
		_("clan invite - invite to clan\nmessage or m - clan message\nother not implemented"),
		NULL);   
	purple_cmd_register("c",
		"ws",                  /* args: recipient and message */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_clan_commands,
		_("clan invite - invite to clan\nmessage or m - clan message\nother not implemented"),
		NULL); 


	/* whois */
	purple_cmd_register("whois",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_who,
		_("Request user status"),
		NULL); 
	/* whois */
	purple_cmd_register("who",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_IM|PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_who,
		_("Request user status"),
		NULL); 

	/* chat joining/leaving */
	purple_cmd_register("j",
		"s",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_join,
		_("Join a new chat"),
		GINT_TO_POINTER(0));
	purple_cmd_register("join",
		"s",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_join,
		_("Join a new chat"),
		GINT_TO_POINTER(0));
	purple_cmd_register("j",
		"ws",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_join,
		_("Join a new chat"),
		GINT_TO_POINTER(1));
	purple_cmd_register("join",
		"ws",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_join,
		_("Join a new chat"),
		GINT_TO_POINTER(1));
	purple_cmd_register("l",
		"",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_leave,
		_("Leave an existing chat"),
		NULL);
	purple_cmd_register("leave",
		"",
		PURPLE_CMD_P_DEFAULT,
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_leave,
		_("Leave an existing chat"),
		NULL);

	/* topic */
	purple_cmd_register("topic",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_topic,
		_("Set channel topic"),
		NULL); 
	purple_cmd_register("t",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_topic,
		_("Set channel topic"),
		NULL); 

	/* password */
	purple_cmd_register("password",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
		"prpl-hon",
		honprpl_password,
		_("Set channel password"),
		NULL); 
	purple_cmd_register("pass",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
		"prpl-hon",
		honprpl_password,
		_("Set channel password"),
		NULL); 

	/* kick */
	purple_cmd_register("kick",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Kick user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_KICK)); 
	purple_cmd_register("k",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Kick user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_KICK)); 

	/* promote */
	purple_cmd_register("promote",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Promote user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_PROMOTE));
	purple_cmd_register("p",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Promote user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_PROMOTE)); 
	/* demote */
	purple_cmd_register("demote",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Promote user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_DEMOTE)); 
	purple_cmd_register("d",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Promote user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_DEMOTE)); 

	/* ban */
	purple_cmd_register("ban",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Ban user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_BAN)); 
	/* unban */
	purple_cmd_register("unban",
		"s",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_kick,
		_("Unban user"),
		GINT_TO_POINTER(HON_CS_CHANNEL_UNBAN)); 

	/* silence */
	purple_cmd_register("silence",
		"ww",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_silence,
		_("Silence user\nsilence <user> <duration in seconds>"),
		GINT_TO_POINTER(HON_CS_CHANNEL_UNBAN)); 

	/* channel auth */
	purple_cmd_register("auth",
		"wws",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
		"prpl-hon",
		honprpl_channel_auth,
		_("auth enable\nauth disable\nauth add\nauth delete\nauth list"),
		GINT_TO_POINTER(HON_CS_CHANNEL_UNBAN)); 
	purple_cmd_register("a",
		"wws",                  /* args: user */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT | PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS,
		"prpl-hon",
		honprpl_channel_auth ,
		_("auth enable\nauth disable\nauth add\nauth delete\nauth list"),
		GINT_TO_POINTER(HON_CS_CHANNEL_UNBAN)); 
	
	purple_cmd_register("e",
		"s",                  /* args: string */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_channel_emote ,
		_("emote string to send emote string :-/"),
		NULL); 
	purple_cmd_register("emote",
		"s",                  /* args: string */
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT,
		"prpl-hon",
		honprpl_channel_emote ,
		_("emote string to send emote string :-/"),
		NULL); 
#if 0
	purple_cmd_register("joingame",
		"wws",
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT|PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_join_game ,
		_("pretend to join a game\n address:port matchid gamename"),
		NULL);
#endif
	purple_cmd_register("whisperbuddies",
		"s",
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT|PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_whisper_buddies ,
		_("whisper all buddies"),
		NULL);
	purple_cmd_register("wb",
		"s",
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT|PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_whisper_buddies ,
		_("whisper all buddies"),
		NULL); 
	purple_cmd_register("buddy",
		"ws",
		PURPLE_CMD_P_DEFAULT,  /* priority */
		PURPLE_CMD_FLAG_CHAT|PURPLE_CMD_FLAG_IM,
		"prpl-hon",
		honprpl_manage_buddies ,
		_("buddy add <nickname> - add a buddy\nbuddy del <nickname> - remove buddy"),
		NULL); 
}

static void honprpl_destroy(PurplePlugin *plugin) {
	purple_debug_info(HON_DEBUG_PREFIX, "shutting down\n");
}


static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,                                     /* magic */
	PURPLE_MAJOR_VERSION,                                    /* major_version */
	PURPLE_MINOR_VERSION,                                    /* minor_version */
	PURPLE_PLUGIN_PROTOCOL,                                  /* type */
	NULL,                                                    /* ui_requirement */
	0,                                                       /* flags */
	NULL,                                                    /* dependencies */
	PURPLE_PRIORITY_DEFAULT,                                 /* priority */
	honprpl_ID,                                             /* id */
	"Heroes of Newerth",                                 /* name */
	DISPLAY_VERSION,                                         /* version */
	N_("Heroes of Newerth Protocol Plugin"),                              /* summary */
	N_("Protocol Plugin for Heroes of Newerth chat server"),                              /* description */
	NULL,                                                    /* author */
	"http://theli.is-a-geek.org/",                                          /* homepage */
	NULL,                                                    /* load */
	NULL,                                                    /* unload */
	honprpl_destroy,                                        /* destroy */
	NULL,                                                    /* ui_info */
	&prpl_info,                                              /* extra_info */
	NULL,                                                    /* prefs_info */
	honprpl_actions,                                        /* actions */
	NULL,                                                    /* padding... */
	NULL,
	NULL,
	NULL,
};

PURPLE_INIT_PLUGIN(honprpl, honprpl_init, info);
