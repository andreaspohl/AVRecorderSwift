//
//  main.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.01.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

let appDelegate = AVRecorderDelegate()

for _ in 1...4 {
    appDelegate.startRecording()
    sleep(4)
}

appDelegate.stopRecording()

