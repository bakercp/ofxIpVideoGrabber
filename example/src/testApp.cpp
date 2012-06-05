#include "testApp.h"

bool locked = false;

//--------------------------------------------------------------
void testApp::setup(){
    ofSetFrameRate(30);
    loadCameras();
    rs = new RandomSampler(ipcams.size());    
    // initialize connection
    for(int i = 0; i < NUM_CAMERAS; i++) {
        IPCameraDef& cam = getRandomCamera();
        ipGrabber[i].setUsername(cam.username);
        ipGrabber[i].setPassword(cam.password);
        URI uri(cam.uri);
        ipGrabber[i].setURI(uri);
        ipGrabber[i].connect();
        // set up the listener!
        ofAddListener(ipGrabber[i].videoResized, this, &testApp::videoResized);
    }
}

//--------------------------------------------------------------
IPCameraDef& testApp::getRandomCamera() {
//    int i = (int)ofRandom(0, ipcams.size());
    int i = rs->next();
    return ipcams[i];
}

//--------------------------------------------------------------
void testApp::loadCameras() {
    
    // all of these cameras were found using this google query
    // http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22
    
    // to define a camera with a username / password
    //ipcams.push_back(IPCameraDef("http://148.61.142.228/axis-cgi/mjpg/video.cgi", "username", "password"));

    ipcams.push_back(IPCameraDef("http://148.61.142.228/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://82.79.176.85:8081/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://81.8.151.136:88/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://130.15.110.15/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://193.68.123.245/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://80.34.88.249/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://212.244.173.167/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://194.103.218.15/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://216.8.159.21/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://130.191.227.248/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://kassertheatercam.montclair.edu/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://194.17.150.25/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://130.95.52.185/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://74.94.55.182/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://213.77.33.2:8080/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://129.89.28.32/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://129.171.176.150/axis-cgi/mjpg/video.cgi?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://134.29.208.43/mjpg/video.mjpg?resolution=320x240"));
    ipcams.push_back(IPCameraDef("http://134.29.208.43/mjpg/video.mjpg?resolution=320x240"));
    
}


//--------------------------------------------------------------
void testApp::videoResized(const void * sender, ofResizeEventArgs& arg) {

    ofLog(OF_LOG_VERBOSE, "A VIDEO GRABBER WAS RESIZED");

    // find the camera that sent the resize event changed
    for(int i = 0; i < NUM_CAMERAS; i++) {
        if(sender == &ipGrabber[i]) {
            string msg = "Camera connected to: " + ipGrabber[i].getURI() + " ";
            msg+= ("New DIM = " + ofToString(arg.width) + "/" + ofToString(arg.height));
            ofLog(OF_LOG_VERBOSE, msg);

        }
    }

    ofLog(OF_LOG_VERBOSE, "Unable to locate the camera.  Very odd.");

    
}


//--------------------------------------------------------------
void testApp::update(){
    // update the cameras
    for(int i = 0; i < NUM_CAMERAS; i++) {
        ipGrabber[i].update();
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
    
    int w = 320;
    int h = 240;
    
    float totalKBPS = 0;
    float totalFPS = 0;
    
    for(int i = 0; i < NUM_CAMERAS; i++) {
        x = col * w;
        y = row * h;

        
        row = (row + 1) % NUM_ROWS;
        if(row == 0) {
            col = (col + 1) % NUM_COLS;
        }

        
        ofPushMatrix();
        ofTranslate(x,y);
        ofSetColor(255,255,255,255);
        ipGrabber[i].draw(0,0,w,h);
        
        
        ofEnableAlphaBlending();
        
        ofSetColor(0,0,0,127);
        ofRect(10,h-45,w-20,35);
        
        float kbps = ipGrabber[i].getBitRate() / (8 * 1000.0);
        totalKBPS+=kbps;
        
        float fps = ipGrabber[i].getFrameRate();
        totalFPS+=fps;
        
        ofSetColor(255,255,255);
        ofDrawBitmapString(" FPS: " + ofToString(fps, 2), 20, h-35);
        ofDrawBitmapString("KB/S: " + ofToString(kbps,2), 20, h-20);
        
        ofDisableAlphaBlending();
        
        ofPopMatrix();
    }
    
    float avgFPS = totalFPS / NUM_CAMERAS;
    float avgKBPS = totalKBPS / NUM_CAMERAS;

    ofEnableAlphaBlending();
    ofSetColor(255,255,255,80);
    ofRect(0,0, 50, 20);
    ofSetColor(0,0,0,80);
    
    ofDrawBitmapString(" AVG_FPS: " + ofToString(avgFPS,   2), 5,10);
    ofDrawBitmapString("AVG_KBPS: " + ofToString(avgKBPS,  2), 5,20);
    ofDrawBitmapString("TOT_KBPS: " + ofToString(totalKBPS,2), 5,30);
    ofDisableAlphaBlending();

}

//--------------------------------------------------------------
void testApp::keyPressed  (int key){
    if(key == ' ') {
     
        rs->reset();
        // initialize connection
        for(int i = 0; i < NUM_CAMERAS; i++) {
            ipGrabber[i].waitForDisconnect();
            // we may need to wait a sec here
            IPCameraDef& cam = getRandomCamera();
            ipGrabber[i].setUsername(cam.username);
            ipGrabber[i].setPassword(cam.password);
            URI uri(cam.uri);
            ipGrabber[i].setURI(uri);
            ipGrabber[i].connect();
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