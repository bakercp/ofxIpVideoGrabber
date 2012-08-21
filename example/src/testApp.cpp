#include "testApp.h"


/*
inurl:”ViewerFrame?Mode=
intitle:Axis 2400 video server
inurl:/view.shtml
intitle:”Live View / – AXIS | inurl:view/view.shtml^
inurl:ViewerFrame?Mode=
inurl:ViewerFrame?Mode=Refresh
inurl:axis-cgi/jpg
inurl:axis-cgi/mjpg (motion-JPEG)
inurl:view/indexFrame.shtml
inurl:view/index.shtml
inurl:view/view.shtml
liveapplet
intitle:”live view” intitle:axis
intitle:liveapplet
allintitle:”Network Camera NetworkCamera”
intitle:axis intitle:”video server”
intitle:liveapplet inurl:LvAppl
intitle:”EvoCam” inurl:”webcam.html”
intitle:”Live NetSnap Cam-Server feed”
intitle:”Live View / – AXIS”
intitle:”Live View / – AXIS 206M”
intitle:”Live View / – AXIS 206W”
intitle:”Live View / – AXIS 210?
inurl:indexFrame.shtml Axis
inurl:”MultiCameraFrame?Mode=Motion”
intitle:start inurl:cgistart
intitle:”WJ-NT104 Main Page”
intext:”MOBOTIX M1? intext:”Open Menu”
intext:”MOBOTIX M10? intext:”Open Menu”
intext:”MOBOTIX D10? intext:”Open Menu”
intitle:snc-z20 inurl:home/
intitle:snc-cs3 inurl:home/
intitle:snc-rz30 inurl:home/
intitle:”sony network camera snc-p1?
intitle:”sony network camera snc-m1?
site:.viewnetcam.com -www.viewnetcam.com
intitle:”Toshiba Network Camera” user login
intitle:”netcam live image”
intitle:”i-Catcher Console – Web Monitor”
*/

bool locked = false;

//--------------------------------------------------------------
void testApp::setup(){
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetFrameRate(30);
    loadCameras();

    
    // initialize connection
    for(int i = 0; i < NUM_CAMERAS; i++) {
        IPCameraDef& cam = getNextCamera();
        
        ofxSharedIpVideoGrabber c( new ofxIpVideoGrabber());

        // if your camera uses standard web-based authentication, use this
        // c->setUsername(cam.username);
        // c->setPassword(cam.password);
        
        // if your camera uses cookies for authentication, use something like this:
        // ipGrabber[i].setCookie("user", cam.username);
        // ipGrabber[i].setCookie("password", cam.password);
        
        c->setCameraName(cam.name);
        c->setURI(cam.url);
        c->connect(); // connect immediately

        // if desired, set up a video resize listener
        ofAddListener(c->videoResized, this, &testApp::videoResized);
        
        ipGrabber.push_back(c);

    }
}

//--------------------------------------------------------------
IPCameraDef& testApp::getNextCamera() {
    nextCamera = (nextCamera + 1) % ipcams.size();
    return ipcams[nextCamera];
}


//--------------------------------------------------------------
void testApp::loadCameras() {
    
    // all of these cameras were found using this google query
    // http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22
    // some of the cameras below may no longer be valid.
    
    // to define a camera with a username / password
    //ipcams.push_back(IPCameraDef("http://148.61.142.228/axis-cgi/mjpg/video.cgi", "username", "password"));

	ofLog(OF_LOG_NOTICE, "---------------Loading Streams---------------");

	ofxXmlSettings XML;
    
	if( XML.loadFile("streams.xml") ){

        XML.pushTag("streams");
		string tag = "stream";
		
		int nCams = XML.getNumTags(tag);
		
		for(int n = 0; n < nCams; n++) {
            
            IPCameraDef def;

			def.name = XML.getAttribute(tag, "name", "", n);
			def.url = XML.getAttribute(tag, "url", "", n);
			def.username = XML.getAttribute(tag, "username", "", n);
			def.password = XML.getAttribute(tag, "password", "", n);
			
			string logMessage = "STREAM LOADED: " + def.name +
			" url: " +  def.url +
			" username: " + def.username +
			" password: " + def.password;
            
            ofLog(OF_LOG_NOTICE, logMessage);
            
            ipcams.push_back(def);
            
		}
		
		XML.popTag();
		
        
		
	} else {
		ofLog(OF_LOG_ERROR, "Unable to load streams.xml.");
	}
	ofLog(OF_LOG_NOTICE, "-----------Loading Streams Complete----------");
    
    
    nextCamera = ipcams.size();
}


