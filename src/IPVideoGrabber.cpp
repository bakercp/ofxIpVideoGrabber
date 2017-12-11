//
// Copyright (c) 2009 Christopher Baker <https://christopherbaker.net>
//
// SPDX-License-Identifier:    MIT
//


#include "IPVideoGrabber.h"
#include "Poco/UTF8String.h"
#include "Poco/Net/HTTPBasicCredentials.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "ofImage.h"


namespace ofx {
namespace Video {


enum ContentStreamMode
{
    MODE_HEADER = 0,
    MODE_JPEG = 1
};

const std::size_t IPVideoGrabber::MIN_JPEG_SIZE = 134; // minimum number of bytes for a valid jpeg

// jpeg starting and ending bytes
const std::size_t IPVideoGrabber::BUF_LEN = 4*512000; // 8 * 65536 = 512 kB

const char IPVideoGrabber::JFF = 0xFF;
const char IPVideoGrabber::SOI = 0xD8;
const char IPVideoGrabber::EOI = 0xD9;


IPVideoGrabber::IPVideoGrabber():
    _isConnected(false),
    defaultBoundaryMarker_a("--myboundary"),
    cameraName_a(""),
    username_a(""),
    password_a(""),
    bUseProxy_a(false),
    proxyUsername_a(""),
    proxyPassword_a(""),
    proxyHost_a("127.0.0.1"),
    proxyPort_a(Poco::Net::HTTPSession::HTTP_PORT),
    ci(0),
    img(std::make_shared<ofImage>()),
    isNewFrameLoaded(false),
    isBackBufferReady_a(false),
    connectTime_a(0),
    elapsedTime_a(0),
    nBytes_a(0),
    nFrames_a(0),
    currentBitRate(0.0),
    currentFrameRate(0.0),
    minBitrate(8),
    lastValidBitrateTime(0),
    reconnectTimeout(5000),
    autoRetryDelay_a(1000), // at least 1 second retry delay
    nextAutoRetry_a(0),
    connectionFailure(false),
    needsReconnect_a(false),
    autoReconnect(true),
    reconnectCount_a(0),
    maxReconnects(20),
    sessionTimeout(2000)
{
    img->allocate(1, 1, OF_IMAGE_COLOR); // allocate something so it won't throw errors
    img->setColor(0, 0, ofColor(0));

    // THIS IS EXTREMELY IMPORTANT.
    // To shut down the thread cleanly, we cannot allow openFrameworks to
    // de-init FreeImage before this thread has completed any current image
    // loads. Thus we need to make sure that this->waitForDisconnect() is called
    // before the main oF loop calls ofImage::ofCloseFreeImage(). This can often
    // result in slight lags during shutdown, especially when multiple threads
    // are running, but it prevents crashes. To do that, we simply need to
    // register an ofEvents.exit listener. Alternatively, each thread could
    // instantiate its own instance of FreeImage, but that seems memory
    // inefficient. Thus, the shutdown lag for now.

#if OF_VERSION_MAJOR < 1 && OF_VERSION_MINOR >= 10
    _exitListener = ofEvents().exit.newListener(this, &IPVideoGrabber::exit);
#endif

}


IPVideoGrabber::~IPVideoGrabber()
{
    // N.B. In most cases, IPVideoGrabber::exit() will be called before
    // the program ever makes it into this destructor.
    waitForDisconnect();
    ofLogVerbose("IPVideoGrabber::~IPVideoGrabber") << "Destroyed.";
}


void IPVideoGrabber::setup(const IpVideoGrabberSettings& settings)
{
    setCameraName(settings.getName());
    setUsername(settings.getUsername());
    setPassword(settings.getPassword());
    setURI(settings.getURL());
}


void IPVideoGrabber::exit(ofEventArgs & a) {
    ofLogVerbose("IPVideoGrabber::exit") << "exit() called on [" << getCameraName() << "] -- cleaning up and exiting.";
    waitForDisconnect();
}


void IPVideoGrabber::update(ofEventArgs& a)
{
    update();
}


void IPVideoGrabber::update()
{
    isNewFrameLoaded = false;

    uint64_t now = ofGetSystemTimeMillis();
    
    std::string cName = getCameraName(); // consequence of scoped locking
    
    if (_isConnected)
    {
        ///////////////////////////////
        mutex.lock();     // LOCKING //
        ///////////////////////////////

        // do opengl texture loading from the main update thread

        elapsedTime_a = (connectTime_a == 0) ? 0 : (now - connectTime_a);

        if (isBackBufferReady_a) 
        {
            ci^=1; // swap buffers (ci^1) was just filled in the thread
            
            int newW = image_a[ci].getWidth();    // new buffer
            int newH = image_a[ci].getHeight();   // new buffer
            int oldW = image_a[ci^1].getWidth();  // old buffer
            int oldH = image_a[ci^1].getHeight(); // old buffer
            
            // send out an alert to anyone who cares.
            // on occasion an mjpeg image stream size can be changed on the fly
            // without our knowledge.  so tell people about it.
            
            // remove lock for a moment to send out the message
            // we must remove the lock, so any calls made to THIS object
            // within the callback will be able to get their locks via
            // synchronized methods.
            mutex.unlock();
            
            if (newW != oldW || newH != oldH) imageResized(newW, newH);
            
            // lock it again.
            mutex.lock();
            
            // get a pixel ref for the image that was just loaded in the thread
            
            img = std::make_shared<ofImage>();
            img->setFromPixels(image_a[ci]);
            
            isNewFrameLoaded = true; // we only set this to true.  this setup is analogous to
                                     // has new pixels in ofVideo
            
            isBackBufferReady_a = false;
        }
        
        if (elapsedTime_a > 0)
        {
            currentFrameRate = ( float(nFrames_a) )       / (elapsedTime_a / (1000.0f)); // frames per second
            currentBitRate   = ( float(nBytes_a ) / 8.0f) / (elapsedTime_a / (1000.0f)); // bits per second
        }
        else
        {
            currentFrameRate = 0;
            currentBitRate   = 0;
        }
        
        if (currentBitRate > minBitrate)
        {
            lastValidBitrateTime = elapsedTime_a;
        }
        else
        {
            if ((elapsedTime_a - lastValidBitrateTime) > reconnectTimeout)
            {
                ofLogVerbose("IPVideoGrabber::update") << "["<< cName << "] Disconnecting because of slow bitrate.";
                needsReconnect_a = true;
            }
            else
            {
                // slowed below min bitrate, waiting to see if we are too low for long enought to merit a reconnect
            }
        }
    
        ///////////////////////////////
        mutex.unlock(); // UNLOCKING //
        ///////////////////////////////
    }
    else
    {
        
        if (getNeedsReconnect())
        {
            if (maxReconnects < 0 || getReconnectCount() < maxReconnects)
            {
                uint64_t nar = getNextAutoRetryTime();

                if (now >= getNextAutoRetryTime())
                {
                    ofLogVerbose("IPVideoGrabber::update") << "[" << cName << "] attempting reconnection " << getReconnectCount() << "/" << maxReconnects;
                    connect();
                }
                else
                {
                    ofLogVerbose("IPVideoGrabber::update") << "[" << cName << "] retry connect in " << (nar - now) << " ms.";
                }
            }
            else
            {
                if (!connectionFailure)
                {
                    ofLogError("IPVideoGrabber::update") << "["<< cName << "] Connection retries exceeded, connection connection failed.  Call ::reset() to try again.";
                    connectionFailure = true;
                }
            }
        }
    }
    
}


void IPVideoGrabber::connect()
{
    if (!_isConnected)
    {
        ofLogNotice("IPVideoGrabber::connect")  << "[" << getCameraName() << "]: Connecting!";

        // start the thread.
        
        lastValidBitrateTime = 0;
        
        currentBitRate   = 0.0;
        currentFrameRate = 0.0;

        _isConnected = true;
        _thread = std::thread(&IPVideoGrabber::threadedFunction, this);

    }
    else
    {
        ofLogWarning("IPVideoGrabber::connect")  << "[" << getCameraName() << "]: Already connected.  Disconnect first.";
    }
}


void IPVideoGrabber::waitForDisconnect()
{
    ofLogNotice("IPVideoGrabber::waitForDisconnect")  << "Waiting for disconnect ... ";

    disconnect();

    try
    {
        _thread.join();
    }
    catch (const std::exception& exc)
    {
        ofLogVerbose("IPVideoGrabber::waitForDisconnect")  << "Joining failed: " << exc.what();
    }
}


void IPVideoGrabber::disconnect()
{
    if (_isConnected)
    {
        _isConnected = false;
    }
    else
    {
        ofLogNotice("IPVideoGrabber::disconnect")  << "[" << getCameraName() << "]: Not connected.  Connect first.";
    }
}


void IPVideoGrabber::close()
{
    disconnect();
}


void IPVideoGrabber::reset()
{
    mutex.lock();
    reconnectCount_a = 0;
    connectionFailure = false;
    mutex.unlock();
    //waitForDisconnect();
}


bool IPVideoGrabber::isConnected() const
{
    return _isConnected;
}


bool IPVideoGrabber::hasConnectionFailed() const
{
    return connectionFailure;
}


uint64_t IPVideoGrabber::getReconnectTimeout() const
{
    return reconnectTimeout;
}


bool IPVideoGrabber::getNeedsReconnect() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return needsReconnect_a;
}


bool IPVideoGrabber::getAutoReconnect() const
{
    return autoReconnect;
}


int IPVideoGrabber::getReconnectCount() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return reconnectCount_a;
}


