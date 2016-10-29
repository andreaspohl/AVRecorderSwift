//
//  MotionWrapper.mm
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 29.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

#import "MotionWrapper.h"
#include "Motion.hpp"
@implementation MotionWrapper
- (void)processVideoWrapped:(NSString *)videoFileName {
    Motion motion;
    motion.processVideo([videoFileName cStringUsingEncoding:NSUTF8StringEncoding]);
}
@end
