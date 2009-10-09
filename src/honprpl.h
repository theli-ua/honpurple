#ifndef _HONPRPL_H
#define _HONPRPL_H
#include <glib.h>

#ifdef _WIN32
#include <libc_interface.h>
#undef vsnprintf /* conflicts with msvc definition */
#endif

#include "utils.h"

#define honprpl_ID "prpl-hon"


#define HON_STATUS_ONLINE   "online"
#define HON_STATUS_AWAY     "ingame"
#define HON_STATUS_OFFLINE  "offline"

#define HON_BUDDIES_GROUP  "HoN Buddies"
#define HON_CLANMATES_GROUP  "HoN Clan Roster"


#define HON_DEBUG_PREFIX "honprpl"

#define DISPLAY_VERSION "0.1"
#define HON_CLIENT_REQUESTER "client_requester.php"
#define HON_DEFAULT_MASTER_SERVER "http://masterserver.hon.s2games.com/"

#define IS_MD5_OPTION "pass_is_md5"

#define HON_CHAT_PORT 11031


#define STATUS_OFFLINE	0
#define STATUS_UNKNOWN	3
#define STATUS_INLOBBY	4
#define STATUS_INGAME	5

#define FLAGS_PREPURCHASED 0x40
#define FLAGS_CHAT_MOD 0x01

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

	int fd;
} hon_account;

#endif
