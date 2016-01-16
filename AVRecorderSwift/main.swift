//
//  main.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.01.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

let appDelegate = AVRecorderDelegate()

appDelegate.startRecording()

let seconds = 15.0
let delay = seconds * Double(NSEC_PER_SEC)  // nanoseconds per seconds
let dispatchTime = dispatch_time(DISPATCH_TIME_NOW, Int64(delay))

dispatch_after(dispatchTime, dispatch_get_main_queue()) {
    appDelegate.stopRecording()
}

let waitQ = dispatch_queue_create("waitForFinishQueue", DISPATCH_QUEUE_SERIAL)

dispatch_after(dispatchTime, waitQ) {
    appDelegate.stopRecording()
}

dispatch_main()