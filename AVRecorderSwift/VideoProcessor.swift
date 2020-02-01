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
        
        let fileManager = FileManager()
        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
        
        let inPath : NSString = moviesPath.appendingPathComponent("3_process/") as NSString
        
        while isRunning {
            autoreleasepool {
                let files = try! fileManager.contentsOfDirectory(atPath: inPath as String)
                
                //print("VideoProcessor checking 3_process... ")
                
                for file in files {
                    if file.contains("new.mov") {
                        debugPrint("processing: ", file)
                        
                        let fromPathFileNameExtension = inPath.appendingPathComponent(file)
                        
                        //here comes the action: process the video
                        MotionWrapper().processVideoWrapped(fromPathFileNameExtension)
                        
                        //as processing is done, lets merge the videos
                        let videoLabel : String = file.replacingOccurrences(of: " new.mov", with: "")
                        VideoMerger().merge(videoLabel: videoLabel)
                        
                        
                        //rename input file to "archive", so they get archived by FileHandler
                        let toFile : String = file.replacingOccurrences(of: " new.mov", with: " archive.mov")
                        let toPathFileNameExtension = inPath.appendingPathComponent(toFile)
                        
                        let fromURL : URL = URL(fileURLWithPath: fromPathFileNameExtension as String)
                        let toURL : URL = URL(fileURLWithPath: toPathFileNameExtension as String)
                        
                        do {
                            try fileManager.moveItem(at: fromURL, to: toURL)
                        } catch let moveError as NSError {
                            print(moveError.localizedDescription)
                        }
                    }
                }
            }
            sleep(2)
        }
    }
}
