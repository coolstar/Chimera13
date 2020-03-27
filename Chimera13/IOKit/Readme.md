IOKit for iOS SDK7.0
=======

![](https://github.com/obaby/IOKit/blob/master/screenshot.jpg?raw=true)

在某些时候可能会用到IOKit来获取一些信息，但是将sdk从6.x升级到7.0的sdk之后就会发现那个libIOKit.dylib找不到了。晚上的办法是将6.x的sdk复制到7.0的sdk下，或者创建一个符号链接。

其实还有另外的一个解决办法，在7.0之后这个东西只是不是dylib了，而是成了一个framework。在这个目录下

 /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS7.0.sdk/System/Library/Frameworks/IOKit.framework，所以只需要将工程中的iokit用framework替换掉就可以了。另外这个并没有头文件，如果要用也得自己去提取相关的头文件。可以用classdump来生成。我用的是apple xun中的头文件，效果是一样的，这里整理了一下，需要的直接放入工程目录下引入IOKitLib.h就可以了。