#!/bin/sh
PLATFORM=`uname -s`

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p dwtest.app/Contents/MacOS
    mkdir -p dwtest.app/Contents/Resources
    
    cp -f mac/Info.plist dwtest.app/Contents
    cp -f mac/PkgInfo dwtest.app/Contents
    cp -f dwtest dwtest.app/Contents/MacOS
    /Developer/Tools/Rez  -o dwtest.app/Contents/Resources/dwtest.rsrc  -d "SystemSevenOrLater=1" -useDF mac/dwtest.r
fi