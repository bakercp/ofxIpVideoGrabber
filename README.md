# ofxIpVideoGrabber

![Screenshot](https://github.com/bakercp/ofxIpVideoGrabber/raw/master/screen.png)

# Description

ofxIpVideoGrabber is an Open Frameworks addon used to capture video streams from IP Cameras that use the mjpeg streaming protocol.  Public cameras can be found with a google search like this:

http://www.google.com/search?q=inurl%3A%22axis-cgi%2Fmjpg%22

See others examples below.

OpenFrameworks is a cross platform open source toolkit for creative coding in C++.

[http://www.openframeworks.cc/](http://www.openframeworks.cc/)

# Installation

To use ofxIpVideoGrabber, first you need to download and install [Open Frameworks](https://github.com/openframeworks/openFrameworks).

To get a copy of the repository you can download the source from [http://github.com/bakercp/ofxIpVideoGrabber](http://github.com/bakercp/ofxIpVideoGrabber) or, alternatively, you can use git clone:

```
git clone git://github.com/bakercp/ofxIpVideoGrabber.git`
```

The addon should sit in `openFrameworks/addons/ofxIpVideoGrabber/`.

#### Which version to use?

ofxIpVideoGrabber has been tested with the latest development version of openFrameworks including 0.9.0+.

The master branch should be is stable.  New features, etc are in the `develop` branch.

For past releases see https://github.com/bakercp/ofxIpVideoGrabber/releases

# Dependencies

None

# Compatability

 Edimax IC-3005
 Axis Cameras
 and others.

# How Do I Find Cameras?

You might try some of the following Google Searches:

```
inurl:”ViewerFrame?Mode=
intitle:Axis 2400 video server
inurl:/view.shtml
intitle:”Live View / – AXIS
inurl:view/view.shtml
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
```

# Alternatives

If you are interested in using these cameras with Processing or Max/MSP/Jitter, you can find code here: [https://github.com/themaw/mawLib/tree/master/src/mxj/trunk/mawLib-mxj/src/maw/jit/ipcam](https://github.com/themaw/mawLib/tree/master/src/mxj/trunk/mawLib-mxj/src/maw/jit/ipcam).  It is part of mawLib and includes pan/tilt, etc control code.

You might also check out [IPCAM2SYPHON](https://github.com/bakercp/IPCAM2SYPHON).  It wraps this library and sends cam textures via [Syphon](http://syphon.v002.info/).

# License
Copyright (c) 2011-2016 Christopher Baker <https://christopherbaker.net>

MIT License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.
