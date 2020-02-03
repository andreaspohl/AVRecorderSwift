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
    
    //set max movie duration, 10 minutes / 600 sec seems ok
    //should not exceed 4GB because of openCV
    //est. 4MB/sec --> max 4000 sec
    let SECONDS : Int64 = 600
    let MAX_MOVIES = 9 // one movie is 10 minutes long, makes the max session duration 90 minutes

    var device: AVCaptureDevice?
    var deviceInput: AVCaptureDeviceInput?
    var movieFileOutput: AVCaptureMovieFileOutput?
    var session: AVCaptureSession?
    
    var recordingUrl: URL? //here we keep the actual file URL where we are recording
    
    var movieNumber = 0 // the actual movie count
    
    var fileManager = FileManager()
    
    //MARK: Initialization
    override init() {
        
        super.init()
        
        //initialize session
        session = AVCaptureSession.init()
        session!.sessionPreset = AVCaptureSession.Preset.high
        
        //look for device
        let discoverySession = AVCaptureDevice.DiscoverySession(deviceTypes: [.externalUnknown], mediaType: .video, position: .unspecified)
        
        for dev in discoverySession.devices {
            print(dev.localizedName)
        }
        
        var takeDefault = true
        for dev in discoverySession.devices {
            if (dev.localizedName.contains("C920")) {
                device = dev
                takeDefault = false
                break
            }
        }
        
        if takeDefault {
            device = AVCaptureDevice.default(for: AVMediaType.video)
        }
        
        let deviceName = device?.localizedName
        print("DeviceName: \(String(describing: deviceName))")
        
        //lock focus (hope that it was set to infinity...)
        do {
            try device!.lockForConfiguration()
        }
        catch let error as NSError {
            NSLog("\(error), \(error.localizedDescription)")
        }
        
        if (device!.isFocusModeSupported(AVCaptureDevice.FocusMode.locked)) {
            print("setting manual focus")
            device?.focusMode = AVCaptureDevice.FocusMode.locked
        }
        
        if (device!.isExposureModeSupported(AVCaptureDevice.ExposureMode.continuousAutoExposure)) {
            print("setting continuous auto exposure")
            device?.exposureMode = AVCaptureDevice.ExposureMode.continuousAutoExposure
        }
        
        device!.unlockForConfiguration()
        
        //set captureDevice
        var error: NSError? = nil
        do {
            deviceInput = try AVCaptureDeviceInput(device: device!)
            print("capture device set")
        } catch let error1 as NSError {
            error = error1
            deviceInput = nil
            print("ERROR: could not set capture device \(String(describing: error))")
        } catch {
            fatalError()
        }
        
        //add input to session
        if session!.canAddInput(deviceInput!) {
            session!.addInput(deviceInput!)
            print("input added to session")
        }
        
        //add output to session and connect to input
        movieFileOutput = AVCaptureMovieFileOutput()
        if session!.canAddOutput(movieFileOutput!) {
            session!.addOutput(movieFileOutput!)
            print("output added to session")
        }
        
        let seconds = SECONDS
        let preferredTimeScale : Int32 = 1
        let maxDuration : CMTime = CMTimeMake(value: seconds, timescale: preferredTimeScale)
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
        
        do {
            try fileManager.moveItem(at: fromURL!, to: toURL)
        } catch let moveError as NSError {
            print(moveError.localizedDescription)
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
        movieFileOutput!.startRecording(to: newUrl!, recordingDelegate: self)
        
        //renaming will be done by fileOutput didFinishRecordingTo
        
        //remember the new file name and url
        recordingUrl = newUrl!

    }
    
    func stopRecording() {
        print("stop recording")
        movieNumber = 0
        movieFileOutput!.stopRecording()
        recordingUrl = nil
        
        session!.stopRunning()
    }
    
    //MARK: Delegate methods
    
    func fileOutput(_ output: AVCaptureFileOutput, didFinishRecordingTo outputFileURL: URL, from connections: [AVCaptureConnection], error: Error?) {
        print("finished recording", outputFileURL.lastPathComponent)
        if (error.debugDescription.contains("Code=-11810")) {
            // has been finished because movie file has reached intended size --> restart
            startRecording()
        }
        renameFileDone(outputFileURL)
    }
    
}