int IPVideoGrabber::getMaxReconnects() const
{
    return maxReconnects;
}


void IPVideoGrabber::setMaxReconnects(int num)
{
    maxReconnects = num;
}


uint64_t IPVideoGrabber::getAutoRetryDelay() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return autoRetryDelay_a;
}


void IPVideoGrabber::setAutoRetryDelay(uint64_t delay_ms)
{
    std::unique_lock<std::mutex> lock(mutex);
    autoRetryDelay_a = delay_ms;
}


uint64_t IPVideoGrabber::getNextAutoRetryTime() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return nextAutoRetry_a;
}


uint64_t IPVideoGrabber::getTimeTillNextAutoRetry() const
{
    std::unique_lock<std::mutex> lock(mutex);

    auto now = ofGetSystemTimeMillis();

    if (nextAutoRetry_a == 0 or now >= nextAutoRetry_a)
    {
        return 0;
    }
    else
    {
        return nextAutoRetry_a - ofGetSystemTimeMillis();
    }
}


void IPVideoGrabber::setDefaultBoundaryMarker(const std::string& _defaultBoundaryMarker)
{
    std::unique_lock<std::mutex> lock(mutex);

    defaultBoundaryMarker_a = _defaultBoundaryMarker;

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setDefaultBoundaryMarker") << "Session currently active.  New boundary info will be applied on the next connection.";
    }
}


