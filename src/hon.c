#include "hon.h"
#include "packet_id.h"
#include "debug.h"
#include <stdarg.h>
#include <notify.h>

/*
	Macroses and utilities
 */
static int do_write(int fd, void* buffer, int len){
	return write(fd,buffer,len);
}

static gboolean hon_send_packet(PurpleConnection* gc,guint8 packet_id,const gchar* paramstring , ...){
	gboolean res;
	guint32 intparam;
	guint8 byteparam;
	const gchar* stringparam;
	va_list marker;
	hon_account* hon = gc->proto_data;
	GByteArray* buffer = g_byte_array_new();
	buffer = g_byte_array_append(buffer,&packet_id,1);
	va_start( marker, paramstring );

	while (paramstring != 0x00 && *paramstring != 0x00)
	{
		switch (*paramstring){
			case 'i':
				intparam = va_arg( marker, guint32);
				buffer = g_byte_array_append(buffer,(const guint8*)&intparam,4);
				break;
			case 's':
				stringparam = va_arg( marker, const gchar*);
				buffer = g_byte_array_append(buffer,stringparam,strlen(stringparam)+1);
				break;
			case 'b':
				byteparam = va_arg( marker, guint8);
				buffer = g_byte_array_append(buffer,&byteparam,1);
				break;
		}
		paramstring++;
	}
	va_end(marker);
	res = buffer->len == do_write(hon->fd,buffer->data,buffer->len);
	g_byte_array_free(buffer,TRUE);
	return res;
}
const char *hon_normalize_nick(const PurpleAccount *acct,
							   const char *input)
{
	if (input[0] == '[')
	{
		while (input[0] != ']')
			input++;
		input++;
	}
	return input;
}

