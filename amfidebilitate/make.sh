#!/bin/bash
make clean
make ARCH=arm64 DEBUG=0
mv bin/amfidebilitate bin/amfidebilitate-arm64
make ARCH=arm64e DEBUG=0
mv bin/amfidebilitate bin/amfidebilitate-arm64e
lipo -create bin/amfidebilitate-arm64 bin/amfidebilitate-arm64e -output bin/amfidebilitate
rm bin/amfidebilitate-arm64*
