ARCH ?= arm64
TARGET = amfidebilitate
OUTDIR ?= bin

SWIFTC = xcrun -sdk iphoneos swiftc -target $(ARCH)-apple-ios13.0 -v
CC = xcrun -sdk iphoneos clang -arch $(ARCH)
LDID = ./ldid

.PHONY: all clean

all: $(OUTDIR)/$(TARGET)

SWIFTFLAGS = 
CFLAGS = 

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

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUTDIR)/$(TARGET): main.swift machoparse.swift amfidtakeover.swift offsets.swift electra.swift kmem.o cutils.o | $(OUTDIR)
	$(SWIFTC) -o $@ $^ -import-objc-header amfidebilitate-Bridging-Header.h $(SWIFTFLAGS)
	strip $@
	$(LDID) -Ksigncert.p12 -Sent.plist $@
	rm *.o

clean:
	rm -f $(OUTDIR)/$(TARGET) *.o