/* Packet parsers */
void hon_parse_packet(PurpleConnection *gc, gchar* buffer, guint32 packet_length){
	guint8 packet_id = *buffer++;
	GString* hexdump;
#if 0
	hexdump = g_string_new(NULL);
	hexdump_g_string_append(hexdump,"",buffer,packet_length - 1);
	purple_debug_info(HON_DEBUG_PREFIX, "packet:\nid:%X(%d)\nlength:%d\ndata:\n%s\n",packet_id,packet_id,packet_length, hexdump->str);
	g_string_free(hexdump,TRUE);
#endif
	switch (packet_id)
	{
	case HON_SC_AUTH_ACCEPTED/*0x00*/: /* logged on ! */
		purple_connection_update_progress(gc, _("Connected"),
			3,   /* which connection step this is */
			4);  /* total number of steps */
		purple_connection_set_state(gc, PURPLE_CONNECTED);
		break;
	case HON_SC_PING/*0x01*/:
		hon_send_pong(gc);
		purple_debug_info(HON_DEBUG_PREFIX, "server ping, sending pong\n");
		break;
	case HON_SC_CHANNEL_MSG/*0x03*/:
		hon_parse_chat_message(gc,buffer);
		break;
	case HON_SC_CHANGED_CHANNEL/*0x04*/:
#ifdef _DEBUG
		hexdump = g_string_new(NULL);
		hexdump_g_string_append(hexdump,"",buffer,packet_length - 1);
		purple_debug_info(HON_DEBUG_PREFIX, "channel join packet:\nid:%X(%d)\nlength:%d\ndata:\n%s\n",packet_id,packet_id,packet_length, hexdump->str);
		g_string_free(hexdump,TRUE);
#endif
		hon_parse_chat_entering(gc,buffer);
		break;
	case HON_SC_JOINED_CHANNEL/*0x05*/:
		hon_parse_chat_join(gc,buffer);
		break;
	case HON_SC_LEFT_CHANNEL/*0x06*/:
		hon_parse_chat_leave(gc,buffer);
		break;
	case HON_SC_WHISPER/*0x08*/:
		hon_parse_pm_whisper(gc,buffer,TRUE);
		break;
	case HON_SC_WHISPER_FAILED/*0x09*/:
		hon_parse_whisper_failed(gc,buffer);
		break;
	case HON_SC_INITIAL_STATUS/*0x0B*/:
		hon_parse_initiall_statuses(gc,buffer);
		break;
	case HON_SC_UPDATE_STATUS/*0x0C*/:
#ifdef _DEBUG
		hexdump = g_string_new(NULL);
		hexdump_g_string_append(hexdump,"",buffer,packet_length - 1);
		purple_debug_info(HON_DEBUG_PREFIX, "user status packet:\nid:%X(%d)\nlength:%d\ndata:\n%s\n",packet_id,packet_id,packet_length, hexdump->str);
		g_string_free(hexdump,TRUE);
#endif
		hon_parse_user_status(gc,buffer);
		break;
	case HON_SC_NOTIFICATION/*0x12*/:
		hon_parse_notification(gc,buffer);
		break;
	case HON_SC_CLAN_MESSAGE/*0x13*/:
		hon_parse_clan_message(gc,buffer);
		break;
	case HON_SC_PM/*0x1C*/:
		hon_parse_pm_whisper(gc,buffer,FALSE);
		break;
	case HON_SC_PM_FAILED/*0x1D*/:
		hon_parse_pm_failed(gc,buffer);
		break;
	case HON_SC_CHANNEL_LIST/*0x1F*/:
		hon_parse_channel_list(gc,buffer);
		break;
	case HON_SC_MAX_CHANNELS/*0x21*/:
		hon_parse_max_channels(gc,buffer);
		break;
	case HON_SC_USER_INFO_NO_EXIST/*0x2b*/:
	case HON_SC_USER_INFO_OFFLINE/*0x2c*/:
	case HON_SC_USER_INFO_ONLINE/*0x2d*/:
	case HON_SC_USER_INFO_IN_GAME/*0x2e*/:
		hon_parse_userinfo(gc,buffer,packet_id);
		break;
	case HON_SC_UPDATE_TOPIC/*0x30*/:
		hon_parse_chat_topic(gc,buffer);
		break;
	case HON_SC_MESSAGE_ALL/*0x39*/:
		hon_parse_global_notification(gc,buffer);
		break;
	default:
		hexdump = g_string_new(NULL);
		hexdump_g_string_append(hexdump,"",buffer,packet_length - 1);
		purple_debug_info(HON_DEBUG_PREFIX, "unknown packet:\nid:%X(%d)\nlength:%d\ndata:\n%s\n",packet_id,packet_id,packet_length, hexdump->str);
		g_string_free(hexdump,TRUE);
		break;
	}
}
void hon_parse_pm_failed(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	purple_notify_error(NULL,_("Message failed"),_("The user you tried to chat with is not online"),
		NULL);
}
void hon_parse_whisper_failed(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	purple_notify_error(NULL,_("Whisper failed"),_("The user you tried to whisper is not online"),
		NULL);
}
void hon_parse_max_channels(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	purple_notify_error(NULL,_("Channel limit reached"),_("You have reached an open channels limit."),
		_("To join other channel close some already opened"));
}
void hon_parse_global_notification(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	gchar* username = read_string(buffer);
	purple_notify_warning(NULL,username,buffer,NULL);
}
void hon_parse_notification(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	guint8 notification_type = *buffer++;
	gchar* title = NULL;
	switch (notification_type)
	{
	case HON_NOTIFICATION_ADDED_AS_BUDDY:
		title = g_strdup(_("User added you as buddy"));
		break;
	case HON_NOTIFICATION_REMOVED_AS_BUDDY:
		title = g_strdup(_("User removed you as buddy"));
		break;
	default :
		title = g_strdup_printf(_("Unknown notification type (%d)"),notification_type);
		break;
	}
	purple_notify_info(NULL,title,buffer,NULL);
	g_free(title);
}
void hon_parse_initiall_statuses(PurpleConnection *gc,gchar* buffer){
	guint32 status,flags;
	hon_account* hon;
	guint32 id,count = read_guint32(buffer);
	hon = gc->proto_data;
	purple_debug_info(HON_DEBUG_PREFIX, "parsing status for %d buddies\n",count);
	while (count-- > 0)
	{
		gchar* nick,*gamename=NULL, *server=NULL,*status_id = HON_STATUS_ONLINE_S;

		id = read_guint32(buffer);
		status = *buffer++;
		flags = *buffer++;
		nick = g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(id));
		if (status == HON_STATUS_INLOBBY || status == HON_STATUS_INGAME)
		{
			server = read_string(buffer);
			status_id = HON_STATUS_INGAME_S;
		}
		if (status == HON_STATUS_INGAME)
		{
			gamename = read_string(buffer);
			gamename = hon_strip(gamename,TRUE);
		}
		if(!status)
			status_id = HON_STATUS_OFFLINE_S;
		purple_debug_info(HON_DEBUG_PREFIX, "status for %s,flags:%d,status:%d,game:%s,server:%s\n",nick,flags,status,gamename,server);
		purple_prpl_got_user_status(gc->account, nick, status_id,
			HON_BUDDYID_ATTR , id,
			HON_STATUS_ATTR,status,HON_FLAGS_ATTR,flags,
			server ? HON_SERVER_ATTR : NULL,server,gamename ? HON_GAME_ATTR : NULL,gamename,NULL);

		g_free(gamename);

	}
}
void hon_parse_user_status(PurpleConnection *gc,gchar* buffer){
	gchar* nick,*gamename=NULL, *server=NULL,*status_id = HON_STATUS_ONLINE_S;
	gchar* clan; // or channel?
	guint32 clanid;
	hon_account* hon = gc->proto_data;
	guint32 status;
	guint32 flags;
	guint32 matchid = 0;

	guint32 id = read_guint32(buffer);
	status = *buffer++;
	flags = *buffer++;
	nick = g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(id));
	/* TODO: figure this out */
	clanid = read_guint32(buffer);
	clan = read_string(buffer); // huh ?
	if (status == HON_STATUS_INLOBBY || status == HON_STATUS_INGAME)
	{
		server = read_string(buffer);
		status_id = HON_STATUS_INGAME_S;
	}
	if (status == HON_STATUS_INGAME)
	{
		gamename = read_string(buffer);
		gamename = hon_strip(gamename,TRUE);
		matchid = read_guint32(buffer);
	}
	if(!status)
		status_id = HON_STATUS_OFFLINE_S;
	purple_debug_info(HON_DEBUG_PREFIX, "status for %s,flags:%d,status:%d,game:%s,server:%s\nclanid:%d, clan?:%s matchid:%d\n"
		,nick,flags,status,gamename,server,clanid,clan,matchid);
	purple_prpl_got_user_status(gc->account, nick, status_id,
		HON_STATUS_ATTR,status,HON_FLAGS_ATTR,flags,
		HON_BUDDYID_ATTR , id,
		server ? HON_SERVER_ATTR : NULL,server,gamename ? HON_GAME_ATTR : NULL,gamename,
		matchid > 0 ? HON_MATCHID_ATTR : NULL, matchid,
		NULL);
	g_free(gamename);
}


