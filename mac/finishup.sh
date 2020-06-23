#!/bin/sh
PLATFORM=`uname -s`

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p dwtest.app/Contents/MacOS
    mkdir -p dwtest.app/Contents/Resources

    cp -f $1/mac/Info.plist dwtest.app/Contents
    cp -f $1/mac/PkgInfo dwtest.app/Contents
    cp -f dwtest dwtest.app/Contents/MacOS
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
           codesign -s "-" dwtest.app/Contents/MacOS/dwtest
       fi
    fi
    if [ -f mac/key.keychain ]; then
        echo "Signing the apllication with certificate in mac/key.crt"
        codesign -s my-signing-identity --keychain mac/key.keychain dwtest.app/Contents/MacOS/dwtest
    fi
fi