std::string IPVideoGrabber::getDefaultBoundaryMarker() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return defaultBoundaryMarker_a;
}


void IPVideoGrabber::threadedFunction()
{
    Poco::Buffer<char> cBuf(BUF_LEN);

    ofBuffer buffer;

    Poco::Net::HTTPClientSession session;
    
    ///////////////////////////
	mutex.lock(); // LOCKING //
    ///////////////////////////
        
    connectTime_a = ofGetSystemTimeMillis(); // start time
    nextAutoRetry_a = 0;
    
    elapsedTime_a = 0;
    nBytes_a      = 0;
    nFrames_a     = 0;
    
    needsReconnect_a = false;
    reconnectCount_a++;

    ///////////////////////
    // configure session //
    ///////////////////////
    
    // configure proxy

    if (bUseProxy_a)
    {
        if (!proxyHost_a.empty())
        {
            session.setProxy(proxyHost_a);
            session.setProxyPort(proxyPort_a);
            if (!proxyUsername_a.empty()) session.setProxyUsername(proxyUsername_a);
            if (!proxyPassword_a.empty()) session.setProxyPassword(proxyPassword_a);
        }
        else
        {
            ofLogError("IPVideoGrabber") << "Attempted to use web proxy, but proxy host was empty.  Continuing without proxy.";
        }
    }
        
    // configure destination
    session.setHost(uri_a.getHost());
    session.setPort(uri_a.getPort());
    
    // basic session info
    session.setKeepAlive(true);
    session.setTimeout(Poco::Timespan(sessionTimeout * Poco::Timespan::MILLISECONDS));
    
    // add trailing slash if nothing is there
    std::string path(uri_a.getPathAndQuery());
    
    if (path.empty()) path = "/";

    // create our header request
    // TODO: add SSL via ofxSSL
    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1); // 1.1 for persistent connections

    // do basic access authentication if needed
    if (!username_a.empty() || !password_a.empty())
    {
        Poco::Net::HTTPBasicCredentials credentials;
        credentials.setUsername(username_a);
        credentials.setPassword(password_a);
        credentials.authenticate(request);
    }
    
    // send cookies if needed (sometimes, we send our authentication as cookies)
    if (!cookies.empty()) request.setCookies(cookies);

    ///////////////////////////////
	mutex.unlock(); // UNLOCKING //
    ///////////////////////////////
    
    /////////////////////////
    // start communicating //
    /////////////////////////

    Poco::Net::HTTPResponse response; // the empty data structure to fill with the response headers

    try
    {

        // sendn the http request.
        std::ostream& requestOutputStream = session.sendRequest(request);
        
        // check the return stream for success.
        if (requestOutputStream.bad() || requestOutputStream.fail())
        {
            throw Poco::Exception("Error communicating with server during sendRequest.");
        }
        
        // prepare to receive the response
        std::istream& responseInputStream = session.receiveResponse(response); // invalidates requestOutputStream
        
        // get and test the status code
        Poco::Net::HTTPResponse::HTTPStatus status = response.getStatus();
        
        if (status != Poco::Net::HTTPResponse::HTTP_OK)
        {
            throw Poco::Exception("Invalid HTTP Reponse : " + response.getReasonForStatus(status), status);
        }
        
        Poco::Net::NameValueCollection nvc;
        std::string contentType;
        std::string boundaryMarker;
        Poco::Net::HTTPResponse::splitParameters(response.getContentType(), contentType, nvc);
    
        boundaryMarker = nvc.get("boundary",getDefaultBoundaryMarker()); // we call the getter here b/c not in critical section
        
        if (Poco::UTF8::icompare(std::string("--"), boundaryMarker.substr(0,2)) != 0)
        {
            boundaryMarker = "--" + boundaryMarker; // prepend the marker
        }
        
        ContentStreamMode mode = MODE_HEADER;
        
        std::size_t c = 0;
        
        bool resetBuffer = false;
        
        // mjpeg params
        // int contentLength = 0;
        std::string boundaryType;
        
        while (_isConnected)
        {
            if (!responseInputStream.fail() && !responseInputStream.bad())
            {
                
                resetBuffer = false;
                responseInputStream.get(cBuf[c]); // put a byte in the buffer
                
                mutex.lock();
                nBytes_a++; // count bytes
                mutex.unlock();
                
                if (c > 0)
                {
                    if (mode == MODE_HEADER && cBuf[c-1] == '\r' && cBuf[c] == '\n')
                    {
                        if (c > 1)
                        {
                            cBuf[c-1] = '\0'; // NULL terminator
                            std::string line(cBuf.begin()); // make a string object
                            
                            std::vector<std::string> keyValue = ofSplitString(line,":", true); // split it (try)
                            if (keyValue.size() > 1)
                            { // a param!
                                
                                std::string& key   = keyValue[0]; // reference to trimmed key for better readability
                                std::string& value = keyValue[1]; // reference to trimmed val for better readability
                                
                                if (Poco::UTF8::icompare(std::string("content-length"), key) == 0)
                                {
                                    // contentLength = ofToInt(value);
                                    // TODO: we don't currently use content length, but could
                                }
                                else if (Poco::UTF8::icompare(std::string("content-type"), key) == 0)
                                {
                                    boundaryType = value;
                                }
                                else
                                {
                                    ofLogVerbose("IPVideoGrabber") << "additional header " << key << "=" << value << " found.";
                                }
                            }
                            else
                            {
                                if (Poco::UTF8::icompare(boundaryMarker,line) == 0)
                                {
                                    mode = MODE_HEADER;
                                }
                                else
                                {
                                    // just a new line
                                }
                                // this is where line == "--myboundary"
                            }
                        }
                        else
                        {
                            // just waiting for at least two bytes
                        }
                        
                        resetBuffer = true; // reset upon new line
                        
                    }
                    else if (cBuf[c-1] == JFF)
                    {
                        if (cBuf[c] == SOI)
                        {
                            mode = MODE_JPEG;
                        }
                        else if (cBuf[c] == EOI)
                        {
                            buffer = ofBuffer(cBuf.begin(), c + 1);

                            if (c >= MIN_JPEG_SIZE)
                            { // some cameras send 2+ EOIs in a row, with no valid bytes in between
    
                                ///////////////////////////////
                                mutex.lock();     // LOCKING //
                                ///////////////////////////////

								ofPixels pix;

								bool result = ofLoadImage(pix, buffer);

                                // get the back image (ci^1)
                                if (result)
                                {
									image_a[ci^1] = pix;
                                    isBackBufferReady_a = true;
                                    nFrames_a++; // incrase frame cout
                                }
                                else
                                {
                                    ofLogError("IPVideoGrabber") << "ofImage could not load the curent buffer, continuing.";
                                }
                                
                                ///////////////////////////////
                                mutex.unlock(); // UNLOCKING //
                                ///////////////////////////////
                                    
                            }
                            else
                            {
                                //
                            }
                            mode = MODE_HEADER;
                            resetBuffer = true;
                        }
                        else
                        {
                        }
                    }
                    else if (mode == MODE_JPEG)
                    {
                    }
                    else
                    {
                    }
                }
                else
                {
                }
                
                // check for buffer overflow
                if (c >= BUF_LEN)
                {
                    resetBuffer = true;
                    ofLogError("IPVideoGrabber") << "[" + getCameraName() +"]: buffer overflow, resetting stream.";
                }
                
                // has anyone requested a buffer reset?
                if (resetBuffer)
                {
                    c = 0;
                }
                else
                {
                    c++;
                }
                
            }
            else
            { // end stream check
                throw Poco::Exception("ResponseInputStream failed or went bad -- it was probably interrupted.");
            }
            
        } // end while

    }
    catch (const Poco::Exception& e)
    {
        mutex.lock();
        needsReconnect_a = true;
//        _isConnected = false;
        nextAutoRetry_a = ofGetSystemTimeMillis() + autoRetryDelay_a;
        mutex.unlock();
        ofLogError("IPVideoGrabber") << "Exception : [" << getCameraName() << "]: " <<  e.displayText();
    }
    catch (...)
    {
        mutex.lock();
        needsReconnect_a = true;
//        _isConnected = false;
        nextAutoRetry_a = ofGetSystemTimeMillis() + autoRetryDelay_a;
        mutex.unlock();
        ofLogError("IPVideoGrabber") << "Unknown exception.";
    }


    // clean up the session
    session.reset();

    // std::cout << "-----------------------------fell off end of loop ... needs reconnect ? " << needsReconnect_a << " is connected? " << _isConnected << std::endl;
    // fall off the end of the loop ...
}


