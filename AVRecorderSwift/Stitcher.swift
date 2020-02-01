//
//  Stitcher.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 27.10.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

// stitches movies from several cams to one single movie

class Stitcher: NSObject {
    
    var isRunning = true
    
    func run () {
        
        print("Stitcher entered")
        
        let fileManager = FileManager()
        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
        
        let inPath : NSString = moviesPath.appendingPathComponent("2_stitch/") as NSString
        
        while isRunning {
            autoreleasepool {
                let files = try! fileManager.contentsOfDirectory(atPath: inPath as String)
                
                //print("Stitcher checking 2_stitch... ")
                
                for file in files {
                    if file.contains("new.mov") {
                        debugPrint("stitching: ", file)
                        
                        //TODO: implement stitching (replace below dummy code)
                        
                        let toFile : String = file.replacingOccurrences(of: " new.mov", with: " done.mov")
                        
                        let fromPathFileNameExtension = inPath.appendingPathComponent(file)
                        
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
            sleep(10)
        }
    }
}