void hon_parse_pm_whisper(PurpleConnection *gc,gchar* buffer,guint8 is_whisper)
{
	hon_account* hon = gc->proto_data;
	PurpleMessageFlags receive_flags;
	gchar* message,*from_username = read_string(buffer);
	message = hon2html(buffer);
	if (from_username[0] == '[')
	{
		while (from_username[0] != ']')
			from_username++;
		from_username++;
	}
	if (is_whisper)
		receive_flags = PURPLE_MESSAGE_WHISPER;
	else
		receive_flags = PURPLE_MESSAGE_RECV;
	serv_got_im(gc, from_username, message, receive_flags, time(NULL));
	g_free(message);
}
void hon_parse_channel_list(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	guint32 count = read_guint32(buffer);
	if (!hon->roomlist)
		return;
	while (count--)
	{
		PurpleRoomlistRoom *room;
		gchar* name,*colorname;
		guint32 id,participants;
		id = read_guint32(buffer);
		name = read_string(buffer);
		colorname = hon_strip(name,FALSE);
		participants = read_guint32(buffer);

		room = purple_roomlist_room_new(PURPLE_ROOMLIST_ROOMTYPE_ROOM, colorname, NULL);

		purple_roomlist_room_add_field(hon->roomlist, room, GINT_TO_POINTER(id));
		purple_roomlist_room_add_field(hon->roomlist, room, name);
		purple_roomlist_room_add_field(hon->roomlist, room, GINT_TO_POINTER(participants));
		purple_roomlist_room_add(hon->roomlist, room);
		g_free(colorname);
	}
	purple_roomlist_set_in_progress(hon->roomlist, FALSE);
	purple_roomlist_unref(hon->roomlist);
	hon->roomlist = NULL;

}
void hon_parse_chat_entering(PurpleConnection *gc,gchar* buffer)
{
	PurpleConversation *convo;
	hon_account* hon = gc->proto_data;
	guint8 unknown,flags;
	guint32 op_count,chat_id,count;
	guint32 purple_flags = 0;
	gchar* topic,*topic_raw;
	const gchar* extra;
	GHashTable* ops = NULL;

	gchar* room = buffer;
	buffer += strlen(buffer) + 1;
	chat_id = read_guint32(buffer);
	unknown = *buffer++;
	topic_raw = read_string(buffer);
	topic = hon2html(topic_raw);
	topic_raw = hon_strip(topic_raw,FALSE);
	op_count = read_guint32(buffer);
	if (op_count != 0)
	{
		guint32 op_id,op_type;
		ops = g_hash_table_new(g_direct_hash,g_direct_equal);
		while (op_count--)
		{
			op_id = read_guint32(buffer);
			op_type = *buffer++;
			g_hash_table_insert(ops,GINT_TO_POINTER(op_id),GINT_TO_POINTER(op_type));
		}
	}
	count = read_guint32(buffer);
	convo = serv_got_joined_chat(gc, chat_id, room);
	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(convo), NULL, topic_raw);

	while (count--)
	{
		guint32 account_id;
		guint8 status;
		const gchar* nickname = read_string(buffer);
		account_id = read_guint32(buffer);
		status = *buffer++;
		flags = *buffer++;
		purple_debug_info(HON_DEBUG_PREFIX, "room participant: %s , id=%d,status=%d,flags=%d\n",
			nickname,account_id,status,flags);

		flags |= GPOINTER_TO_INT(g_hash_table_lookup(ops,GINT_TO_POINTER(account_id)));

		purple_flags = PURPLE_CBFLAGS_NONE;

		if (flags & HON_FLAGS_CHAT_FOUNDER)
		{
			purple_flags |= PURPLE_CBFLAGS_FOUNDER;
		}
		else if (flags & HON_FLAGS_CHAT_MOD)
			purple_flags |= PURPLE_CBFLAGS_OP;


		extra = nickname;
		nickname = hon_normalize_nick(gc->account,nickname);
		purple_conv_chat_add_user(PURPLE_CONV_CHAT(convo), nickname, extra, purple_flags, FALSE);
		if (!g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(account_id)))
		{
			g_hash_table_insert(hon->id2nick,GINT_TO_POINTER(account_id),g_strdup(nickname));
		}

	}
	flags = 0;
	purple_flags = PURPLE_CBFLAGS_NONE;

	if(ops)
	{
		flags = GPOINTER_TO_INT(g_hash_table_lookup(ops,GINT_TO_POINTER(hon->self.account_id)));
		g_hash_table_destroy(ops);
	}
	if (flags & HON_FLAGS_CHAT_FOUNDER)
	{
		purple_flags |= PURPLE_CBFLAGS_FOUNDER;
	}
	else if (flags & HON_FLAGS_CHAT_MOD)
		purple_flags |= PURPLE_CBFLAGS_OP;

	purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", topic, PURPLE_MESSAGE_SYSTEM|PURPLE_MESSAGE_NO_LOG, time(NULL));
	g_free(topic);
	g_free(topic_raw);

	purple_conv_chat_add_user(PURPLE_CONV_CHAT(convo), hon->self.nickname, NULL,purple_flags , FALSE);
}
void hon_parse_chat_message(PurpleConnection *gc,gchar* buffer){
	hon_account *hon = gc->proto_data;
	guint32 account_id;
	guint32 chan_id;
	gchar* msg;
	gchar* sender;

	account_id = read_guint32(buffer);
	chan_id = read_guint32(buffer);
	msg = hon2html(buffer);
	sender = g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(account_id));
	serv_got_chat_in(gc,chan_id,sender? sender : "unknown user" ,PURPLE_MESSAGE_RECV,msg,time(NULL));
	g_free(msg);
}
void hon_parse_chat_join(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	guint32 account_id;
	guint32 chan_id,purple_flags = PURPLE_CBFLAGS_NONE;
	PurpleConversation* conv;
	guint8 status,flags;
	const gchar* extra;
	const gchar* nick = read_string(buffer);
	account_id = read_guint32(buffer);
	chan_id = read_guint32(buffer);
	conv = purple_find_chat(gc,chan_id);

	extra = nick;
	nick = hon_normalize_nick(gc->account,nick);
	status = *buffer++;
	flags = *buffer++;

	if (flags & HON_FLAGS_CHAT_FOUNDER)
	{
		purple_flags |= PURPLE_CBFLAGS_FOUNDER;
	}
	else if (flags & HON_FLAGS_CHAT_MOD)
		purple_flags |= PURPLE_CBFLAGS_OP;


	if (conv)
	{
		purple_conv_chat_add_user(PURPLE_CONV_CHAT(conv),nick,extra,purple_flags,TRUE);
	}
	if (!g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(account_id)))
	{
		g_hash_table_insert(hon->id2nick,GINT_TO_POINTER(account_id),g_strdup(nick));
	}
}
void hon_parse_chat_leave(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	guint32 account_id;
	guint32 chan_id;
	gchar* nick;
	PurpleConversation* conv;
	account_id = read_guint32(buffer);
	chan_id = read_guint32(buffer);
	nick = g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(account_id));
	conv = purple_find_chat(gc,chan_id);
	if (conv && nick)
	{
		purple_conv_chat_remove_user(PURPLE_CONV_CHAT(conv),nick,"");
	}
}
void hon_parse_clan_message(PurpleConnection *gc,gchar* buffer){
	hon_account* hon = gc->proto_data;
	guint32 buddy_id;
	gchar* message,*user;
	PurpleConversation* clanConv;
	GString* clan_chat_name;
	buddy_id = read_guint32(buffer);
	message = hon2html(buffer);
	clan_chat_name = g_string_new("Clan ");
	clan_chat_name = g_string_append(clan_chat_name,hon->self.clan_name);
	clanConv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_CHAT,clan_chat_name->str,gc->account);
	g_string_free(clan_chat_name,TRUE);
	if (clanConv)
	{
		user = g_hash_table_lookup(hon->id2nick,GINT_TO_POINTER(buddy_id));
		purple_conv_chat_write(PURPLE_CONV_CHAT(clanConv), user,message, PURPLE_MESSAGE_WHISPER, time(NULL));
	}
	g_free(message);
}
void hon_parse_chat_topic(PurpleConnection* gc,gchar* buffer){
	PurpleConversation *convo;
	hon_account* hon = gc->proto_data;
	gchar * topic_raw, * topic_html, * msg;
	guint32 chan_id = read_guint32(buffer);

	convo = purple_find_chat(gc,chan_id);
	if (!convo) {
		purple_debug(PURPLE_DEBUG_ERROR, HON_DEBUG_PREFIX, "Got a topic for %d, which doesn't exist\n", chan_id);
		return;
	}
	topic_raw = hon_strip(buffer,FALSE);
	topic_html = hon2html(buffer);
	msg = g_strdup_printf(_("%s has changed the topic to: %s"), "someone", topic_html);

	purple_conv_chat_set_topic(PURPLE_CONV_CHAT(convo), NULL, topic_raw);
	purple_conv_chat_write(PURPLE_CONV_CHAT(convo), "", msg, PURPLE_MESSAGE_SYSTEM, time(NULL));

	g_free(topic_raw);
	g_free(topic_html);
	g_free(msg);
}