bool IPVideoGrabber::isFrameNew() const
{
    return isNewFrameLoaded;
}


bool IPVideoGrabber::isInitialized() const
{
	return true;
}


bool IPVideoGrabber::setPixelFormat(ofPixelFormat pixelFormat)
{
	return false;
}


ofPixelFormat IPVideoGrabber::getPixelFormat() const
{
	return img->getPixels().getPixelFormat();
}


ofPixels& IPVideoGrabber::getPixels()
{
    return img->getPixels();
}


const ofPixels& IPVideoGrabber::getPixels() const
{
    return img->getPixels();
}


std::shared_ptr<ofImage> IPVideoGrabber::getFrame()
{
    return img;
}


ofTexture& IPVideoGrabber::getTexture()
{
	return img->getTexture();
}


const ofTexture& IPVideoGrabber::getTexture() const
{
    return img->getTexture();
}


void IPVideoGrabber::setUseTexture(bool useTexture)
{
    ofLogWarning("IPVideoGrabber::setUseTexture") << "This is always true.";
}


bool IPVideoGrabber::isUsingTexture() const
{
	return true;
}


std::vector<ofTexture>& IPVideoGrabber::getTexturePlanes()
{
	texPlanes.clear();
	texPlanes.push_back(getTexture());
	return texPlanes;
}


