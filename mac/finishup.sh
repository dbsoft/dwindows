#!/bin/sh
PLATFORM=`uname -s`
SRCDIR=$1
BINNAME=$2
IDENTITY=$3

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p $BINNAME.app/Contents/MacOS
    mkdir -p $BINNAME.app/Contents/Resources

    cat $SRCDIR/mac/Info.template | sed s/APPNAME/$BINNAME/ >  $BINNAME.app/Contents/Info.plist
    cp -f $SRCDIR/mac/PkgInfo $BINNAME.app/Contents 
    cp -f $SRCDIR/mac/file.png $BINNAME.app/Contents/Resources
    cp -f $SRCDIR/mac/folder.png $BINNAME.app/Contents/Resources
    cp -f $SRCDIR/image/test.png $BINNAME.app/Contents/Resources
    cp -f $BINNAME $BINNAME.app/Contents/MacOS
    # Check if there is a certificate to sign with...
    if [ -z "$IDENTITY" ]; then
        echo "No identity set signing AdHoc."
        codesign --deep -s "-" $BINNAME.app
    else
        echo "Signing code with identity: $IDENTITY"
        codesign --deep -s "$IDENTITY" $BINNAME.app
    fi
fi
