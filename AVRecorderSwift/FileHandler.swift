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
        
        
        let inPath : NSString = moviesPath.appendingPathComponent("1_in/") as NSString
        let stitchedPath : NSString = moviesPath.appendingPathComponent("2_stitch/") as NSString
        let procPath : NSString = moviesPath.appendingPathComponent("3_process/") as NSString
        
        
        while isRunning {

            //move from 1_in to 2_stitch
            var files = try! fileManager.contentsOfDirectory(atPath: inPath as String)
            
            print("\nFileHandler checking 1_in... ")
            
            for file in files {
                if file.contains("done.mov") {
                    debugPrint("moving file from 1_in to 2_stitch: ", file)
                    
                    let toFile : String = file.replacingOccurrences(of: " done.mov", with: " new.mov")
                    
                    let fromPathFileNameExtension = inPath.appendingPathComponent(file)
                    
                    let toPathFileNameExtension = stitchedPath.appendingPathComponent(toFile)
                    
                    let fromURL : URL = URL(fileURLWithPath: fromPathFileNameExtension as String)
                    let toURL : URL = URL(fileURLWithPath: toPathFileNameExtension as String)
                    
                    do {
                        try fileManager.moveItem(at: fromURL, to: toURL)
                    } catch let moveError as NSError {
                        print(moveError.localizedDescription)
                    }
                }
            }
            
            //move from 2_stitch to 3_process
            files = try! fileManager.contentsOfDirectory(atPath: stitchedPath as String)
            
            print("\nFileHandler checking 2_stitch... ")
            
            for file in files {
                if file.contains("done.mov") {
                    debugPrint("moving file from 2_stitch to 3_process: ", file)
                    
                    let toFile : String = file.replacingOccurrences(of: " done.mov", with: " new.mov")
                    
                    let fromPathFileNameExtension = stitchedPath.appendingPathComponent(file)
                    
                    let toPathFileNameExtension = procPath.appendingPathComponent(toFile)
                    
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