//--------------------------------------------------------------
void testApp::videoResized(const void * sender, ofResizeEventArgs& arg) {
    // find the camera that sent the resize event changed
    for(int i = 0; i < NUM_CAMERAS; i++) {
        if(sender == &ipGrabber[i]) {
            stringstream ss;
            ss << "videoResized: ";
            ss << "Camera connected to: " << ipGrabber[i]->getURI() + " ";
            ss << "New DIM = " << arg.width << "/" << arg.height;
            ofLogVerbose("testApp") << ss.str();
        }
    }
}


//--------------------------------------------------------------
void testApp::update(){
    // update the cameras
    for(size_t i = 0; i < ipGrabber.size(); i++) {
        ipGrabber[i]->update();
    }
}

//--------------------------------------------------------------
void testApp::draw(){
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
    
    for(size_t i = 0; i < ipGrabber.size(); i++) {
        x = col * w;
        y = row * h;

        // draw in a grid
        row = (row + 1) % NUM_ROWS;
        if(row == 0) {
            col = (col + 1) % NUM_COLS;
        }

        
        ofPushMatrix();
        ofTranslate(x,y);
        ofSetColor(255,255,255,255);
        ipGrabber[i]->draw(0,0,w,h); // draw the camera
        
        ofEnableAlphaBlending();
        
        // draw the info box
        ofSetColor(0,80);
        ofRect(5,5,w-10,h-10);
        
        float kbps = ipGrabber[i]->getBitRate() / 1000.0f; // kilobits / second, not kibibits / second
        totalKbps+=kbps;
        
        float fps = ipGrabber[i]->getFrameRate();
        totalFPS+=fps;
        
        
        stringstream ss;
        
        ss << "          NAME: " << ipGrabber[i]->getCameraName() << endl;
        ss << "          HOST: " << ipGrabber[i]->getHost() << endl;
        ss << "           FPS: " << ofToString(fps,  2,13,' ') << endl;
        ss << "          Kb/S: " << ofToString(kbps, 2,13,' ') << endl;
        ss << " #Bytes Recv'd: " << ofToString(ipGrabber[i]->getNumBytesReceived(),  0,10,' ') << endl;
        ss << "#Frames Recv'd: " << ofToString(ipGrabber[i]->getNumFramesReceived(), 0,10,' ') << endl;
        ss << "Auto Reconnect: " << (ipGrabber[i]->getAutoReconnect() ? "YES" : "NO") << endl;
        ss << " Needs Connect: " << (ipGrabber[i]->getNeedsReconnect() ? "YES" : "NO") << endl;
        ss << "Time Till Next: " << ipGrabber[i]->getTimeTillNextAutoRetry() << " ms" << endl;
        ss << "Num Reconnects: " << ofToString(ipGrabber[i]->getReconnectCount()) << endl;
        ss << "Max Reconnects: " << ofToString(ipGrabber[i]->getMaxReconnects()) << endl;
        ss << "  Connect Fail: " << (ipGrabber[i]->hasConnectionFailed() ? "YES" : "NO");

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
    ofRect(5,5, 150, 40);
    
    ofSetColor(255);
    ofDrawBitmapString(" AVG FPS: " + ofToString(avgFPS,2,7,' '), 10,17);
    ofDrawBitmapString("AVG Kb/S: " + ofToString(avgKbps,2,7,' '), 10,29);
    ofDrawBitmapString("TOT Kb/S: " + ofToString(totalKbps,2,7,' '), 10,41);
    ofDisableAlphaBlending();

}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){
    if(key == ' ') {
        // initialize connection
        for(int i = 0; i < NUM_CAMERAS; i++) {
            ofRemoveListener(ipGrabber[i]->videoResized, this, &testApp::videoResized);
            ofxSharedIpVideoGrabber c( new ofxIpVideoGrabber());
            IPCameraDef& cam = getNextCamera();
            c->setUsername(cam.username);
            c->setPassword(cam.password);
            URI uri(cam.url);
            c->setURI(uri);
            c->connect();
            
            ipGrabber[i] = c;
            
        }
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int butipGrabbern){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int butipGrabbern){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int butipGrabbern){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
