//
//  PurpleHoNService.m
//  PurpleHoN
//
//  Created by Dopefish on 2010-10-10.
//  Copyright 2010 David Rudie. All rights reserved.
//

#import <Adium/AIPlugin.h>
#import <Adium/AIStatusControllerProtocol.h>
#import <AdiumLibpurple/PurpleAccountViewController.h>

#import "PurpleHoNService.h"
#import "PurpleHoNAccount.h"

#include "../../src/honprpl.h"

@interface PurpleHoNAccountViewController : PurpleAccountViewController {}
@end

@implementation PurpleHoNAccountViewController

- (NSString *)nibName
{
	return @"PurpleHoNAccountView";
}

@end

#import <Adium/AIStatusControllerProtocol.h>
#import <AIUtilities/AIImageAdditions.h>

@implementation PurpleHoNService

- (Class)accountClass
{
	return [PurpleHoNAccount class];
}

- (AIAccountViewController *)accountViewController
{
	return [PurpleHoNAccountViewController accountViewController];
}

/*
- (DCJoinChatViewController *)joinChatView
{
	return [DCPurpleHoNJoinChatViewController joinChatView];
}
 */

- (NSString *)serviceCodeUniqueID
{
	return @"prpl-hon";
}

- (NSString *)serviceID
{
	return @"Heroes of Newerth";
}

- (NSString *)serviceClass
{
	return @"Heroes of Newerth";
}

- (NSString *)shortDescription
{
	return @"Heroes of Newerth";
}

- (NSString *)longDescription
{
	return @"Heroes of Newerth";
}

- (NSCharacterSet *)allowedCharacters
{
	return [NSCharacterSet characterSetWithCharactersInString:@"abcdefghijklmnopqrstuvwxyz_`"];
}

- (NSUInteger)allowedLength
{
	return 13;
}

- (BOOL)caseSensitive
{
	return NO;
}

- (BOOL)canCreateGroupChats
{
	return YES;
}

- (AIServiceImportance)serviceImportance
{
	return AIServiceSecondary;
}

- (NSString *)userNameLabel
{
	return @"Username";
}

- (void)registerStatuses
{
	purple_debug_info("hon", "setting status types\n");
	[adium.statusController registerStatus:STATUS_NAME_AVAILABLE
						   withDescription:[adium.statusController localizedDescriptionForCoreStatusName:STATUS_NAME_AVAILABLE]
									ofType:AIAvailableStatusType
								forService:self];
	
	[adium.statusController registerStatus:STATUS_NAME_AWAY
						   withDescription:[adium.statusController localizedDescriptionForCoreStatusName:STATUS_NAME_AWAY]
									ofType:AIAwayStatusType
								forService:self];

	[adium.statusController registerStatus:STATUS_NAME_OFFLINE
						   withDescription:[adium.statusController localizedDescriptionForCoreStatusName:STATUS_NAME_OFFLINE]
									ofType:AIOfflineStatusType
								forService:self];
}

- (NSImage *)defaultServiceIconOfType:(AIServiceIconType)iconType
{
	if ((iconType == AIServiceIconSmall) || (iconType == AIServiceIconList))
	{
		return [NSImage imageNamed:@"hon-small" forClass:[self class] loadLazily:YES];
	}
	else
	{
		return [NSImage imageNamed:@"hon" forClass:[self class] loadLazily:YES];
	}
}

- (NSString *)pathForDefaultServiceIconOfType:(AIServiceIconType)iconType
{
	return nil;
}

@end