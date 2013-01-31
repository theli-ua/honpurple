#ifndef _HONPRPL_H
#define _HONPRPL_H
#include <glib.h>

#ifdef _WIN32
#include <libc_interface.h>
#undef vsnprintf /* conflicts with msvc definition and not needed */
#endif

#include "utils.h"
#include <roomlist.h>
#include <request.h>
#include "srp.h"



#define honprpl_ID "prpl-hon"


#define HON_STATUS_ONLINE_S   "online"
#define HON_STATUS_OFFLINE_S  "offline"
#define HON_STATUS_INGAME_S  "ingame"


#define HON_BUDDIES_GROUP  "HoN Buddies"
#define HON_CLANMATES_GROUP  "HoN Clan Roster"


#define HON_DEBUG_PREFIX "honprpl"

#define DISPLAY_VERSION "0.5.11.4"
#define HON_CLIENT_REQUESTER "client_requester.php"
#define HON_DEFAULT_MASTER_SERVER "http://masterserver.hon.s2games.com/"

#define HON_REMOVE_BUDDY_REQUEST "%s/%s?f=remove_buddy&account_id=%d&buddy_id=%d&cookie=%s"
#define HON_ADD_BUDDY_REQUEST "%s/%s?f=new_buddy&account_id=%d&buddy_id=%d&cookie=%s"
#define HON_NICK2ID_REQUEST "%s%s?f=nick2id&nickname[]=%s"
#define HON_STATS_REQUEST "%s%s?f=get_all_stats&account_id[0]=%d"

#define IS_MD5_OPTION "pass_is_md5"
#define login_useSRP "login_useSRP"
#define PROT_VER_STRING "protover"

#define HON_CHAT_PORT 11031


#define HON_PROTOCOL_ICON "hon"
#define HON_INGAME_EMBLEM "hon_ingame"
#define HON_PREMIUM_EMBLEM "hon_premium"

#define HON_NETWORK_TIMEOUT 30

#ifndef N_
#	define N_(x) (x)
#endif
#ifndef _
#	define _(x) (x)
#endif
void honpurple_get_icon(PurpleAccount* account,const gchar* nick, const gchar* icon, guint32 accountid);

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
	gchar* ip;
	gchar* auth;
	user_info self;
	PurpleRoomlist* roomlist;
	int fd;
	PurpleConversation* whois_conv;
	guint32 got_length;
	GByteArray* databuff;
	guint timeout_handle;
	gboolean gotPacket;
} hon_account;

typedef void (*nick2idCallback)(PurpleBuddy* buddy);
typedef struct {
	nick2idCallback cb;
	nick2idCallback error_cb;
	PurpleBuddy* buddy;
}nick2id_cb_data;

typedef struct {
    struct SRPUser     * usr;
    PurpleConnection   * gc;
	GString* password_md5;
} srp_auth_cb_data;


#define S2_N "DA950C6C97918CAE89E4F5ECB32461032A217D740064BC12FC0723CD204BD02A7AE29B53F3310C13BA998B7910F8B6A14112CBC67BDD2427EDF494CB8BCA68510C0AAEE5346BD320845981546873069B337C073B9A9369D500873D647D261CCED571826E54C6089E7D5085DC2AF01FD861AE44C8E64BCA3EA4DCE942C5F5B89E5496C2741A9E7E9F509C261D104D11DD4494577038B33016E28D118AE4FD2E85D9C3557A2346FAECED3EDBE0F4D694411686BA6E65FEE43A772DC84D394ADAE5A14AF33817351D29DE074740AA263187AB18E3A25665EACAA8267C16CDE064B1D5AF0588893C89C1556D6AEF644A3BA6BA3F7DEC2F3D6FDC30AE43FBD6D144BB"
#define S2_G "2"
#define S2_SRP_MAGIC1 "[!~esTo0}"
#define S2_SRP_MAGIC2 "taquzaph_?98phab&junaj=z=kuChusu"

#define HON_USER_AGENT "S2 Games/Heroes of Newerth/2.6.32.2/lac/x86-biarch"

#undef MINBIF
#ifdef MINBIF
#define MINBIF_USER "minbif"
#define MINBIF_STATUS "internalstatus"
#define MINBIF_INFO "internalinfo"
#endif
#endif
