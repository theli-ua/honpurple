#ifndef _HON_H
#define _HON_H
#include "connection.h"
#include "honprpl.h"

#ifdef _WIN32
#define MSG_WAITALL 0
#endif

#define HON_STATUS_ATTR "status"
#define HON_FLAGS_ATTR "flags"
#define HON_GAME_ATTR "game"
#define HON_MATCHID_ATTR "matchid"
#define HON_SERVER_ATTR "server"
#define HON_BUDDYID_ATTR "buddy_id"

#define HON_STATUS_OFFLINE	0
#define HON_STATUS_ONLINE	3
#define HON_STATUS_INLOBBY	4
#define HON_STATUS_INGAME	5

#define HON_FLAGS_PREPURCHASED 0x40
#define HON_FLAGS_CHAT_NONE 0x00
#define HON_FLAGS_CHAT_OFFICER 0x01
#define HON_FLAGS_CHAT_LEADER 0x02
#define HON_FLAGS_CHAT_ADMINISTRATOR 0x03
#define HON_FLAGS_CHAT_STAFF 0x04

#define HON_NOTIFICATION_ADDED_AS_BUDDY 0x01
#define HON_NOTIFICATION_BUDDY_ACCEPTED 0x02
#define HON_NOTIFICATION_REMOVED_AS_BUDDY 0x03
#define HON_NOTIFICATION_BUDDY_REMOVED 0x04

const char *hon_normalize_nick(const PurpleAccount *acct, const char *input);
int hon_parse_packet(PurpleConnection *gc, gchar* buffer,int packet_length);

void hon_parse_initiall_statuses(PurpleConnection *gc,gchar* buffer);
void hon_parse_user_status(PurpleConnection *gc,gchar* buffer);
void hon_parse_pm_whisper(PurpleConnection *gc,gchar* buffer,guint16 is_whisper);
void hon_parse_chat_entering(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_message(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_join(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_leave(PurpleConnection *gc,gchar* buffer);
void hon_parse_clan_message(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_topic(PurpleConnection* gc,gchar* buffer);
void hon_parse_userinfo(PurpleConnection* gc,gchar* buffer,guint16 packet_id);
void hon_parse_notification(PurpleConnection *gc,gchar* buffer);
void hon_parse_global_notification(PurpleConnection *gc,gchar* buffer);
void hon_parse_max_channels(PurpleConnection *gc,gchar* buffer);
void hon_parse_whisper_failed(PurpleConnection *gc,gchar* buffer);
void hon_parse_pm_failed(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_kick(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_promote_demote(PurpleConnection *gc,gchar* buffer,guint16 packet_id);
void hon_parse_channel_banned(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_ban_unban(PurpleConnection *gc,gchar* buffer,guint16 packet_id);
void hon_parse_join_channel_password(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_password_changed(PurpleConnection* gc,gchar* buffer);
void hon_parse_channel_silenced(PurpleConnection* gc,gchar* buffer);
void hon_parse_channel_silence_lifted(PurpleConnection* gc,gchar* buffer);
void hon_parse_channel_silence_placed(PurpleConnection* gc,gchar* buffer);
void hon_parse_channel_auth_enable_disable(PurpleConnection *gc,gchar* buffer,guint16 packet_id);
void hon_parse_channel_auth_add_delete(PurpleConnection *gc,gchar* buffer,guint16 packet_id);
void hon_parse_channel_auth_list(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_update(PurpleConnection *gc,gchar* buffer);
void hon_parse_emote(PurpleConnection *gc,gchar* buffer);
void hon_parse_request(PurpleConnection *gc,gchar* buffer);


gboolean hon_send_pong(PurpleConnection *gc);
gboolean hon_send_login(PurpleConnection *gc,const gchar* cookie,guint32 protocolVersion);
gboolean hon_send_pm(PurpleConnection* gc,const gchar *username,const gchar* message);
gboolean hon_send_join_chat(PurpleConnection* gc,const gchar *room);
gboolean hon_send_leave_chat(PurpleConnection* gc,gchar* name);
gboolean hon_send_chat_message(PurpleConnection *gc, guint32 id, const char *message);
gboolean hon_send_chat_topic(PurpleConnection *gc, guint32 id, const char *topic);
gboolean hon_send_whisper(PurpleConnection* gc,const gchar *username,const gchar* message);
gboolean hon_send_clan_invite(PurpleConnection* gc,const gchar *username);
gboolean hon_send_clan_message(PurpleConnection* gc,const gchar *message);
gboolean hon_send_whois(PurpleConnection* gc,const gchar *username);
gboolean hon_send_add_buddy_notification(PurpleConnection* gc,guint32 selfid, gchar* nick);
gboolean hon_send_channel_kick(PurpleConnection* gc,guint32 chatid, guint32 kickedid);
gboolean hon_send_channel_promote(PurpleConnection* gc,guint32 chatid, guint32 promotedid);
gboolean hon_send_channel_demote(PurpleConnection* gc,guint32 chatid, guint32 demotedid);
gboolean hon_send_channel_ban(PurpleConnection* gc,guint32 chatid, const gchar* banned);
gboolean hon_send_channel_unban(PurpleConnection* gc,guint32 chatid, const gchar* banned);
gboolean hon_send_channel_password(PurpleConnection* gc,guint32 chatid, const gchar* password);
gboolean hon_send_join_chat_password(PurpleConnection* gc,const gchar *room,const gchar* password);
gboolean hon_send_channel_silence(PurpleConnection* gc,guint32 chatid, const gchar* user,guint32 duration);
gboolean hon_send_channel_auth_enable(PurpleConnection* gc,guint32 chatid);
gboolean hon_send_channel_auth_disable(PurpleConnection* gc,guint32 chatid);
gboolean hon_send_channel_auth_add(PurpleConnection* gc,guint32 chatid,gchar* username);
gboolean hon_send_channel_auth_delete(PurpleConnection* gc,guint32 chatid,gchar* username);
gboolean hon_send_channel_auth_list(PurpleConnection* gc,guint32 chatid);
gboolean hon_send_emote(PurpleConnection* gc,guint32 chatid,const gchar* string);
gboolean hon_send_join_game(PurpleConnection* gc,const gchar* status,guint32 matchid,gchar* server);
gboolean hon_send_whisper_buddies(PurpleConnection* gc,const gchar* message);

gboolean hon_send_accept_buddy(PurpleConnection* gc,const gchar* buddy);

#endif