const std::vector<ofTexture>& IPVideoGrabber::getTexturePlanes() const
{
	texPlanes.clear();
	texPlanes.push_back(getTexture());
	return texPlanes;
}


float IPVideoGrabber::getWidth() const
{
    return img->getWidth();
}


float IPVideoGrabber::getHeight() const
{
    return img->getHeight();
}


uint64_t IPVideoGrabber::getNumFramesReceived() const
{
    if (_isConnected)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return nFrames_a;
    }
    else
    {
        return 0;
    }
}


uint64_t IPVideoGrabber::getNumBytesReceived() const
{
    if (_isConnected)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return nBytes_a;
    }
    else
    {
        return 0;
    }
}


void IPVideoGrabber::draw(float x, float y) const
{
    img->draw(x, y);
}


void IPVideoGrabber::draw(float x, float y, float width, float height) const
{
    img->draw(x, y, width, height);
}


void IPVideoGrabber::draw(const glm::vec3& point) const
{
    draw(point.x, point.y);
}


void IPVideoGrabber::draw(const ofRectangle & rect) const
{
    draw(rect.x, rect.y, rect.width, rect.height); 
}


void IPVideoGrabber::setAnchorPercent(float xPct, float yPct)
{
    img->setAnchorPercent(xPct,yPct);
}


