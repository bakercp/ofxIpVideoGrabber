//
// Copyright (c) 2020 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier: MIT
//


#include "ofApp.h"


void ofApp::setup()
{
    ofSetFrameRate(30);

    // Set the camera URL.
    // Many axis cameras allow video resolution, fps, etc to be manipulated with url arguments.
    // N.B. If you don't see an image, the camera may be offline or no longer in service and
    // this link may need to be replaced with a valid link.
    grabber.setURI("http://107.1.228.34/axis-cgi/mjpg/video.cgi?resolution=320x240");

    // Connect to the stream.
    grabber.connect();
}


void ofApp::update()
{
    grabber.update();
    
    if (grabber.isFrameNew())
    {
        cameraPix = grabber.getPixels();
        
        // Use or modify pixels in some way, e.g. invert the colors.
        for (std::size_t x = 0; x < cameraPix.getWidth(); x++)
        {
            for (std::size_t y = 0; y < cameraPix.getHeight(); y++)
            {
                cameraPix.setColor(x, y, cameraPix.getColor(x, y).getInverted());
            }
        }
        
        // Load the texture.
        cameraTex.loadData(cameraPix);
    }
}


void ofApp::draw()
{
    ofSetColor(255);
    
    // Draw the camera.
    grabber.draw(0, 0, cameraWidth, cameraHeight);
    
    // Draw the modified pixels if they are available.
    if (cameraTex.isAllocated())
    {
        cameraTex.draw(cameraWidth, 0, cameraWidth, cameraHeight);
    }
    
    std::stringstream ss;

    // Show connection statistics if desired.
    if (showStats)
    {
        // Metadata about the connection state if needed.
        float kbps = grabber.getBitRate() / 1000.0f; // kilobits / second, not kibibits / second
        float fps = grabber.getFrameRate();
        
        ss << "          NAME: " << grabber.getCameraName() << std::endl;
        ss << "          HOST: " << grabber.getHost() << std::endl;
        ss << "           FPS: " << ofToString(fps,  2, 13, ' ') << std::endl;
        ss << "          Kb/S: " << ofToString(kbps, 2, 13, ' ') << std::endl;
        ss << " #Bytes Recv'd: " << ofToString(grabber.getNumBytesReceived(),  0, 10, ' ') << std::endl;
        ss << "#Frames Recv'd: " << ofToString(grabber.getNumFramesReceived(), 0, 10, ' ') << std::endl;
        ss << "Auto Reconnect: " << (grabber.getAutoReconnect() ? "YES" : "NO") << std::endl;
        ss << " Needs Connect: " << (grabber.getNeedsReconnect() ? "YES" : "NO") << std::endl;
        ss << "Time Till Next: " << grabber.getTimeTillNextAutoRetry() << " ms" << std::endl;
        ss << "Num Reconnects: " << ofToString(grabber.getReconnectCount()) << std::endl;
        ss << "Max Reconnects: " << ofToString(grabber.getMaxReconnects()) << std::endl;
        ss << "  Connect Fail: " << (grabber.hasConnectionFailed() ? "YES" : "NO");
    }
    else
    {
        ss << "Press any key to show connection stats.";
    }
    

    ofSetColor(255);
    ofDrawBitmapStringHighlight(ss.str(), 10, 10+12, ofColor(0, 80));
}


void ofApp::keyPressed(int key)
{
    showStats = !showStats;
}
