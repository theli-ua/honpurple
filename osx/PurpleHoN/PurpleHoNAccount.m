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
#import "SLPurpleCocoaAdapter.h"

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

#pragma mark libpurple

- (const char *)protocolPlugin
{
	return "prpl-hon";
}

- (void)configurePurpleAccount
{
	[super configurePurpleAccount];

	purple_account_set_bool(self.purpleAccount, IS_MD5_OPTION, FALSE);

//	[adium.chatController registerChatObserver:self];
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