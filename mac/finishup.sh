#!/bin/sh
PLATFORM=`uname -s`

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p $2.app/Contents/MacOS
    mkdir -p $2.app/Contents/Resources

    cat $1/mac/Info.plist | sed s/APPNAME/$2/ >  $2.app/Contents/Info.plist
    cp -f $1/mac/PkgInfo $2.app/Contents 
    cp -f $1/mac/file.png $2.app/Contents/Resources
    cp -f $1/mac/folder.png $2.app/Contents/Resources
    cp -f $1/image/test.png $2.app/Contents/Resources
    cp -f $2 $2.app/Contents/MacOS
    # Check if there is a certificate to sign with...
    if [ ! -f mac/key.crt ]; then
       if [ -f mac/key.rsa ]; then
          # If not we generate a self-signed one for testing purposes
          echo "No certificate in mac/key.crt so generating self-signed certificate..."
          openssl req -new -key mac/key.rsa -out mac/key.csr -config mac/openssl.cnf
          openssl x509 -req -days 3650 -in mac/key.csr -signkey mac/key.rsa -out mac/key.crt -extfile mac/openssl.cnf -extensions codesign
          certtool i mac/key.crt k="`pwd`/mac/key.keychain" r=mac/key.rsa c p=moof
       else
           echo "No key pair found, cannot generate certificate... signing AdHoc."
           codesign -s "-" $2.app/Contents/MacOS/$2
       fi
    fi
    if [ -f mac/key.keychain ]; then
        echo "Signing the apllication with certificate in mac/key.crt"
        codesign -s my-signing-identity --keychain mac/key.keychain $2.app/Contents/MacOS/$2
    fi
fi
