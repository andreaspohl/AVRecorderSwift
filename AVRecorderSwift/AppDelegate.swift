//
//  AppDelegate.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.01.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation
import AVFoundation

class AVRecorderDelegate: NSObject, AVCaptureFileOutputRecordingDelegate {
    
    var device: AVCaptureDevice?
    var deviceInput: AVCaptureDeviceInput?
    var movieFileOutput: AVCaptureMovieFileOutput?
    var session: AVCaptureSession?
    
    var recordingUrl: NSURL? //here we keep the actual file URL where we are recording
    
    var fileManager = NSFileManager()
    
    //MARK: Initialization
    override init() {
        
        super.init()
        
        //initialize session
        session = AVCaptureSession.init()
        session!.sessionPreset = AVCaptureSessionPresetHigh
        
        //look for device
        device = AVCaptureDevice.defaultDeviceWithMediaType(AVMediaTypeVideo)
        let deviceName = device?.localizedName
        print("DeviceName: \(deviceName)")
        
        //set captureDevice
        var error: NSError? = nil
        do {
            deviceInput = try AVCaptureDeviceInput(device: device)
            print("capture device set")
        } catch let error1 as NSError {
            error = error1
            deviceInput = nil
            print("ERROR: could not set capture device \(error)")
        } catch {
            fatalError()
        }
        
        //add input to session
        if session!.canAddInput(deviceInput) {
            session!.addInput(deviceInput)
            print("input added to session")
        }
        
        //add output to session and connect to input
        movieFileOutput = AVCaptureMovieFileOutput()
        if session!.canAddOutput(movieFileOutput) {
            session!.addOutput(movieFileOutput)
            print("output added to session")
        }
        
        //set max movie duration
//        let seconds : Int64 = 700
        let seconds : Int64 = 5
        let preferredTimeScale : Int32 = 1
        let maxDuration : CMTime = CMTimeMake(seconds, preferredTimeScale)
        movieFileOutput!.maxRecordedDuration = maxDuration
    }
    
    //MARK: Utility
    func renameFileDone (fromURL: NSURL?) {
        
        //renames the file fromURL by adding 'done' to the file name
        
        if fromURL == nil {
            return
        }
        
        let path = fromURL!.URLByDeletingLastPathComponent
        let fileNameWithExtension : NSString = fromURL!.lastPathComponent!
        let fileName = fileNameWithExtension.stringByDeletingPathExtension
        let fileExtension = fileNameWithExtension.pathExtension
        
        let donePathAndFileNameWithExtension = path?.URLByAppendingPathComponent("\(fileName) done.\(fileExtension)")
                
        let toURL = donePathAndFileNameWithExtension
        
        if recordingUrl != nil {
            do {
                try fileManager.moveItemAtURL(fromURL!, toURL: toURL!)
            } catch let moveError as NSError {
                print(moveError.localizedDescription)
            }
        }
    
    }
    
    //MARK: Control recording
    func startRecording() {
        
        //start the session if not already running
        if  !session!.running {
            session!.startRunning()
        }

        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.MoviesDirectory, NSSearchPathDomainMask.AllDomainsMask, true).first!
        
        let path : NSString = moviesPath.stringByAppendingPathComponent("1_in/")
        
        //calculate timestamp
        let now = NSDate()
        let formatter = NSDateFormatter()
        formatter.dateFormat = "yyyy-MM-dd' 'HH':'mm':'ss"
        formatter.timeZone = NSTimeZone(forSecondsFromGMT: 0)
        let timestamp = formatter.stringFromDate(now)
        
        
        // make filename
        let fileName = "\(timestamp) CAM0"
        let filePathNameExtension = path.stringByAppendingPathComponent("\(fileName).mov")
        let newUrl : NSURL? = NSURL(fileURLWithPath: filePathNameExtension as String)

        print("recording to file: \(filePathNameExtension)")
        movieFileOutput!.startRecordingToOutputFileURL(newUrl!, recordingDelegate: self)
        
        //now rename the old file
        renameFileDone(recordingUrl)
        
        //remember the new file name and url
        recordingUrl = newUrl!

    }
    
    func stopRecording() {
        print("stop recording")
        movieFileOutput!.stopRecording()
        renameFileDone(recordingUrl)
        recordingUrl = nil
        
        session!.stopRunning()
    }
    
    //MARK: Delegate methods
    
    func captureOutput(captureOutput: AVCaptureFileOutput!, didFinishRecordingToOutputFileAtURL outputFileURL: NSURL!, fromConnections connections: [AnyObject]!, error: NSError!) {
        if error == nil {
            print("finished writing without error")
        } else {
            //print(error!.code)
            //print(error!.userInfo)
            if error!.code == -11810 {
                print("max file length reached, restarting recording")
                startRecording()
            } else {
                print("captureOuput called with", error!.userInfo)
            }
        }
    }
}

