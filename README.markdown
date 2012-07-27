ofxIpVideoGrabber
==================

Copyright (c) 2011, 2012 Christopher Baker <http://christopherbaker.net>

MIT License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.

Description
-----------

ofxIpVideoGrabber is an Open Frameworks addon for the to capture video streams from IP Cameras that use the mjpeg streaming protocol.  Public cameras can be found with a google search like this:

http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22

OpenFrameworks is a cross platform open source toolkit for creative coding in C++.

[http://www.openframeworks.cc/](http://www.openframeworks.cc/)

Installation
------------

To use ofxIpVideoGrabber, first you need to download and install [Open Frameworks](https://github.com/openframeworks/openFrameworks).

To get a copy of the repository you can download the source from [http://github.com/bakercp/ofxIpVideoGrabber](http://github.com/bakercp/ofxIpVideoGrabber) or, alternatively, you can use git clone:

<pre>
git clone git://github.com/bakercp/ofxIpVideoGrabber.git
</pre>

The addon should sit in `openFrameworks/addons/ofxIpVideoGrabber/`.

#### Which version to use?

ofxIpVideoGrabber has been tested with the latest development version of Open Frameworks.

Dependencies
------------
The library itself does not require any dependencies.  The example uses [ofxUtils](https://github.com/bakercp/ofxUtils).

Compatability
-------------

Edimax IC-3005
Axis Cameras
and others.

Alternatives
------------

If you are interested in using these cameras with Processing or Max/MSP/Jitter, you can find code here: https://github.com/themaw/mawLib/tree/master/src/mxj/trunk/mawLib-mxj/src/maw/jit/ipcam.  It is part of mawLib and includes pan/tilt, etc control code.

