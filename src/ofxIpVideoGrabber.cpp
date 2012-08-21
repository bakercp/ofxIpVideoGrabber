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


enum ContentStreamMode {
    MODE_HEADER = 0,
    MODE_JPEG = 1
};

static int MIN_JPEG_SIZE = 134; // minimum number of bytes for a valid jpeg

// jpeg starting and ending bytes
static int BUF_LEN = 512000; // 8 * 65536 = 512 kB

static char JFF = 0xFF;
static char SOI = 0xD8;
static char EOI = 0xD9;

//--------------------------------------------------------------
ofxIpVideoGrabber::ofxIpVideoGrabber() : ofBaseVideoDraws(), ofThread() {
    
    ci = 0; // current index
    
    // prepare the double buffer
    for(int i = 0; i < 2; i++) {
        // we only load pixel data b/c tex allocation can only happen in main thread
        image[i].setUseTexture(false);
    }
    
    img = ofPtr<ofImage>(new ofImage());
    img->allocate(1,1, OF_IMAGE_COLOR); // allocate something so it won't throw errors
    img->setColor(0,0,ofColor(0));
    
    isNewFrameLoaded  = false;
    isBackBufferReady = false;
    
    cameraName = "";
    
    sessionTimeout = 2000; // ms
    
    reconnectTimeout = 5000;
    
    // stats
    lastValidBitrateTime = 0;
    
    currentBitRate   = 0.0;
    currentFrameRate = 0.0;
    nBytesReceived   = 0;
    nFramesReceived  = 0;
    
    connectTime_a = 0;
    elapsedTime_a = 0;
    nBytes_a      = 0;
    nFrames_a     = 0;
    
    needsReconnect = false;
    // stats
    
    minBitrate = 8;

    connectionFailure = false;
    needsReconnect = false;
    autoReconnect  = true;
    
    reconnectCount = 0;
    maxReconnects = 20;
    
    retryDelay = 1000; // at least 1 second retry delay
    nextRetry = 0;

    bUseProxy = false;
    proxyUsername = "";
    proxyPassword = "";
    proxyHost = "127.0.0.1";
    proxyPort = HTTPSession::HTTP_PORT;
    
    // AXIS default (--myboundary)
    // other cameras have other odd boundaries
    defaultBoundaryMarker = "--myboundary";
    
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
void ofxIpVideoGrabber::update(ofEventArgs & a) {
    update();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::update() {
        
    if(!isMainThread()) {
        // we enforce this because textures are uploaded here.
        ofLogError("ofxIpVideoGrabber") << "update() may not be called from outside the main thread.  Returning.";
        return;
    }

    bool shouldDisconnect = false;

    unsigned long now = ofGetSystemTime();

    if(isConnected()) {

        ///////////////////////////////
        mutex.lock();     // LOCKING //
        ///////////////////////////////
        
        // do opengl texture loading from the main update thread 

        elapsedTime_a = (connectTime_a == 0) ? 0 : (now - connectTime_a);

        if(isBackBufferReady) {
            ci^=1; // swap buffers (ci^1) was just filled in the thread
            
            int newW = image[ci].getWidth();    // new buffer
            int newH = image[ci].getHeight();   // new buffer
            int oldW = image[ci^1].getWidth();  // old buffer
            int oldH = image[ci^1].getHeight(); // old buffer
            
            // send out an alert to anyone who cares.
            // on occasion an mjpeg image stream size can be changed on the fly
            // without our knowledge.  so tell people about it.
            if(newW != oldW || newH != oldH) imageResized(newW, newH);
            
            // get a pixel ref for the image that was just loaded in the thread
            
            img = ofPtr<ofImage>(new ofImage());
            img->setFromPixels(image[ci].getPixelsRef());
            
            isNewFrameLoaded = true;
            
            isBackBufferReady = false;
        }
        
        nBytesReceived = nBytes_a;
        nFramesReceived = nFrames_a;
        
        if(elapsedTime_a > 0) {
            currentFrameRate = ( float(nFrames_a) )       / (elapsedTime_a / (1000.0f)); // frames per second
            currentBitRate   = ( float(nBytes_a ) / 8.0f) / (elapsedTime_a / (1000.0f)); // bits per second
        } else {
            currentFrameRate = 0;
            currentBitRate   = 0;
        }
        
        if(currentBitRate > minBitrate) {
            lastValidBitrateTime = elapsedTime_a;
        } else {
            if((elapsedTime_a - lastValidBitrateTime) > reconnectTimeout) {
                ofLogVerbose("ofxIpVideoGrabber") << "["<< getCameraName() << "] Disconnecting because of slow bitrate." << isConnected();
                needsReconnect = true;
            } else {
                // slowed below min bitrate, waiting to see if we are too low for long enought to merit a reconnect
            }
        }
        
        ///////////////////////////////
        mutex.unlock(); // UNLOCKING //
        ///////////////////////////////

    } else {
        if(needsReconnect) {
            if(reconnectCount < maxReconnects) {
                if(now > nextRetry) {
                    ofLogVerbose("ofxIpVideoGrabber") << "[" << getCameraName() << "] attempting reconnection " << reconnectCount << "/" << maxReconnects;
                    connect();
                } else {
                    ofLogVerbose("ofxIpVideoGrabber") << "[" << getCameraName() << "] retry connect in " << (nextRetry - now) << " ms.";
                }
            } else {
                if(!connectionFailure) {
                    ofLogError("ofxIpVideoGrabber") << "["<< getCameraName() << "] Connection retries exceeded, connection connection failed.  Call ::reset() to try again.";
                    connectionFailure = true;
                }
            }
        }
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::connect() {
    if(!isConnected()) {
        // start the thread.
        
        lastValidBitrateTime = 0;
        
        currentBitRate   = 0.0;
        currentFrameRate = 0.0;
        nBytesReceived   = 0;
        nFramesReceived  = 0;

        startThread(true, false);   // blocking, verbose
    } else {
        ofLogWarning("ofxIpVideoGrabber")  << "[" << getCameraName() << "]: Already connected.  Disconnect first.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::waitForDisconnect() {
    if(isConnected()) {
        waitForThread(true); // close it all down (politely) -- true sets running flag
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::disconnect() {
    if(isConnected()) {
        stopThread();
    } else {
        ofLogWarning("ofxIpVideoGrabber")  << "[" << getCameraName() << "]: Not connected.  Connect first.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::close() {
    disconnect();
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::reset() {
    reconnectCount = 0;
    connectionFailure = false;
    waitForDisconnect();
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::isConnected() {
    return isThreadRunning();
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::hasConnectionFailed() const {
    return connectionFailure;
}

//--------------------------------------------------------------
unsigned long ofxIpVideoGrabber::getReconnectTimeout() const {
    return reconnectTimeout;
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::getNeedsReconnect() const {
    return needsReconnect;
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::getAutoReconnect() const {
    return autoReconnect;
}

//--------------------------------------------------------------
unsigned long ofxIpVideoGrabber::getReconnectCount() const {
    return reconnectCount;
}

//--------------------------------------------------------------
unsigned long ofxIpVideoGrabber::getMaxReconnects() const {
    return maxReconnects;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setMaxReconnects(unsigned long num) {
    maxReconnects = num;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setDefaultBoundaryMarker(const string& _defaultBoundaryMarker) {
    defaultBoundaryMarker = _defaultBoundaryMarker;
    if(isConnected()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New boundary info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getDefaultBoundaryMarker() {
    return defaultBoundaryMarker;
}


//--------------------------------------------------------------
void ofxIpVideoGrabber::threadedFunction(){
    
    char* cBuf = new char[BUF_LEN];

    ofBuffer buffer;
    
    HTTPClientSession session;
    
    ///////////////////////////
	mutex.lock(); // LOCKING //
    ///////////////////////////
    
    connectTime_a = ofGetSystemTime(); // start time

    elapsedTime_a = 0;
    nBytes_a      = 0;
    nFrames_a     = 0;
    
    needsReconnect = false;
    reconnectCount++;

    ///////////////////////
    // configure session //
    ///////////////////////
    
    // configure proxy

    if(bUseProxy) {
        if(!proxyHost.empty()) {
            session.setProxy(proxyHost);
            session.setProxyPort(proxyPort);
            if(!proxyUsername.empty()) session.setProxyUsername(proxyUsername);
            if(!proxyPassword.empty()) session.setProxyPassword(proxyPassword);
        } else {
            ofLogError("ofxIpVideoGrabber") << "Attempted to use web proxy, but proxy host was empty.  Continuing without proxy.";
        }
    }
    
    // configure destination
    session.setHost(uri.getHost());
    session.setPort(uri.getPort());
    
    // basic session info
    session.setKeepAlive(true);
    session.setTimeout(Poco::Timespan(sessionTimeout / 1000,0)); // 20 sececond timeout
    
    // add trailing slash if nothing is there
    string path(uri.getPathAndQuery());
    if (path.empty()) path = "/";

    // create our header request
    HTTPRequest request(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1); // 1.1 for persistent connections

    // do basic access authentication if needed
    if(!username.empty() || !password.empty()) {
        HTTPBasicCredentials credentials;
        credentials.setUsername(username);
        credentials.setPassword(password);
        credentials.authenticate(request);
    }
    
    // send cookies if needed (sometimes, we send our authentication as cookies)
    if(!cookies.empty()) request.setCookies(cookies);

    ///////////////////////////////
	mutex.unlock(); // UNLOCKING //
    ///////////////////////////////

    
    /////////////////////////
    // start communicating //
    /////////////////////////

    HTTPResponse response; // the empty data structure to fill with the response headers

    try {
        
        // sendn the http request.
        ostream& requestOutputStream = session.sendRequest(request);
        
        // check the return stream for success.
        if(requestOutputStream.bad() || requestOutputStream.fail()) {
            throw Exception("Error communicating with server during sendRequest.");
        }
        
        // prepare to receive the response
        istream& responseInputStream = session.receiveResponse(response); // invalidates requestOutputStream
        
        // get and test the status code
        HTTPResponse::HTTPStatus status = response.getStatus();
        
        if(status != HTTPResponse::HTTP_OK) {
            throw Exception("Invalid HTTP Reponse : " + response.getReasonForStatus(status),status);
        }
        
        NameValueCollection nvc;
        string contentType;
        string boundaryMarker;
        HTTPResponse::splitParameters(response.getContentType(), contentType, nvc);
        boundaryMarker = nvc.get("boundary",defaultBoundaryMarker); 
        
        if(icompare(string("--"), boundaryMarker.substr(0,2)) != 0) {
            boundaryMarker = "--" + boundaryMarker; // prepend the marker
        }
        
        ContentStreamMode mode = MODE_HEADER;
        
        int c = 0;
        
        bool resetBuffer = false;
        
        // mjpeg params
        int contentLength = 0;
        string boundaryType;
        
        
        while( isThreadRunning() ){
            
            if(!responseInputStream.fail() && !responseInputStream.bad()) {
                
                resetBuffer = false;
                responseInputStream.get(cBuf[c]); // put a byte in the buffer
                
                mutex.lock();
                nBytes_a++; // count bytes
                mutex.unlock();
                
                if(c > 0) {
                    if(mode == MODE_HEADER && cBuf[c-1] == '\r' && cBuf[c] == '\n') {
                        if(c > 1) {
                            cBuf[c-1] = 0; // NULL terminator
                            string line(cBuf); // make a string object
                            
                            vector<string> keyValue = ofSplitString(line,":", true); // split it (try)
                            if(keyValue.size() > 1) { // a param!
                                
                                string& key   = keyValue[0]; // reference to trimmed key for better readability
                                string& value = keyValue[1]; // reference to trimmed val for better readability
                                
                                if(icompare(string("content-length"), key) == 0) {
                                    contentLength = ofToInt(value);
                                    // TODO: we don't currently use content length, but could
                                } else if(icompare(string("content-type"), key) == 0) {
                                    boundaryType = value;
                                } else {
                                    ofLogVerbose("ofxIpVideoGrabber") << "additional header " << key << "=" << value << " found.";
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
                            if( c >= MIN_JPEG_SIZE) { // some cameras send 2+ EOIs in a row, with no valid bytes in between

                                ///////////////////////////////
                                mutex.lock();     // LOCKING //
                                ///////////////////////////////

                                // get the back image (ci^1)
                                if(image[ci^1].loadImage(buffer)) {
                                    isBackBufferReady = true;
                                    nFrames_a++; // incrase frame cout
                                } else {
                                    ofLogError("ofxIpVideoGrabber") << "ofImage could not load the curent buffer, continuing.";
                                }
                                
                                ///////////////////////////////
                                mutex.unlock(); // UNLOCKING //
                                ///////////////////////////////
                                    
                            } else {}
                            mode = MODE_HEADER;
                            resetBuffer = true;
                        } else {}
                    } else if(mode == MODE_JPEG) {
                    } else {}
                } else {}
                
                
                // check for buffer overflow
                if(c >= BUF_LEN) {
                    resetBuffer = true;
                    ofLogError("ofxIpVideoGrabber") << "[" + getCameraName() +"]: buffer overflow, resetting stream.";
                }
                
                // has anyone requested a buffer reset?
                if(resetBuffer) {
                    c = 0;
                } else {
                    c++;
                }
                
            } else { // end stream check
                throw Exception("ResponseInputStream failed or went bad -- it was probably interrupted.");
            }
            
        } // end while

    } catch (Exception& e) {
        mutex.lock();
        needsReconnect = true;
        nextRetry = ofGetSystemTime() + retryDelay;
        mutex.unlock();
        ofLogError("ofxIpVideoGrabber") << "Exception : [" << getCameraName() << "]: " <<  e.displayText();
    }


    // clean up the session
    session.reset();

    // clean up the buffer
    delete[] cBuf;
    cBuf = NULL;

    // fall off the end of the loop ...
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
unsigned long ofxIpVideoGrabber::getNumFramesReceived() {
    if(isConnected()) {
        return nFramesReceived;
    } else {
        return 0;
    }
}

//--------------------------------------------------------------
unsigned long ofxIpVideoGrabber::getNumBytesReceived() {
    if(isConnected()) {
        return nBytesReceived;
    } else {
        return 0;
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
    if(isConnected()) {
        return  currentFrameRate;
    } else {
        return 0.0f;
    }
}
    
//--------------------------------------------------------------
float ofxIpVideoGrabber::getBitRate() {
    if(isConnected()) {
        return currentBitRate;
    } else {
        return 0.0f;
    }
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
    if(isConnected()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New cookie info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::eraseCookie(const string& key) {
    cookies.erase(key);
    if(isConnected()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New cookie info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getCookie(const string& key) const {
    return cookies.get(key);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setUsername(const string& _username) {
    username = _username;
    if(isConnected()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New authentication info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setPassword(const string& _password) {
    password = _password;
    if(isConnected()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New authentication info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getUsername() const {
    return username;
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getPassword() const {
    return password;
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

//--------------------------------------------------------------
URI ofxIpVideoGrabber::getPocoURI() const {
    return uri;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(const string& _uri) {
    URI uri(_uri);
    setURI(uri);
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New URI will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setURI(const URI& _uri) {
    uri = _uri;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New URI will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setUseProxy(bool useProxy) {
    bUseProxy = useProxy;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyUsername(const string& username) {
    proxyUsername = username;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPassword(const string& password) {
    proxyPassword = password;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyHost(const string& host) {
    proxyHost = host;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setProxyPort(Poco::UInt16 port) {
    proxyPort = port;
    if(isThreadRunning()) {
        ofLogWarning("ofxIpVideoGrabber") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}

//--------------------------------------------------------------
bool ofxIpVideoGrabber::getUseProxy() const {
    return bUseProxy;
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyUsername() const {
    return proxyUsername;
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyPassword() const {
    return proxyPassword;
}

//--------------------------------------------------------------
string ofxIpVideoGrabber::getProxyHost() const {
    return proxyHost;
}

//--------------------------------------------------------------
UInt16 ofxIpVideoGrabber::getProxyPort() const {
    return proxyPort;
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::imageResized(int width, int height) {
    ofResizeEventArgs resizeEvent;
    resizeEvent.width = width;
    resizeEvent.height = height;
    ofNotifyEvent(videoResized, resizeEvent, this);
}

//--------------------------------------------------------------
void ofxIpVideoGrabber::setReconnectTimeout(unsigned long ms) {
    reconnectTimeout = ms;
}

