#ifndef _HONPRPL_H
#define _HONPRPL_H
#include <glib.h>

#ifdef _WIN32
#include <libc_interface.h>
#undef vsnprintf /* conflicts with msvc definition and not needed */
#endif

#include "utils.h"
#include <roomlist.h>



#define honprpl_ID "prpl-hon"


#define HON_STATUS_ONLINE_S   "online"
#define HON_STATUS_OFFLINE_S  "offline"
#define HON_STATUS_INGAME_S  "ingame"


#define HON_BUDDIES_GROUP  "HoN Buddies"
#define HON_CLANMATES_GROUP  "HoN Clan Roster"


#define HON_DEBUG_PREFIX "honprpl"

#define DISPLAY_VERSION "0.3"
#define HON_CLIENT_REQUESTER "client_requester.php"
#define HON_DEFAULT_MASTER_SERVER "http://masterserver.hon.s2games.com/"

#define HON_REMOVE_BUDDY_REQUEST "%s/%s?f=remove_buddy&account_id=%d&buddy_id=%d&cookie=%s"
#define HON_ADD_BUDDY_REQUEST "%s/%s?f=new_buddy&account_id=%d&buddy_id=%d&cookie=%s"
#define HON_NICK2ID_REQUEST "%s%s?f=nick2id&nickname[]=%s"
#define HON_STATS_REQUEST "%s%s?f=get_all_stats&account_id[0]=%d"

#define IS_MD5_OPTION "pass_is_md5"

#define HON_CHAT_PORT 11031


#define HON_PROTOCOL_ICON "hon"
#define HON_INGAME_EMBLEM "hon_ingame"
#define HON_PREMIUM_EMBLEM "hon_premium"

#define N_(x) (x)
#define _(x) (x)

typedef struct {
	gchar* nickname;
	guint account_id;
	gchar* clan_name;
	gchar* clan_tag;
} user_info;

typedef struct {
	deserialized_element* account_data;
	GHashTable* buddies;
	GHashTable* clanmates;
	GHashTable* ignores;
	GHashTable* banned;
	GHashTable* clan_info;
	GHashTable* id2nick;
	gchar* cookie;
	user_info self;
	PurpleRoomlist* roomlist;
	int fd;
	PurpleConversation* whois_conv;
	guint32 got_length;
	GByteArray* databuff;
} hon_account;

typedef void (*nick2idCallback)(PurpleBuddy* buddy);
typedef struct {
	nick2idCallback cb;
	nick2idCallback error_cb;
	PurpleBuddy* buddy;
}nick2id_cb_data;

#undef MINBIF
#ifdef MINBIF
#define MINBIF_USER "minbif"
#define MINBIF_STATUS "internal_status"
#define MINBIF_INFO "internal_info"
#endif
#endif
