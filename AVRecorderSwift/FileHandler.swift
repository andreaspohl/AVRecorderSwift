//
//  FileHandler.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 23.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

//handles file moving between the different directories of the application

class FileHandler: NSObject {
    
    func run () {
        
        print("FileHandler entered")
        
        // put everything else in a low prio queue
        for i in 1...10 {
            print(i)
            sleep(2)
        }
    }
}

