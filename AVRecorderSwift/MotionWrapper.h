//
//  MotionWrapper.h
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 29.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

#pragma once

#import <Foundation/Foundation.h>
@interface MotionWrapper : NSObject
- (void)processVideoWrapped:(NSString *)videoFileName;
- (void)processVideoDebug:(NSString *)videoFileName;
@end
