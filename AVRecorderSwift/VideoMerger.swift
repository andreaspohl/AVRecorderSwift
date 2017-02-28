//
//  VideoMerger.swift
//  AVRecorderSwift
//
//  Created by Andreas Pohl on 16.11.16.
//  Copyright © 2016 Andreas Pohl. All rights reserved.
//

import Foundation
import AVKit
import AVFoundation

//merges the results of the video processing to one single video
//renames the input files from " processing.mov" to " .mov" and puts result into " done.mov"

class VideoMerger: NSObject {
    
    let moviesPath = NSSearchPathForDirectoriesInDomains(FileManager.SearchPathDirectory.moviesDirectory, FileManager.SearchPathDomainMask.allDomainsMask, true).first! as NSString
    let fileManager = FileManager()
    
    private func cleanUpVideos(videoLabel : String) {
        
        debugPrint("cleaning up files containing: ", videoLabel)
        
        let processPath = moviesPath.appendingPathComponent("3_process/") as NSString
        let files = try! fileManager.contentsOfDirectory(atPath: processPath as String)
        
        for file in files {
            var outFile = ""
            if file.contains(videoLabel) {
                if file.contains(" processing.mov") {
                    outFile = file.replacingOccurrences(of: " processing.mov", with: " delete.mov")
                } else if file.contains(" merging.mov") {
                    outFile = file.replacingOccurrences(of: " merging.mov", with: " done.mov")
                }
                
                if outFile != "" {
                    let fromURL = URL(fileURLWithPath: processPath.appendingPathComponent(file))
                    let toURL = URL(fileURLWithPath: processPath.appendingPathComponent(outFile))
                    try? fileManager.moveItem(at: fromURL, to: toURL)
                } else {
                    debugPrint("something went wrong with file ", file)
                }
            }
        }
    }
    
    public func merge(videoLabel : String) {
        
        
        //get all input files in 3_process
        let processPath = moviesPath.appendingPathComponent("3_process/") as NSString
        let files = try! fileManager.contentsOfDirectory(atPath: processPath as String)
        
        //create the composition which will hold all the input tracks
        let composition = AVMutableComposition()
        let compositionTrack = composition.addMutableTrack(withMediaType: AVMediaTypeVideo, preferredTrackID: kCMPersistentTrackID_Invalid)
        //TODO: audio track, add it from the unprocessed track
        
        var totalDuration : CMTime = kCMTimeZero
        
        for file in files {
            //do this only for the files named according to videoLabel and "processing"
            if (file.contains(videoLabel) && file.contains("processing")) {
                debugPrint("getting file for merging: ", file)
                
                let inURL : URL = URL(fileURLWithPath: processPath.appendingPathComponent(file) as String)
                let asset : AVAsset = AVAsset.init(url: inURL)
                let videoTrack : AVAssetTrack = asset.tracks(withMediaType: AVMediaTypeVideo).last!
                let trackDuration : CMTime = videoTrack.timeRange.duration
                try! compositionTrack.insertTimeRange(CMTimeRangeMake(kCMTimeZero, trackDuration), of: videoTrack, at: totalDuration)
                totalDuration = CMTimeAdd(totalDuration, trackDuration)
            }
        }
        
        // AVAssetExportPresetMediumQuality AVAssetExportPresetHighestQuality
        let exporter : AVAssetExportSession = AVAssetExportSession.init(asset: composition, presetName: AVAssetExportPresetMediumQuality)!
        
        //generate output file name
        let outPathURL = URL(fileURLWithPath: processPath.appendingPathComponent(videoLabel + " merging.mov"))
        
        //check if output file exists, if yes, delete first
        if fileManager.fileExists(atPath: String(describing: outPathURL)) {
            try? fileManager.removeItem(at: outPathURL)
        }
        
        exporter.outputURL = outPathURL
        //exporter.outputFileType = AVFileTypeQuickTimeMovie
        exporter.outputFileType = AVFileTypeMPEG4
        exporter.timeRange = CMTimeRange(start: kCMTimeZero, duration: totalDuration)
        exporter.metadata = nil
        exporter.exportAsynchronously(completionHandler: {
            DispatchQueue.main.async {
                switch exporter.status {
                case AVAssetExportSessionStatus.cancelled :
                    debugPrint("cancelled")
                case AVAssetExportSessionStatus.completed :
                    debugPrint("completed")
                    DispatchQueue.global(qos: DispatchQoS.QoSClass.utility).async {
                        self.cleanUpVideos(videoLabel: videoLabel)
                    }
                case AVAssetExportSessionStatus.exporting :
                    debugPrint("exporting")
                case AVAssetExportSessionStatus.failed :
                    debugPrint("failed")
                    debugPrint(exporter.error!)
                case AVAssetExportSessionStatus.unknown :
                    debugPrint("unknown")
                case AVAssetExportSessionStatus.waiting :
                    debugPrint("waiting")
                }
            }
        })
        
        debugPrint("exporting")
        
    }
}
