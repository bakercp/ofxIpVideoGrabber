// =============================================================================
//
// Copyright (c) 2009-2016 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "ofApp.h"


void ofApp::setup()
{
//    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetFrameRate(30);

    ipcams = ofx::Video::IpVideoGrabberSettings::fromFile("streams.json");

    ofLogNotice("ofApp::setup()") << "Loaded " << ipcams.size() << " locations.";

    reloadCameras();

}


ofx::Video::IpVideoGrabberSettings& ofApp::getNextCamera()
{
    nextCamera = (nextCamera + 1) % ipcams.size();
    return ipcams[nextCamera];
}


void ofApp::reloadCameras()
{
    // Clear the grabbers.
    grabbers.clear();

    // Initialize new grabbers.
    for (std::size_t i = 0; i < numCameras; ++i)
    {
        auto c = std::make_shared<ofx::Video::IPVideoGrabber>();
        auto& settings = getNextCamera();
        c->setUsername(settings.getUsername());
        c->setPassword(settings.getPassword());
        c->setURI(settings.getURL());
        c->connect();

        grabbers.push_back(c);;
    }
}


void ofApp::videoResized(const void* sender, ofResizeEventArgs& arg)
{
    // Find the camera that sent the resize event changed.
    for (std::size_t i = 0; i < numCameras; ++i)
    {
        if (sender == grabbers[i].get())
        {
            std::stringstream ss;
            ss << "videoResized: ";
            ss << "Camera connected to: " << grabbers[i]->getURI() + " ";
            ss << "New DIM = " << arg.width << "/" << arg.height;
            ofLogVerbose("ofApp") << ss.str();
        }
    }
}


void ofApp::update()
{
    for (auto& grabber: grabbers)
    {
        grabber->update();
    }
}


void ofApp::draw()
{
    ofBackground(0,0,0);

    ofSetHexColor(0xffffff);
    
    std::size_t row = 0;
    std::size_t col = 0;
    
    float x = 0;
    float y = 0;
    
    float w = ofGetWidth() / numCameraColumns;
    float h = ofGetHeight() / numCameraRows;
    
    float totalKbps = 0;
    float totalFPS = 0;
    
    for (std::size_t i = 0; i < grabbers.size(); ++i)
    {
        x = col * w;
        y = row * h;

        // draw in a grid
        row = (row + 1) % numCameraRows;

        if (row == 0)
        {
            col = (col + 1) % numCameraColumns;
        }
        
        ofPushMatrix();
        ofTranslate(x,y);
        ofSetColor(255,255,255,255);
        grabbers[i]->draw(0,0,w,h); // draw the camera
        
        ofEnableAlphaBlending();
        
        // draw the info box
        //ofSetColor(0,80);
        //ofDrawRectangle(5,5,w-10,h-10);
        
        float kbps = grabbers[i]->getBitRate() / 1000.0f; // kilobits / second, not kibibits / second
        totalKbps+=kbps;
        
        float fps = grabbers[i]->getFrameRate();
        totalFPS+=fps;
        
        std::stringstream ss;
        
        // ofToString formatting available in 0072+
        ss << "          NAME: " << grabbers[i]->getCameraName() << std::endl;
        ss << "          HOST: " << grabbers[i]->getHost() << std::endl;
        ss << "           FPS: " << ofToString(fps,  2/*,13,' '*/) << std::endl;
        ss << "          Kb/S: " << ofToString(kbps, 2/*,13,' '*/) << std::endl;
        ss << " #Bytes Recv'd: " << ofToString(grabbers[i]->getNumBytesReceived(),  0/*,10,' '*/) << std::endl;
        ss << "#Frames Recv'd: " << ofToString(grabbers[i]->getNumFramesReceived(), 0/*,10,' '*/) << std::endl;
        ss << "Auto Reconnect: " << (grabbers[i]->getAutoReconnect() ? "YES" : "NO") << std::endl;
        ss << " Needs Connect: " << (grabbers[i]->getNeedsReconnect() ? "YES" : "NO") << std::endl;
        ss << "Time Till Next: " << grabbers[i]->getTimeTillNextAutoRetry() << " ms" << std::endl;
        ss << "Num Reconnects: " << ofToString(grabbers[i]->getReconnectCount()) << std::endl;
        ss << "Max Reconnects: " << ofToString(grabbers[i]->getMaxReconnects()) << std::endl;
        ss << "  Connect Fail: " << (grabbers[i]->hasConnectionFailed() ? "YES" : "NO");

        ofSetColor(255);
        ofDrawBitmapString(ss.str(), 10, 10+12);
        
        ofDisableAlphaBlending();
        
        ofPopMatrix();
    }
    
    // keep track of some totals
    float avgFPS = totalFPS / numCameras;
    float avgKbps = totalKbps / numCameras;

    ofEnableAlphaBlending();
    ofSetColor(0,80);
    ofDrawRectangle(5,5, 150, 40);
    
    ofSetColor(255);
    // ofToString formatting available in 0072+
    ofDrawBitmapString(" AVG FPS: " + ofToString(avgFPS,2/*,7,' '*/), 10, 17);
    ofDrawBitmapString("AVG Kb/S: " + ofToString(avgKbps,2/*,7,' '*/), 10, 29);
    ofDrawBitmapString("TOT Kb/S: " + ofToString(totalKbps,2/*,7,' '*/), 10, 41);
    ofDisableAlphaBlending();

    ofDrawBitmapString("Press Spacebar for next Video", 10, ofGetHeight() - 14);
}

void ofApp::keyPressed(int key)
{
    if (key == ' ')
    {
        reloadCameras();
    }
}
