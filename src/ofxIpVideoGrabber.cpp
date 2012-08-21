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
#define MINIMUM_JPEG_SIZE 134 // minimum number of bytes for a valid jpeg

// jpeg starting and ending bytes
#define JFF static_cast<char> (0xFF)
#define SOI static_cast<char> (0xD8)
#define EOI static_cast<char> (0xD9)

#define BUF_LEN 512000 // 8 * 65536 = 512 kB

//--------------------------------------------------------------
ofxIpVideoGrabber::ofxIpVideoGrabber() : ofBaseVideoDraws(), ofThread() {
    
    connectionState = DISCONNECTED;
    
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
    cameraName = "";
    
    // THIS IS EXTREMELY IMPORTANT.
    // To shut down the thread cleanly, we cannot allow openFrameworks to de-init FreeImage
    // before this thread has completed any current image loads.  Thus we need to make sure
    // that this->waitForDisconnect() is called before the main oF loop calls
    // ofImage::ofCloseFreeImage().  This can often result in slight lags during shutdown,
    // especially when multiple threads are running, but it prevents crashes.
    // To do that, we simply need to register an ofEvents.exit listener.
    // Alternatively, each thread could instantiate its own instance of FreeImage, but that
    // seems memory inefficient.  Thus, the shutdown lag for now.

    ofAddListener(ofEvents().exit,this,&ofxIpVideoGrabber::exit);
}

//--------------------------------------------------------------
ofxIpVideoGrabber::~ofxIpVideoGrabber() {
    // N.B. In most cases, ofxIpVideoGrabber::exit() will be called before
    // the program ever makes it into this destructor, but in some cases we
    waitForDisconnect(); //
    
    // it is ok to unregister an item that is not currently registered
    // POCO's internal loop won't complain or return errors
    // POCO stores the delegates in a std::vector and iterates through
    // deleting and returning on match, and doing nothing on a no-match condition
    ofRemoveListener(ofEvents().exit,this,&ofxIpVideoGrabber::exit);
}

void ofxIpVideoGrabber::exit(ofEventArgs & a) {
    waitForDisconnect();
    ofRemoveListener(ofEvents().exit,this,&ofxIpVideoGrabber::exit);
}


//--------------------------------------------------------------
void ofxIpVideoGrabber::connect() {
    if(isDisconnected()) {
        resetStats();
        startThread(true, false);   // blocking, verbose
    } else {
        ofLog(OF_LOG_ERROR, "ofxIpVideoGrabber: Already connected.  Disconnect first.");
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::waitForDisconnect() {
    if(!isDisconnected()) {
        resetStats();
        if(session.connected()) {
            session.reset(); // close the session httpclient, this might cause an exception to be thrown
        }
        waitForThread(true); // close it all down (politely)
    } else {
        ofLog(OF_LOG_WARNING, "ofxIpVideoGrabber: Not connected. Connect first.");
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::disconnect() {
    if(isConnected()) {
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
bool ofxIpVideoGrabber::isConnected() const {
    return connectionState == CONNECTED;
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isDisconnected() const {
    return connectionState == DISCONNECTED;
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isConnecting() const {
    return connectionState == CONNECTING;
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isDisconnecting() const {
    return connectionState == DISCONNECTING;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::threadedFunction(){

    connectionState = CONNECTING;
    
    try {
        session.setHost(uri.getHost());
        session.setPort(uri.getPort());
        
        session.setKeepAlive(true);
        session.setTimeout(Poco::Timespan(2, 0));
        
        string path(uri.getPathAndQuery());
        if (path.empty()) path = "/";

        HTTPRequest req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
        credentials.authenticate(req); // autheticate
        req.setCookies(cookies);

        session.sendRequest(req);

    } catch (Poco::Exception& exc) {
        ofLog(OF_LOG_ERROR,"ofxIpVideoGrabber: [" + getURI() +"]:" + exc.displayText());
        session.reset();
        connectionState = DISCONNECTED;
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
        connectionState = DISCONNECTED;
        return;
    }

    HTTPResponse::HTTPStatus status = res.getStatus(); 

    if(status != HTTPResponse::HTTP_OK) {
        string reason = res.getReasonForStatus(status);
        ofLog(OF_LOG_ERROR, "ofxIpVideoGrabber: Error connecting! [" + getURI() +"]: " + reason + " (" + ofToString(status) + ")");
        session.reset();
        connectionState = DISCONNECTED;
        return;
    }
    
    connectionState = CONNECTED;
    
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
                    if( c >= MINIMUM_JPEG_SIZE) { // some cameras send 2+ EOIs in a row, with no valid bytes in between
                        lock(); 
                        {
                            // there is a jpeg in the buffer
                            // get the back image (ci^1)
                            image[ci^1]->loadImage(buffer); 
                            isBackBufferReady = true;
                        }
                        unlock();
                        nFrames++; // incrase frame cout
                    } else {
                        ofLogVerbose("ofxIpVideoGrabber: received EOI, but number of bytes was less than MINIMUM_JPEG_SIZE.  Resetting."); 
                    }
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
    cBuf = NULL;

    // we may have already aborted the connection. but double check
    if(session.connected()) {
        session.reset(); // close the session httpclient
    }
    
    connectionState = DISCONNECTED;
    
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
    if(isConnected() && isBackBufferReady) {
        
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
    if(!isConnected()) return 0;
    if(t0 == 0) t0 = ofGetSystemTime(); // start time
    elapsedTime = (int)(ofGetSystemTime() - t0);
    return float(nFrames) / (elapsedTime / (1000.0f));
}
    
//--------------------------------------------------------------
float ofxIpVideoGrabber::getBitRate() {
    if(!isConnected()) return 0;
    if(t0 == 0) t0 = ofGetSystemTime(); // start time
    elapsedTime = (int)(ofGetSystemTime() - t0);
    return (float(nBytes) / 8.0f) / (elapsedTime / (1000.0f)); // bits per second
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setCameraName(const string& _cameraName) {
    cameraName = _cameraName;
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getCameraName() const {
    if(cameraName.empty()) {
        return getURI();
    } else {
        return cameraName;
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setCookie(const string& key, const string& value) {
    cookies.add(key, value);
}

void ofxIpVideoGrabber::eraseCookie(const string& key) {
    cookies.erase(key);
}

string ofxIpVideoGrabber::getCookie(const string& key) const {
    return cookies.get(key);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setUsername(const string& username) {
    credentials.setUsername(username); // autheticate
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setPassword(const string& password) {
    credentials.setPassword(password); // autheticate
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getUsername() const {
    return credentials.getUsername();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getPassword() const {
    return credentials.getPassword();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getURI() const {
    return uri.toString();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getHost() const {
    return uri.getHost();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getQuery() const {
    return uri.getQuery();
}

//--------------------------------------------------------------
int ofxIpVideoGrabber::getPort() const {
    return (int)uri.getPort();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getFragment() const {
    return uri.getFragment();
}

URI ofxIpVideoGrabber::getPocoURI() const {
    return uri;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(const string& _uri) {
    URI uri(_uri);
    setURI(uri);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(const URI& _uri) {
    uri = _uri;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyUsername(const string& username) {
    session.setProxyUsername(username);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPassword(const string& password) {
    session.setProxyPassword(password);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyHost(const string& host) {
    session.setProxyHost(host);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPort(int port) {
    session.setProxyPort(port);
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyUsername() const {
    return getProxyUsername();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyPassword() const {
    return getProxyPassword();
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyHost() const {
    return getProxyHost();
}

//--------------------------------------------------------------
int ofxIpVideoGrabber::getProxyPort() const {
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