void IPVideoGrabber::setAnchorPoint(float x, float y)
{
    img->setAnchorPoint(x,y);
}


void IPVideoGrabber::resetAnchor()
{
    img->resetAnchor();
}


float IPVideoGrabber::getFrameRate() const
{
    if (_isConnected)
    {
        return currentFrameRate;
    }
    else
    {
        return 0.0f;
    }
}
    

float IPVideoGrabber::getBitRate() const
{
    if (_isConnected)
    {
        return currentBitRate;
    }
    else
    {
        return 0.0f;
    }
}


void IPVideoGrabber::setCameraName(const std::string& _cameraName)
{
    std::unique_lock<std::mutex> lock(mutex);
    cameraName_a = _cameraName;
}


std::string IPVideoGrabber::getCameraName() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (cameraName_a.empty())
    {
        return uri_a.toString();
    }
    else
    {
        return cameraName_a;
    }
}


void IPVideoGrabber::setCookie(const std::string& key, const std::string& value)
{
    cookies.add(key, value);

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setCookie") << "Session currently active.  New cookie info will be applied on the next connection.";
    }
}


void IPVideoGrabber::eraseCookie(const std::string& key)
{
    cookies.erase(key);

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::eraseCookie") << "Session currently active.  New cookie info will be applied on the next connection.";
    }
}


