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
    
    var isRunning = true
    
    func run () {
        
        print("FileHandler entered")
        
        let fileManager = FileManager()
        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
        
        let fromPath : NSString = moviesPath.appendingPathComponent("1_in/") as NSString
        let toPath : NSString = moviesPath.appendingPathComponent("2_stitched/") as NSString
        
        while isRunning {
            let files = try! fileManager.contentsOfDirectory(atPath: fromPath as String)
            
            print("\nchecking 1_in... ")
            
            for file in files {
                if file.contains("done.mov") {
                    debugPrint("moving file from 1_in to 2_stitched: ", file)
                    
                    let toFile : String = file.replacingOccurrences(of: " done", with: "")
                    
                    let fromPathFileNameExtension = fromPath.appendingPathComponent(file)

                    let toPathFileNameExtension = toPath.appendingPathComponent(toFile)
                    
                    let fromURL : URL = URL(fileURLWithPath: fromPathFileNameExtension as String)
                    let toURL : URL = URL(fileURLWithPath: toPathFileNameExtension as String)
                    
                    do {
                        try fileManager.moveItem(at: fromURL, to: toURL)
                    } catch let moveError as NSError {
                        print(moveError.localizedDescription)
                    }
                }
            }
            
            sleep(10)
        }
    }
}

