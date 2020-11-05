//
//  MetaxRunner.h
//  Metax
//
//  Created by Raffi Knyazyan on 5/30/18.
//  Copyright © 2018 Instigate Mobile. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface MetaxRunner : NSObject
+(void) startMetax;
+(void) stopMetax;
+(void) restartServer;

@end
