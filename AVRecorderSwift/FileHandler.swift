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
        
        print("FileHandler started...")
        
        let fileManager = FileManager()
        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
        
        
        let inPath : NSString = moviesPath.appendingPathComponent("1_in/") as NSString
        let stitchedPath : NSString = moviesPath.appendingPathComponent("2_stitch/") as NSString
        let procPath : NSString = moviesPath.appendingPathComponent("3_process/") as NSString
        let transPath : NSString = moviesPath.appendingPathComponent("4_transfer/") as NSString
        let archPath : NSString = moviesPath.appendingPathComponent("5_archive/") as NSString
        
        
        while isRunning {

            //move from 1_in to 2_stitch
            var files = try! fileManager.contentsOfDirectory(atPath: inPath as String)
            
            print("FileHandler checking 1_in... ")
            
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
            
            print("FileHandler checking 2_stitch... ")
            
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
            
            //move from 3_process to 4_transfer
            files = try! fileManager.contentsOfDirectory(atPath: procPath as String)
            
            print("FileHandler checking 3_process... ")
            
            for file in files {
                if file.contains("done.mov") {
                    debugPrint("moving file from 3_process to 4_transfer: ", file)
                    
                    let toFile : String = file.replacingOccurrences(of: " done.mov", with: " new.mov")
                    
                    let fromPathFileNameExtension = procPath.appendingPathComponent(file)
                    
                    let toPathFileNameExtension = transPath.appendingPathComponent(toFile)
                    
                    let fromURL : URL = URL(fileURLWithPath: fromPathFileNameExtension as String)
                    let toURL : URL = URL(fileURLWithPath: toPathFileNameExtension as String)
                    
                    do {
                        try fileManager.moveItem(at: fromURL, to: toURL)
                    } catch let moveError as NSError {
                        print(moveError.localizedDescription)
                    }
                } else if file.contains("archive.mov") {
                    debugPrint("moving file from 3_process to 5_archive: ", file)
                    
                    let fromPathFileNameExtension = procPath.appendingPathComponent(file)
                    
                    let toPathFileNameExtension = archPath.appendingPathComponent(file)
                    
                    let fromURL : URL = URL(fileURLWithPath: fromPathFileNameExtension as String)
                    let toURL : URL = URL(fileURLWithPath: toPathFileNameExtension as String)
                    
                    do {
                        try fileManager.moveItem(at: fromURL, to: toURL)
                    } catch let moveError as NSError {
                        print(moveError.localizedDescription)
                    }
                }
            }
            
            //move from 4_transfer to 5_archive
            files = try! fileManager.contentsOfDirectory(atPath: transPath as String)
            
            print("FileHandler checking 4_transfer... ")
            
            for file in files {
                if file.contains("done.mov") {
                    debugPrint("moving file from 4_transfer to 5_archive: ", file)
                    
                    let toFile : String = file.replacingOccurrences(of: " done.mov", with: " transferred.mov")
                    
                    let fromPathFileNameExtension = transPath.appendingPathComponent(file)
                    
                    let toPathFileNameExtension = archPath.appendingPathComponent(toFile)
                    
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

