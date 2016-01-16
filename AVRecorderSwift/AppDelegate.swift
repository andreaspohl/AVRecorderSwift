//
//  AppDelegate.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.01.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation

import Foundation
import AVFoundation

class AVRecorderDelegate: NSObject, AVCaptureFileOutputRecordingDelegate {
    
    var device: AVCaptureDevice?
    var deviceInput: AVCaptureDeviceInput?
    var movieFileOutput: AVCaptureMovieFileOutput?
    var session: AVCaptureSession?
    
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
        let seconds : Int64 = 13
        let preferredTimeScale : Int32 = 1
        let maxDuration : CMTime = CMTimeMake(seconds, preferredTimeScale)
        movieFileOutput!.maxRecordedDuration = maxDuration
    }
    
    //MARK: Control recording
    func startRecording() {
        
        var filename : NSString = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.AllDomainsMask, true).first!
        
        filename = filename.stringByAppendingPathComponent("workspace/AVRecorderSwift/in/in.mov")
        print("\(filename)")
        
        if (NSFileManager.defaultManager().fileExistsAtPath(filename as String)) {
            print("File exists! Deleting...")
            do {
                try NSFileManager.defaultManager().removeItemAtPath(filename as String)
            } catch {
                print("ERROR: can't remove file \(filename as String)")
                fatalError()
            }
        }
        
        let path : NSURL? = NSURL(fileURLWithPath: filename as String)
        
        session!.startRunning()

        print("start recording")
        movieFileOutput!.startRecordingToOutputFileURL(path!, recordingDelegate: self)
    }
    
    func stopRecording() {
        session!.stopRunning()
        print("stop recording")
        dispatch_async(dispatch_get_main_queue()) {
            exit(1)
        }
        exit(0)
    }
    
    //MARK: Delegate methods
    
    func captureOutput(captureOutput: AVCaptureFileOutput!, didFinishRecordingToOutputFileAtURL outputFileURL: NSURL!, fromConnections connections: [AnyObject]!, error: NSError!) {
        print("finished recording")
        self.stopRecording()
        dispatch_async(dispatch_get_main_queue()) {
            exit(2)
        }
    }
}

