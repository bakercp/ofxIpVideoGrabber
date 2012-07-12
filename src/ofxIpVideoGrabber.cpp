/*==============================================================================
 
 Copyright (c) 2011, 2012 Christopher Baker <http://christopherbaker.net>
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 ==============================================================================*/


#include "ofxIpVideoGrabber.h"

#define MODE_HEADER 0
#define MODE_JPEG   1

// jpeg starting and ending bytes
#define JFF static_cast<char> (0xFF)
#define SOI static_cast<char> (0xD8)
#define EOI static_cast<char> (0xD9)

#define BUF_LEN 512000 // 8 * 65536 = 512 kB

//--------------------------------------------------------------
ofxIpVideoGrabber::ofxIpVideoGrabber() : ofBaseVideoDraws(), ofThread() {

    
    bIsConnected = false;
    ci = 0; // current index
    // prepare the buffer
    image[0] = ofPtr<ofImage>(new ofImage());
    image[0]->setUseTexture(false); 
    image[1] = ofPtr<ofImage>(new ofImage());
    image[1]->setUseTexture(false);  // we cannot use textures b/c loading data
                                    // happens in another thread an opengl  
                                    // requires a single thread.

    img = ofPtr<ofImage>(new ofImage());
    img->allocate(1,1, OF_IMAGE_COLOR); // allocate something so it won't throw errors
    
    isBackBufferReady = false;
    isNewFrameLoaded  = false;
    
    ofAddListener(ofEvents().exit, this, &ofxIpVideoGrabber::exit); // for cleanup
}

//--------------------------------------------------------------
ofxIpVideoGrabber::~ofxIpVideoGrabber() {
    stopThread();
}