std::string IPVideoGrabber::getCookie(const std::string& key) const
{
    std::unique_lock<std::mutex> lock(mutex);
    return cookies.get(key);
}


void IPVideoGrabber::setUsername(const std::string& _username)
{
    mutex.lock();
    username_a = _username;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setUsername") << "Session currently active.  New authentication info will be applied on the next connection.";
    }
}


void IPVideoGrabber::setPassword(const std::string& _password)
{
    mutex.lock();
    password_a = _password;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setPassword") << "Session currently active.  New authentication info will be applied on the next connection.";
    }
}


std::string IPVideoGrabber::getUsername() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return username_a;
}


std::string IPVideoGrabber::getPassword() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return password_a;
}


std::string IPVideoGrabber::getURI() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return uri_a.toString();
}


std::string IPVideoGrabber::getHost() const
{
    return uri_a.getHost();
}


std::string IPVideoGrabber::getQuery() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return uri_a.getQuery();
}


uint16_t IPVideoGrabber::getPort() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return uri_a.getPort();
}


std::string IPVideoGrabber::getFragment() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return uri_a.getFragment();
}


Poco::URI IPVideoGrabber::getPocoURI() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return uri_a;
}


void IPVideoGrabber::setURI(const std::string& _uri)
{
    mutex.lock();
    uri_a = Poco::URI(_uri);
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setURI") << "Session currently active.  New URI will be applied on the next connection.";
    }
}


void IPVideoGrabber::setURI(const Poco::URI& _uri)
{
    mutex.lock();
    uri_a = _uri;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setURI") << "Session currently active.  New URI will be applied on the next connection.";
    }
}


void IPVideoGrabber::setUseProxy(bool useProxy)
{
    mutex.lock();
    bUseProxy_a = useProxy;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setUseProxy") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}


void IPVideoGrabber::setProxyUsername(const std::string& username)
{
    mutex.lock();
    proxyUsername_a = username;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setProxyUsername") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}


void IPVideoGrabber::setProxyPassword(const std::string& password)
{
    mutex.lock();
    proxyPassword_a = password;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setProxyPassword") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}


void IPVideoGrabber::setProxyHost(const std::string& host)
{
    mutex.lock();
    proxyHost_a = host;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setProxyHost") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}


void IPVideoGrabber::setProxyPort(uint16_t port)
{
    mutex.lock();
    proxyPort_a = port;
    mutex.unlock();

    if (_isConnected)
    {
        ofLogWarning("IPVideoGrabber::setProxyPort") << "Session currently active.  New proxy info will be applied on the next connection.";
    }
}


bool IPVideoGrabber::getUseProxy() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return bUseProxy_a;
}


std::string IPVideoGrabber::getProxyUsername() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return proxyUsername_a;
}


std::string IPVideoGrabber::getProxyPassword() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return proxyPassword_a;
}


std::string IPVideoGrabber::getProxyHost() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return proxyHost_a;
}


Poco::UInt16 IPVideoGrabber::getProxyPort() const
{
    std::unique_lock<std::mutex> lock(mutex);
    return proxyPort_a;
}


void IPVideoGrabber::imageResized(int width, int height)
{
    ofResizeEventArgs resizeEvent;
    resizeEvent.width = width;
    resizeEvent.height = height;
    ofNotifyEvent(videoResized, resizeEvent, this);
}


void IPVideoGrabber::setReconnectTimeout(uint64_t ms)
{
    reconnectTimeout = ms;
}


} } // namespace ofx::Video
