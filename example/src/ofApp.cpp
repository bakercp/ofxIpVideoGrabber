// =============================================================================
//
// Copyright (c) 2009-2015 Christopher Baker <http://christopherbaker.net>
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
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetFrameRate(30);
    loadCameras();
    
    // initialize connection
    for (std::size_t i = 0; i < NUM_CAMERAS; i++)
    {
        IPCameraDef& cam = getNextCamera();

		std::shared_ptr<Video::IPVideoGrabber> c = std::make_shared<Video::IPVideoGrabber>();

        // if your camera uses standard web-based authentication, use this
        // c->setUsername(cam.username);
        // c->setPassword(cam.password);
        
        // if your camera uses cookies for authentication, use something like this:
        // c->setCookie("user", cam.username);
        // c->setCookie("password", cam.password);
        
        c->setCameraName(cam.getName());
        c->setURI(cam.getURL());
        c->connect(); // connect immediately

        // if desired, set up a video resize listener
        ofAddListener(c->videoResized, this, &ofApp::videoResized);
        
        grabbers.push_back(c);

    }
}


IPCameraDef& ofApp::getNextCamera()
{
    nextCamera = (nextCamera + 1) % ipcams.size();
    return ipcams[nextCamera];
}


void ofApp::loadCameras()
{
    
    // all of these cameras were found using this google query
    // http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22
    // some of the cameras below may no longer be valid.
    
    // to define a camera with a username / password
    //ipcams.push_back(IPCameraDef("http://148.61.142.228/axis-cgi/mjpg/video.cgi", "username", "password"));

	ofLog(OF_LOG_NOTICE, "---------------Loading Streams---------------");

	ofxXmlSettings XML;
    
	if (XML.loadFile("streams.xml"))
    {
        XML.pushTag("streams");
        std::string tag = "stream";
		
		int nCams = XML.getNumTags(tag);
		
		for (std::size_t n = 0; n < nCams; ++n)
        {
            
            IPCameraDef def(XML.getAttribute(tag, "name", "", n),
                            XML.getAttribute(tag, "url", "", n),
                            XML.getAttribute(tag, "username", "", n),
                            XML.getAttribute(tag, "password", "", n));


            std::string logMessage = "STREAM LOADED: " + def.getName() +
			" url: " +  def.getURL() +
			" username: " + def.getUsername() +
			" password: " + def.getPassword();
            
            ofLogNotice() << logMessage;
            
            ipcams.push_back(def);
            
		}
		
		XML.popTag();

	}
    else
    {
		ofLog(OF_LOG_ERROR, "Unable to load streams.xml.");
	}
    
	ofLog(OF_LOG_NOTICE, "-----------Loading Streams Complete----------");
    
    nextCamera = ipcams.size();
}


void ofApp::videoResized(const void* sender, ofResizeEventArgs& arg)
{
    // find the camera that sent the resize event changed
    for(std::size_t i = 0; i < NUM_CAMERAS; ++i)
    {
        if(sender == &grabbers[i])
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
    // update the cameras
    for (std::size_t i = 0; i < grabbers.size(); ++i)
    {
        grabbers[i]->update();
    }
}


void ofApp::draw()
{
	ofBackground(0,0,0);

	ofSetHexColor(0xffffff);
    
    int row = 0;
    int col = 0;
    
    int x = 0;
    int y = 0;
    
    int w = ofGetWidth() / NUM_COLS;
    int h = ofGetHeight() / NUM_ROWS;
    
    float totalKbps = 0;
    float totalFPS = 0;
    
    for(std::size_t i = 0; i < grabbers.size(); i++)
    {
        x = col * w;
        y = row * h;

        // draw in a grid
        row = (row + 1) % NUM_ROWS;

        if(row == 0)
        {
            col = (col + 1) % NUM_COLS;
        }

        
        ofPushMatrix();
        ofTranslate(x,y);
        ofSetColor(255,255,255,255);
        grabbers[i]->draw(0,0,w,h); // draw the camera
        
        ofEnableAlphaBlending();
        
        // draw the info box
        ofSetColor(0,80);
        ofDrawRectangle(5,5,w-10,h-10);
        
        float kbps = grabbers[i]->getBitRate() / 1000.0f; // kilobits / second, not kibibits / second
        totalKbps+=kbps;
        
        float fps = grabbers[i]->getFrameRate();
        totalFPS+=fps;
        
        std::stringstream ss;
        
        // ofToString formatting available in 0072+
        ss << "          NAME: " << grabbers[i]->getCameraName() << endl;
        ss << "          HOST: " << grabbers[i]->getHost() << endl;
        ss << "           FPS: " << ofToString(fps,  2/*,13,' '*/) << endl;
        ss << "          Kb/S: " << ofToString(kbps, 2/*,13,' '*/) << endl;
        ss << " #Bytes Recv'd: " << ofToString(grabbers[i]->getNumBytesReceived(),  0/*,10,' '*/) << endl;
        ss << "#Frames Recv'd: " << ofToString(grabbers[i]->getNumFramesReceived(), 0/*,10,' '*/) << endl;
        ss << "Auto Reconnect: " << (grabbers[i]->getAutoReconnect() ? "YES" : "NO") << endl;
        ss << " Needs Connect: " << (grabbers[i]->getNeedsReconnect() ? "YES" : "NO") << endl;
        ss << "Time Till Next: " << grabbers[i]->getTimeTillNextAutoRetry() << " ms" << endl;
        ss << "Num Reconnects: " << ofToString(grabbers[i]->getReconnectCount()) << endl;
        ss << "Max Reconnects: " << ofToString(grabbers[i]->getMaxReconnects()) << endl;
        ss << "  Connect Fail: " << (grabbers[i]->hasConnectionFailed() ? "YES" : "NO");

        ofSetColor(255);
        ofDrawBitmapString(ss.str(), 10, 10+12);
        
        ofDisableAlphaBlending();
        
        ofPopMatrix();
    }
    
    // keep track of some totals
    float avgFPS = totalFPS / NUM_CAMERAS;
    float avgKbps = totalKbps / NUM_CAMERAS;

    ofEnableAlphaBlending();
    ofSetColor(0,80);
    ofDrawRectangle(5,5, 150, 40);
    
    ofSetColor(255);
    // ofToString formatting available in 0072+
    ofDrawBitmapString(" AVG FPS: " + ofToString(avgFPS,2/*,7,' '*/), 10,17);
    ofDrawBitmapString("AVG Kb/S: " + ofToString(avgKbps,2/*,7,' '*/), 10,29);
    ofDrawBitmapString("TOT Kb/S: " + ofToString(totalKbps,2/*,7,' '*/), 10,41);
    ofDisableAlphaBlending();

}


void ofApp::keyPressed(int key)
{
    if(key == ' ')
    {
        // initialize connection
        for (std::size_t i = 0; i < NUM_CAMERAS; ++i)
        {
            ofRemoveListener(grabbers[i]->videoResized, this, &ofApp::videoResized);
			std::shared_ptr<Video::IPVideoGrabber> c = std::make_shared<Video::IPVideoGrabber>();
            IPCameraDef& cam = getNextCamera();
            c->setUsername(cam.getUsername());
            c->setPassword(cam.getPassword());
            Poco::URI uri(cam.getURL());
            c->setURI(uri);
            c->connect();
            
            grabbers[i] = c;
        }
    }
}
