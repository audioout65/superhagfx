#include "NativeWindowCapture.h"
#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>

bool captureNativeWindowToFile (void* nsViewPtr, const std::string& outputPath)
{
    if (nsViewPtr == nullptr)
        return false;

    NSView* view = (__bridge NSView*) nsViewPtr;
    NSWindow* window = [view window];

    if (window == nullptr)
        return false;

    CGWindowID windowID = (CGWindowID) [window windowNumber];

    CGImageRef image = CGWindowListCreateImage (CGRectNull,
                                                 kCGWindowListOptionIncludingWindow,
                                                 windowID,
                                                 kCGWindowImageBoundsIgnoreFraming);

    if (image == nullptr)
        return false;

    NSString* pathString = [NSString stringWithUTF8String: outputPath.c_str()];
    NSURL* url = [NSURL fileURLWithPath: pathString];

    CGImageDestinationRef destination = CGImageDestinationCreateWithURL ((__bridge CFURLRef) url,
                                                                         (__bridge CFStringRef) @"public.png",
                                                                         1, nullptr);

    bool success = false;

    if (destination != nullptr)
    {
        CGImageDestinationAddImage (destination, image, nullptr);
        success = CGImageDestinationFinalize (destination);
        CFRelease (destination);
    }

    CGImageRelease (image);

    return success;
}
