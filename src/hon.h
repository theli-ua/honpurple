#ifndef _HON_H
#define _HON_H
#include "connection.h"
#include "honprpl.h"

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
#define HON_FLAGS_CHAT_STUFF 0x04

#define HON_NOTIFICATION_ADDED_AS_BUDDY 0x01
#define HON_NOTIFICATION_BUDDY_ADDED 0x02
#define HON_NOTIFICATION_REMOVED_AS_BUDDY 0x03
#define HON_NOTIFICATION_BUDDY_REMOVED 0x04

const char *hon_normalize_nick(const PurpleAccount *acct, const char *input);

void hon_parse_initiall_statuses(PurpleConnection *gc,gchar* buffer);
void hon_parse_user_status(PurpleConnection *gc,gchar* buffer);
void hon_parse_pm_whisper(PurpleConnection *gc,gchar* buffer,guint8 is_whisper);
void hon_parse_channel_list(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_entering(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_message(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_join(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_leave(PurpleConnection *gc,gchar* buffer);
void hon_parse_clan_message(PurpleConnection *gc,gchar* buffer);
void hon_parse_chat_topic(PurpleConnection* gc,gchar* buffer);
void hon_parse_userinfo(PurpleConnection* gc,gchar* buffer,guint8 packet_id);
void hon_parse_packet(PurpleConnection *gc, gchar* buffer, guint32 packet_length);
void hon_parse_notification(PurpleConnection *gc,gchar* buffer);
void hon_parse_global_notification(PurpleConnection *gc,gchar* buffer);
void hon_parse_max_channels(PurpleConnection *gc,gchar* buffer);
void hon_parse_whisper_failed(PurpleConnection *gc,gchar* buffer);
void hon_parse_pm_failed(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_kick(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_promote_demote(PurpleConnection *gc,gchar* buffer,guint8 packet_id);
void hon_parse_channel_banned(PurpleConnection *gc,gchar* buffer);
void hon_parse_channel_ban_unban(PurpleConnection *gc,gchar* buffer,guint8 packet_id);

gboolean hon_send_pong(PurpleConnection *gc);
gboolean hon_send_login(PurpleConnection *gc,const gchar* cookie);
gboolean hon_send_pm(PurpleConnection* gc,const gchar *username,const gchar* message);
gboolean hon_send_join_chat(PurpleConnection* gc,const gchar *room);
gboolean hon_send_leave_chat(PurpleConnection* gc,gchar* name);
gboolean hon_send_chat_message(PurpleConnection *gc, guint32 id, const char *message);
gboolean hon_send_chat_topic(PurpleConnection *gc, guint32 id, const char *topic);
gboolean hon_send_room_list_request(PurpleConnection *gc);
gboolean hon_send_whisper(PurpleConnection* gc,const gchar *username,const gchar* message);
gboolean hon_send_clan_invite(PurpleConnection* gc,const gchar *username);
gboolean hon_send_clan_message(PurpleConnection* gc,const gchar *message);
gboolean hon_send_whois(PurpleConnection* gc,const gchar *username);
gboolean hon_send_remove_buddy_notification(PurpleConnection* gc,guint32 buddyid, guint32 code1, guint32 code2);
gboolean hon_send_add_buddy_notification(PurpleConnection* gc,guint32 buddyid, guint32 code1, guint32 code2);
gboolean hon_send_channel_kick(PurpleConnection* gc,guint32 chatid, guint32 kickedid);
gboolean hon_send_channel_promote(PurpleConnection* gc,guint32 chatid, guint32 promotedid);
gboolean hon_send_channel_demote(PurpleConnection* gc,guint32 chatid, guint32 demotedid);
gboolean hon_send_channel_ban(PurpleConnection* gc,guint32 chatid, const gchar* banned);
gboolean hon_send_channel_unban(PurpleConnection* gc,guint32 chatid, const gchar* banned);
#endif
