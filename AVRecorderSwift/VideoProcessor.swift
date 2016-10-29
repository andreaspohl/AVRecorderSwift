//
//  VideoProcessor.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 29.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

//analyses videos for motion and makes a new video showing the interesting part

class VideoProcessor: NSObject {

    var isRunning = true
    
    func run () {
        print("VideoProcessor started...")
        let str = "********it works***********\n"
        MotionWrapper().processVideoWrapped(str)
    }
}
