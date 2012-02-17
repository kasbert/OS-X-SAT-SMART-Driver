/*
   g++ -framework CoreFoundation -framework IOKit -o SetPropertiesTestTool SetPropertiesTestTool.cpp
 */


/*
   File:           SetPropertiesTestTool.c

   Description:    This sample demonstrates how to set up a simple interface to a custom kernel driver
   using I/O Registry properties. It works with the VendorSpecificType00 driver included
   with this sample.

   Copyright:      © Copyright 2002-2006 Apple Computer, Inc. All rights reserved.

   Disclaimer:     IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
   ("Apple") in consideration of your agreement to the following terms, and your
   use, installation, modification or redistribution of this Apple software
   constitutes acceptance of these terms.  If you do not agree with these terms,
   please do not use, install, modify or redistribute this Apple software.

   In consideration of your agreement to abide by the following terms, and subject
   to these terms, Apple grants you a personal, non-exclusive license, under Apple's
   copyrights in this original Apple software (the "Apple Software"), to use,
   reproduce, modify and redistribute the Apple Software, with or without
   modifications, in source and/or binary forms; provided that if you redistribute
   the Apple Software in its entirety and without modifications, you must retain
   this notice and the following text and disclaimers in all such redistributions of
   the Apple Software.  Neither the name, trademarks, service marks or logos of
   Apple Computer, Inc. may be used to endorse or promote products derived from the
   Apple Software without specific prior written permission from Apple.  Except as
   expressly stated in this notice, no other rights or licenses, express or implied,
   are granted by Apple herein, including but not limited to any patent rights that
   may be infringed by your derivative works or by other works in which the Apple
   Software may be incorporated.

   The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
   WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
   WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
   COMBINATION WITH YOUR PRODUCTS.

   IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
   OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
   (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Change History (most recent first):

   1.1     08/31/2006          Updated to produce a universal binary. Now requires Xcode 2.2.1 or
   later to build.

   1.0     08/15/2002          New sample.

 */

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

// IO_OBJECT_NULL was added in Mac OS X 10.4. If it's not defined in the headers, define it ourselves.
// This allows the same code to build using the 10.4.0 SDK for Intel-based systems and earlier SDKs for backwards compatibility
// with PowerPC-based systems.
#ifndef IO_OBJECT_NULL
#define IO_OBJECT_NULL  ((io_object_t) 0)
#endif

int main (int argc, const char *argv[])
{
    CFMutableDictionaryRef dictRef;
    io_iterator_t iter;
    io_service_t service;
    kern_return_t kr;
    CFNumberRef numberRef;
    SInt32 constantOne = 1;

    constantOne=42;

    if (argc > 1) {
        constantOne = atoi(argv[1]);
    }

    // The bulk of this code locates all instances of our driver running on the system.

    // First find all children of our driver. As opposed to nubs, drivers are often not registered
    // via the registerServices call because nothing is expected to match to them. Unregistered
    // objects in the I/O Registry are not returned by IOServiceGetMatchingServices.

    // IOBlockStorageServices is our child in the I/O Registry
    dictRef = IOServiceMatching("IOBlockStorageServices");
    if (!dictRef) {
        fprintf(stderr, "IOServiceMatching returned NULL.\n");
        return -1;
    }

    // Create an iterator over all matching IOService nubs.
    // This consumes a reference on dictRef.
    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, dictRef, &iter);
    if (KERN_SUCCESS != kr) {
        fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x.\n", kr);
        CFRelease(dictRef);
        return -1;
    }

    // Create a dictionary to pass to our driver. This dictionary has the key "MyProperty"
    // and the value an integer 1.
    dictRef = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &constantOne);
    CFDictionarySetValue(dictRef, CFSTR("MyProperty"), numberRef);
    CFRelease(numberRef);

    // Iterate across all instances of IOBlockStorageServices.
    while ((service = IOIteratorNext(iter))) {
        io_registry_entry_t parent;

        // Now that our child has been found we can traverse the I/O Registry to find our driver.
        kr = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
        if (KERN_SUCCESS != kr) {
            fprintf(stderr, "IORegistryEntryGetParentEntry returned 0x%08x.\n", kr);
        }
        else {
            // We're only interested in the parent object if it's our driver class.
            if (IOObjectConformsTo(parent, "org_dungeon_driver_IOSATDriver")) {
                // This is the function that results in ::setProperties() being called in our
                // kernel driver. The dictionary we created is passed to the driver here.
                kr = IORegistryEntrySetCFProperties(parent, dictRef);
                if (KERN_SUCCESS != kr) {
                    fprintf(stderr, "IORegistryEntrySetCFProperties returned 0x%08x.\n", kr);
                }
            }

            // Done with the parent object.
            IOObjectRelease(parent);
        }

        // Done with the object returned by the iterator.
        IOObjectRelease(service);
    }

    if (iter != IO_OBJECT_NULL) {
        IOObjectRelease(iter);
        iter = IO_OBJECT_NULL;
    }

    if (dictRef) {
        CFRelease(dictRef);
        dictRef = NULL;
    }

    return 0;
}

