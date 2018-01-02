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
    
    var recordingUrl: URL? //here we keep the actual file URL where we are recording
    
    let MAX_MOVIES = 9 // one movie is 10 minutes long, makes the max session duration 90 minutes
    var movieNumber = 0 // the actual movie count
    
    var fileManager = FileManager()
    
    //MARK: Initialization
    override init() {
        
        super.init()
        
        //initialize session
        session = AVCaptureSession.init()
        session!.sessionPreset = AVCaptureSessionPresetHigh
        
        //look for device
        device = AVCaptureDevice.defaultDevice(withMediaType: AVMediaTypeVideo)
        let deviceName = device?.localizedName
        print("DeviceName: \(deviceName)")
        
        //lock focus (hope that it was set to infinity...)
        do {
            try device!.lockForConfiguration()
        }
        catch let error as NSError {
            NSLog("\(error), \(error.localizedDescription)")
        }
        
        if (device!.isFocusModeSupported(AVCaptureFocusMode.locked)) {
            print("setting manual focus")
            device?.focusMode = AVCaptureFocusMode.locked
        }
        
        device!.unlockForConfiguration()
        
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
        
        //set max movie duration, 10 minutes seems ok
        //should not exceed 4GB because of openCV
        //est. 4MB/sec --> max 4000 sec
        let seconds : Int64 = 600 //600
        let preferredTimeScale : Int32 = 1
        let maxDuration : CMTime = CMTimeMake(seconds, preferredTimeScale)
        movieFileOutput!.maxRecordedDuration = maxDuration
    }
    
    //MARK: Utility
    func renameFileDone (_ fromURL: URL?) {
        
        //renames the file fromURL by adding 'done' to the file name
        
        if fromURL == nil {
            return
        }
        
        let path = fromURL!.deletingLastPathComponent()
        let fileNameWithExtension : NSString = fromURL!.lastPathComponent as NSString
        let fileName = fileNameWithExtension.deletingPathExtension
        let fileExtension = fileNameWithExtension.pathExtension
        
        let donePathAndFileNameWithExtension = path.appendingPathComponent("\(fileName) done.\(fileExtension)")
        
        let toURL = donePathAndFileNameWithExtension
        
        if recordingUrl != nil {
            do {
                try fileManager.moveItem(at: fromURL!, to: toURL)
            } catch let moveError as NSError {
                print(moveError.localizedDescription)
            }
        }
        
    }
    
    //MARK: Control recording
    func startRecording() {
        
        //check if max movie count is reached
        movieNumber = movieNumber + 1
        if (movieNumber > MAX_MOVIES) {
            stopRecording()
            return
        }
        
        //start the session if not already running
        if  !session!.isRunning {
            session!.startRunning()
        }
        
        //get path to movies directory
        let moviesPath : NSString = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
        
        let path : NSString = moviesPath.appendingPathComponent("1_in/") as NSString
        
        //calculate timestamp
        let now = Date()
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd' 'HH'-'mm'-'ss"
        formatter.timeZone = TimeZone(secondsFromGMT: 0)
        let timestamp = formatter.string(from: now)
        
        
        // make filename
        let fileName = "\(timestamp) CAM0"
        let filePathNameExtension = path.appendingPathComponent("\(fileName).mov")
        let newUrl : URL? = URL(fileURLWithPath: filePathNameExtension as String)

        print("recording to file: \(filePathNameExtension)")
        movieFileOutput!.startRecording(toOutputFileURL: newUrl!, recordingDelegate: self)
        
        //now rename the old file
        renameFileDone(recordingUrl)
        
        //remember the new file name and url
        recordingUrl = newUrl!

    }
    
    func stopRecording() {
        print("stop recording")
        movieNumber = 0
        movieFileOutput!.stopRecording()
        renameFileDone(recordingUrl)
        recordingUrl = nil
        
        session!.stopRunning()
    }
    
    //MARK: Delegate methods
    
    func capture(_ captureOutput: AVCaptureFileOutput!, didFinishRecordingToOutputFileAt outputFileURL: URL!,
                 fromConnections connections: [Any]!, error: Error!) {
        if error == nil {
            print("finished writing without error")
        } else {
            if error._code == -11810 {
                print("max file length reached, restarting recording")
                startRecording()
            } else {
                print("captureOuput called with ", error.localizedDescription)
            }
        }
    }
}

