TARGETNAME = amfidebilitate
OUTDIR ?= bin

SWIFTC = xcrun -sdk iphoneos swiftc -target $(ARCH)-apple-ios13.0
CC = xcrun -sdk iphoneos clang -arch $(ARCH)
LDID = ./ldid

.PHONY: all clean

all: fat

SWIFTFLAGS = -Xcc "-Iinclude" -Xcc -DENABLE_XPC
CFLAGS = -Iinclude -DENABLE_XPC

DEBUG ?= 1
ifeq (($DEBUG), 1)
	SWIFTFLAGS += -D DEBUG
	CFLAGS += -DDEBUG
else
	SWIFTFLAGS += -O
	CFLAGS += -O3
endif

$(OUTDIR):
	mkdir -p $(OUTDIR)

%-$(ARCH).o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUTDIR)/$(TARGETNAME)-$(ARCH): main.swift machoparse.swift amfidtakeover.swift amfidhook.swift offsets.swift electra.swift swiftUtil.swift kmem-$(ARCH).o cutils-$(ARCH).o | $(OUTDIR)
	$(SWIFTC) -o $@ $^ -import-objc-header amfidebilitate-Bridging-Header.h $(SWIFTFLAGS)
	strip $@
	$(LDID) -Ksigncert.p12 -Sent.plist $@
	rm *.o

fat: 
	$(MAKE) $(OUTDIR)/$(TARGETNAME)-arm64 ARCH=arm64
	$(MAKE) $(OUTDIR)/$(TARGETNAME)-arm64e ARCH=arm64e
	lipo -create $(OUTDIR)/$(TARGETNAME)-arm64 $(OUTDIR)/$(TARGETNAME)-arm64e -output $(OUTDIR)/$(TARGETNAME)
	rm $(OUTDIR)/$(TARGETNAME)-arm64 $(OUTDIR)/$(TARGETNAME)-arm64e

clean:
	rm -f $(OUTDIR)/$(TARGETNAME) $(OUTDIR)/$(TARGETNAME)-arm64 $(OUTDIR)/$(TARGETNAME)-arm64e *.o

