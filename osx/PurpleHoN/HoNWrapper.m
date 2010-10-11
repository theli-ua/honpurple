//
//  HoNWrapper.m
//  PurpleHoN
//
//  Created by Dopefish on 2010-10-10.
//  Copyright 2010 David Rudie. All rights reserved.
//

#import "HoNWrapper.h"
#import "PurpleHoNService.h"
#import "honprpl.h"

@implementation HoNWrapper

- (void)installLibpurplePlugin
{
	NSLog(@"HON installLibpurplePlugin");
}

- (void)loadLibpurplePlugin
{
	NSLog(@"HON loadLibpurplePlugin");
}

- (void)installPlugin
{
	NSLog(@"HON installPlugin Begin");
	[PurpleHoNService registerService];
	purple_init_honprpl_plugin();
	NSLog(@"HON installPlugin End");
}

@end