void hon_parse_userinfo(PurpleConnection* gc,gchar* buffer,guint8 packet_id){
	hon_account* hon = gc->proto_data;
	/* TODO: this is not right .. conversation could be closed already */
	gchar* message = NULL;
	gchar* user = read_string(buffer);
	if (!hon->whois_conv)
		return;

	switch (packet_id){
	case 0x2b:
		message = g_strdup_printf(_("Cannot find user %s"),user);
		break;
	case 0x2c:
		message = g_strdup_printf(_("User %s is offline, last seen %s"),user,buffer);
		break;
	case 0x2d:
		{
			GString* msg = g_string_new(NULL);
			guint32 chan_count = read_guint32(buffer);
			if (chan_count > 0)
				g_string_printf(msg,_("User %s is online and in channels: "),user);
			else
				g_string_printf(msg,_("User %s is online."),user);
			while (chan_count--)
			{
				msg = g_string_append(msg,buffer);
				buffer += strlen(buffer) + 1;
				if (chan_count == 0)
					msg = g_string_append(msg,".");
				else
					msg = g_string_append(msg,", ");
			}
			message = g_string_free(msg,FALSE);
		}
		break;
	case 0x2e:
		message = g_strdup_printf(_("User %s is ingame, game name: %s, game time: %s"),user,buffer,buffer + (strlen(buffer) + 1));
		break;
	}

	purple_conversation_write(hon->whois_conv, "",message, PURPLE_MESSAGE_SYSTEM|PURPLE_MESSAGE_NO_LOG, time(NULL));
	g_free(message);
	hon->whois_conv = NULL;
}


