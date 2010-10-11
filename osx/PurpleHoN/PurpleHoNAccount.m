//
//  PurpleHoNAccount.m
//  PurpleHoN
//
//  Created by Dopefish on 2010-10-10.
//  Copyright 2010 David Rudie. All rights reserved.
//

#import "PurpleHoNAccount.h"
#import <Adium/AIPlugin.h>
#import <Adium/AIHTMLDecoder.h>
#import <Adium/AIListContact.h>
#import <Adium/AIChat.h>
#import <Adium/AIContentMessage.h>
#import <Adium/AIMenuControllerProtocol.h>
#import <Adium/AIChatControllerProtocol.h>
#import <Adium/AIService.h>
#import <Adium/AIStatus.h>
#import <Adium/AIStatusControllerProtocol.h>
#import <AdiumLibpurple/SLPurpleCocoaAdapter.h>
#import <AIUtilities/AIAttributedStringAdditions.h>
#import <AIUtilities/AIMutableStringAdditions.h>
#import <AIUtilities/AIStringAdditions.h>
#import <libpurple/cmds.h>

#include "../../src/honprpl.h"

@implementation PurpleHoNAccount

- (BOOL)chatShouldAutocompleteUID:(AIChat *)inChat
{
	return YES;
}

- (BOOL)openChat:(AIChat *)chat
{
	chat.hideUserIconAndStatus = YES;

	return [super openChat:chat];
}

- (void)openInspectorForContactInfo:(AIListContact *)theContact
{
	[[NSNotificationCenter defaultCenter] postNotificationName:@"AIShowContactInfo" object:theContact];
}

#pragma mark Command handling

static PurpleConversation *fakeConversation(PurpleAccount *account)
{
	PurpleConversation *conv;
	
	conv = g_new0(PurpleConversation, 1);
	conv->type = PURPLE_CONV_TYPE_IM;
	conv->account = account;
	
	return conv;
}

- (void)didConnect
{
	[super didConnect];
	
	PurpleConversation *conv = fakeConversation(self.purpleAccount);
	
	for (NSString *command in [[self preferenceForKey:KEY_HON_COMMANDS
												group:GROUP_ACCOUNT_STATUS] componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]])
	{
		if ([command hasPrefix:@"/"])
		{
			command = [command substringFromIndex:1];
		}

		if (command.length)
		{
			char *error;
			PurpleCmdStatus cmdStatus = purple_cmd_do_command(conv, [command UTF8String], [command UTF8String], &error);

			if (cmdStatus == PURPLE_CMD_STATUS_NOT_FOUND)
			{
				NSLog(@"Command (%@) not found", command);
			}
			else if (cmdStatus != PURPLE_CMD_STATUS_OK)
			{
				NSLog(@"Command (%@) failed: %d - %@", command, cmdStatus, [NSString stringWithUTF8String:error]);
			}
		}
	}

	g_free(conv);

	[self setPreference:[[NSAttributedString stringWithString:@"Adium"] dataRepresentation]
				 forKey:KEY_ACCOUNT_DISPLAY_NAME
				  group:GROUP_ACCOUNT_STATUS];
}

#pragma mark libpurple

- (const char *)protocolPlugin
{
	return "prpl-hon";
}

- (void)configurePurpleAccount
{
	[super configurePurpleAccount];

	purple_account_set_bool(self.purpleAccount, IS_MD5_OPTION, FALSE);
}

- (NSString *)displayName
{
	/*
	if (account)
	{
		PurpleConnection *purpleConnection = purple_account_get_connection(account);

		if (purpleConnection)
			return [NSString stringWithUTF8String:purple_connection_get_display_name(purpleConnection)];
	}
	*/

	return self.formattedUID;
}

- (BOOL)shouldSendAutoreplyToMessage:(AIContentMessage *)message
{
	return NO;
}

- (BOOL)canSendOfflineMessageToContact:(AIListContact *)inContact
{
	return NO;
}

- (NSString *)host
{
	return @"masterserver.hon.s2games.com";
}

- (int)port
{
	return 80;
}

- (BOOL)closeChat:(AIChat *)chat
{
	if (adium.isQuitting)
		return NO;
	else
		return [super closeChat:chat];
}

- (BOOL)groupChatsSupportTopic
{
	return YES;
}

- (const char *)purpleStatusIDForStatus:(AIStatus *)statusState
							  arguments:(NSMutableDictionary *)arguments
{
	const char *statusID = NULL;
	NSString *statusMessageString = [statusState statusMessageString];

	if (!statusMessageString)
		statusMessageString = @"";

	switch (statusState.statusType)
	{
		case AIAvailableStatusType:
			statusID = "online";
			break;

		case AIAwayStatusType:
			statusID = "ingame";
			break;

		case AIOfflineStatusType:
			statusID = "offline";
			break;
	}

	if (statusID == NULL)
		statusID = [super purpleStatusIDForStatus:statusState
										arguments:arguments];

	return statusID;
}

@end