//--------------------------------------------------------------
void ofxIpVideoGrabber::connect() {
    if(!bIsConnected) {
        resetStats();
        startThread(true, false);   // blocking, verbose
    } else {
        ofLog(OF_LOG_ERROR, "ofxIpVideoGrabber: Already connected.  Disconnect first.");
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::waitForDisconnect() {
    if(bIsConnected) {
        resetStats();
        if(session.connected()) {
            session.reset(); // close the session httpclient
        }
        waitForThread(true); // close it all down (politely)
    } else {
        ofLog(OF_LOG_WARNING, "ofxIpVideoGrabber: Not connected. Connect first.");
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::disconnect() {
    if(bIsConnected) {
        resetStats();
        if(session.connected()) {
            session.reset(); // close the session httpclient
        }
        stopThread();
    } else {
        ofLog(OF_LOG_WARNING, "ofxIpVideoGrabber: Not connected. Connect first.");
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::close() {
    disconnect();
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isConnected() {
    return bIsConnected;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::threadedFunction(){

    try {
        session.setHost(uri.getHost());
        session.setPort(uri.getPort());
        
        session.setKeepAlive(true);
        session.setTimeout(Poco::Timespan(2, 0));
        
        string path(uri.getPathAndQuery());
        if (path.empty()) path = "/";

        HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        credentials.authenticate(req); // autheticate

        session.sendRequest(req);

    } catch (Poco::Exception& exc) {
        ofLog(OF_LOG_ERROR,"ofxIpVideoGrabber: [" + getURI() +"]:" + exc.displayText());
        session.reset();
        return;
    }

    
    HTTPResponse res;
    istream* rs;

    try {
        rs = (istream*) &session.receiveResponse(res);
        // istream& rs = session.receiveResponse(res);
    } catch (Poco::Net::NoMessageException& e) {
        ofLog(OF_LOG_ERROR,"ofxIpVideoGrabber: [" + getURI() +"] : " + e.displayText());
        session.reset();
        return;
    }

    HTTPResponse::HTTPStatus status = res.getStatus(); 

    if(status != HTTPResponse::HTTP_OK) {
        string reason = res.getReasonForStatus(status);
        ofLog(OF_LOG_ERROR, "ofxIpVideoGrabber: Error connecting! [" + getURI() +"]: " + reason + " (" + ofToString(status) + ")");
        session.reset();
        return;
    }
    
    bIsConnected = true;  // we are now connected
    
    vector<string> param = ofSplitString(res.getContentType(),";", true); // split it (try)
    
    NameValueCollection nvc;
    string contentType;
    string boundaryMarker;
    HTTPResponse::splitParameters(res.getContentType(), contentType, nvc);
    boundaryMarker = nvc.get("boundary","--myboundary"); // AXIS default (--myboundary)
    
    if(icompare(string("--"), boundaryMarker.substr(0,2)) != 0) {
        boundaryMarker = "--" + boundaryMarker; // prepend the marker 
    }
    
    int mode = MODE_HEADER;
    
    ofBuffer buffer;
    char* cBuf = new char[BUF_LEN];
    int c = 0;
    
    bool resetBuffer = false;

    // mjpeg params
    int contentLength = 0;
    string boundaryType;
    
    
    while( isThreadRunning() != 0 && rs->good() ){

        resetBuffer = false;
        rs->get(cBuf[c]); // put a byte in the buffer
        nBytes++; // count bytes
        
        if(c > 0) {
            if(mode == MODE_HEADER && cBuf[c-1] == '\r' && cBuf[c] == '\n') {
                if(c > 1) {
                    cBuf[c-1] = 0; // NULL terminator
                    string line(cBuf); // make a string object
                    
                    vector<string> keyValue = ofSplitString(line,":", true); // split it (try)
                    if(keyValue.size() > 1) { // a param!
                        
                        string key =   trim(keyValue[0]);
                        string value = trim(keyValue[1]);

                        if(icompare(string("content-length"), key) == 0) {
                            contentLength = ofToInt(value);
                            // TODO: we don't currently use content length, but could
                        } else if(icompare(string("content-type"), key) == 0) {
                            boundaryType = value;
                        } else {
                            // some other header
                        }
                    } else {
                        if(icompare(boundaryMarker, line) == 0) {
                            mode = MODE_HEADER;
                        } else {
                            // just a new line
                        }
                        // this is where line == "--myboundary"
                    }
                } else {
                    // just waiting for at least two bytes
                }
                
                resetBuffer = true; // reset upon new line
                
            } else if(cBuf[c-1] == JFF) {
                if(cBuf[c] == SOI ) {
                    mode = MODE_JPEG;
                } else if(cBuf[c] == EOI ) {
                    buffer = ofBuffer(cBuf, c+1);
                    lock(); 
                    {
                        // there is a jpeg in the buffer
                        // get the back image (ci^1)
                        image[ci^1]->loadImage(buffer); 
                        isBackBufferReady = true;
                    }
                    unlock();
                    nFrames++; // incrase frame cout
                    
                    mode = MODE_HEADER;
                    resetBuffer = true;
                } else {}
            } else if(mode == MODE_JPEG) {
            } else {}
        } else {}
        
        
        // check for buffer overflow
        if(c >= BUF_LEN) {
            resetBuffer = true;
            ofLog(OF_LOG_ERROR, "ofxIpVideoGrabber: [" + getURI() +"]: BUFFER OVERFLOW, RESETTING");
        }
        
        if(resetBuffer) {
            c = 0;
        } else {
            c++;
        }
    }
    
    delete[] cBuf;

    // we may have already aborted the connection. but double check
    if(session.connected()) {
        session.reset(); // close the session httpclient
    }
    bIsConnected = false;

}

//--------------------------------------------------------------
void ofxIpVideoGrabber::exit(ofEventArgs & a) {
    waitForDisconnect();
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isFrameNew() {
    return isNewFrameLoaded;
}

//--------------------------------------------------------------
unsigned char * ofxIpVideoGrabber::getPixels() {
    return img->getPixels();
}

//--------------------------------------------------------------
ofPixelsRef ofxIpVideoGrabber::getPixelsRef() {
    return img->getPixelsRef();
}

//--------------------------------------------------------------
ofPtr<ofImage> ofxIpVideoGrabber::getFrame() {
    return img;
}

//--------------------------------------------------------------
ofTexture & ofxIpVideoGrabber::getTextureReference() {
    return img->getTextureReference();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setUseTexture(bool bUseTex) {
    img->setUseTexture(bUseTex);
}

//--------------------------------------------------------------
float ofxIpVideoGrabber::getWidth() {
    return img->getWidth();
}

//--------------------------------------------------------------
float ofxIpVideoGrabber::getHeight() {
    return img->getHeight();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::update(ofEventArgs & a) {
    update();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::update() {
    // do opengl texture loading from the main update thread
    if(bIsConnected && isBackBufferReady) {
        
        lock();  // lock down the thread!
        {
            ci^=1; // switch buffers (ci^1) was just filled in the thread
            
            int newW = image[ci]->getWidth();    // new buffer
            int newH = image[ci]->getHeight();   // new buffer
            int oldW = image[ci^1]->getWidth();  // old buffer
            int oldH = image[ci^1]->getHeight(); // old buffer
            
            // send out an alert to anyone who cares.
            // on occasion an mjpeg image stream size can be changed on the fly 
            // without our knowledge.  so tell people about it.
            if(newW != oldW || newH != oldH) imageResized(newW, newH);
            
            // get a pixel ref for the image that was just loaded in the thread
            img = ofPtr<ofImage>(new ofImage());
            img->setFromPixels(image[ci]->getPixelsRef());
            
            isNewFrameLoaded = true;
        }
        
        isBackBufferReady = false;
        
        unlock();
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::draw(float x, float y){
    img->draw(x,y);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::draw(float x, float y, float width, float height) {
    img->draw(x,y,width,height);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::draw(const ofPoint & point){
    draw( point.x, point.y);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::draw(const ofRectangle & rect){
    draw(rect.x, rect.y, rect.width, rect.height); 
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setAnchorPercent(float xPct, float yPct) {
    img->setAnchorPercent(xPct,yPct);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setAnchorPoint(float x, float y) {
    img->setAnchorPoint(x,y);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::resetAnchor() {
    img->resetAnchor();
}


//--------------------------------------------------------------
float ofxIpVideoGrabber::getFrameRate() {
    if(!bIsConnected) return 0;
    if(t0 == 0) t0 = ofGetSystemTime(); // start time
    elapsedTime = (int)(ofGetSystemTime() - t0);
    return float(nFrames) / (elapsedTime / (1000.0f));
}
    
//--------------------------------------------------------------
float ofxIpVideoGrabber::getBitRate() {
    if(!bIsConnected) return 0;
    if(t0 == 0) t0 = ofGetSystemTime(); // start time
    elapsedTime = (int)(ofGetSystemTime() - t0);
    return 8 * float(nBytes) / (elapsedTime / (1000.0f)); // bits per second
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setUsername(string username) {
    credentials.setUsername(username); // autheticate
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setPassword(string password) {
    credentials.setPassword(password); // autheticate
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getUsername() {
    return credentials.getUsername();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getPassword() {
    return credentials.getPassword();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getURI() {
    return uri.toString();
}


//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(string _uri) {
    URI uri(_uri);
    setURI(uri);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(URI _uri) {
    uri = _uri;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyUsername(string username) {
    session.setProxyUsername(username);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPassword(string password) {
    session.setProxyPassword(password);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyHost(string host) {
    session.setProxyHost(host);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPort(int port) {
    session.setProxyPort(port);
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyUsername() {
    return getProxyUsername();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyPassword() {
    return getProxyPassword();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyHost() {
    return getProxyHost();
}

//--------------------------------------------------------------
int ofxIpVideoGrabber::getProxyPort() {
    return getProxyPort();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::imageResized(int width, int height) {
    ofResizeEventArgs resizeEvent;
    resizeEvent.width = width;
    resizeEvent.height = height;
    ofNotifyEvent(videoResized, resizeEvent, this);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::resetStats() {
    t0 = 0;
    elapsedTime = 0;
    nBytes = 0;
    nFrames = 0;
}

//--------------------------------------------------------------
HTTPClientSession* ofxIpVideoGrabber::getSession() {
    return &session;
}