/* packet creators */
gboolean hon_send_pong(PurpleConnection *gc){
	return hon_send_packet(gc,HON_CS_PONG/*0x02*/,"");
}
gboolean hon_send_login(PurpleConnection *gc, const gchar* cookie){
	hon_account* hon = gc->proto_data;
	return hon_send_packet(gc,HON_CS_AUTH_INFO/*0xFF*/,"isi",hon->self.account_id,hon->cookie,2);
}
gboolean hon_send_pm(PurpleConnection* gc,const gchar *username,const gchar* message){
	return hon_send_packet(gc,HON_CS_PM/*0x1C*/,"ss",username,message);
}
gboolean hon_send_join_chat(PurpleConnection* gc,const gchar *room){
	return hon_send_packet(gc,HON_CS_JOIN_CHANNEL/*0x1e*/,"s",room);
}
gboolean hon_send_leave_chat(PurpleConnection* gc,gchar* name){
	return hon_send_packet(gc,HON_CS_LEAVE_CHANNEL/*0x22*/,"s",name);
}
gboolean hon_send_chat_message(PurpleConnection *gc, guint32 id, const char *message){
	return hon_send_packet(gc,HON_CS_CHANNEL_MSG/*0x03*/,"is",id,message);
}
gboolean hon_send_chat_topic(PurpleConnection *gc, guint32 id, const char *topic){
	return hon_send_packet(gc,HON_CS_UPDATE_TOPIC/*0x30*/,"is",id,topic);
}
gboolean hon_send_room_list_request(PurpleConnection *gc){
	return hon_send_packet(gc,HON_CS_CHANNEL_LIST/*0x1F*/,"");
}
gboolean hon_send_whisper(PurpleConnection* gc,const gchar *username,const gchar* message){
	return hon_send_packet(gc,HON_CS_WHISPER/*0x08*/,"ss",username,message);
}
gboolean hon_send_clan_invite(PurpleConnection* gc,const gchar *username){
	return hon_send_packet(gc,HON_CS_CLAN_ADD_MEMBER/*0x47*/,"s",username);
}
gboolean hon_send_clan_message(PurpleConnection* gc,const gchar *message){
	return hon_send_packet(gc,HON_CS_CLAN_MESSAGE/*0x13*/,"s",message);
}
gboolean hon_send_whois(PurpleConnection* gc,const gchar *username){
	return hon_send_packet(gc,HON_CS_USER_INFO/*0x2a*/,"s",username);
}