/*
 * smart_status.c
 * SMARTStatus
 *
 * Created by Jim Dovey on 6/12/2009.
 *
 * Copyright (c) 2009 Jim Dovey
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the project's author nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
   gcc -framework CoreFoundation -framework IOKit -o SMARTStatus smart_status.c
 */

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOMessage.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/ata/ATASMARTLib.h>
#include <sysexits.h>

static const char *getStatus(int status) {
    switch (status) {
    case kIOReturnSuccess: return "kIOReturnSuccess";
    case kIOReturnError: return "kIOReturnError";
    case kIOReturnNoMemory: return "kIOReturnNoMemory";
    case kIOReturnNoResources: return "kIOReturnNoResources";
    case kIOReturnIPCError: return "kIOReturnIPCError";
    case kIOReturnNoDevice: return "kIOReturnNoDevice";
    case kIOReturnNotPrivileged: return "kIOReturnNotPrivileged";
    case kIOReturnBadArgument: return "kIOReturnBadArgument";
    case kIOReturnLockedRead: return "kIOReturnLockedRead";
    case kIOReturnLockedWrite: return "kIOReturnLockedWrite";
    case kIOReturnExclusiveAccess: return "kIOReturnExclusiveAccess";
    case kIOReturnBadMessageID: return "kIOReturnBadMessageID";
    case kIOReturnUnsupported: return "kIOReturnUnsupported";
    case kIOReturnVMError: return "kIOReturnVMError";
    case kIOReturnInternalError: return "kIOReturnInternalError";
    case kIOReturnIOError: return "kIOReturnIOError";
    //  case kIOReturn???Error: return "kIOReturn???Error";
    case kIOReturnCannotLock: return "kIOReturnCannotLock";
    case kIOReturnNotOpen: return "kIOReturnNotOpen";
    case kIOReturnNotReadable: return "kIOReturnNotReadable";
    case kIOReturnNotWritable: return "kIOReturnNotWritable";
    case kIOReturnNotAligned: return "kIOReturnNotAligned";
    case kIOReturnBadMedia: return "kIOReturnBadMedia";
    case kIOReturnStillOpen: return "kIOReturnStillOpen";
    case kIOReturnRLDError: return "kIOReturnRLDError";
    case kIOReturnDMAError: return "kIOReturnDMAError";
    case kIOReturnBusy: return "kIOReturnBusy";
    case kIOReturnTimeout: return "kIOReturnTimeout";
    case kIOReturnOffline: return "kIOReturnOffline";
    case kIOReturnNotReady: return "kIOReturnNotReady";
    case kIOReturnNotAttached: return "kIOReturnNotAttached";
    case kIOReturnNoChannels: return "kIOReturnNoChannels";
    case kIOReturnNoSpace: return "kIOReturnNoSpace";
    //case kIOReturn???Error: return "kIOReturn???Error";
    case kIOReturnPortExists: return "kIOReturnPortExists";
    case kIOReturnCannotWire: return "kIOReturnCannotWire";
    case kIOReturnNoInterrupt: return "kIOReturnNoInterrupt";
    case kIOReturnNoFrames: return "kIOReturnNoFrames";
    case kIOReturnMessageTooLarge: return "kIOReturnMessageTooLarge";
    case kIOReturnNotPermitted: return "kIOReturnNotPermitted";
    case kIOReturnNoPower: return "kIOReturnNoPower";
    case kIOReturnNoMedia: return "kIOReturnNoMedia";
    case kIOReturnUnformattedMedia: return "kIOReturnUnformattedMedia";
    case kIOReturnUnsupportedMode: return "kIOReturnUnsupportedMode";
    case kIOReturnUnderrun: return "kIOReturnUnderrun";
    case kIOReturnOverrun: return "kIOReturnOverrun";
    case kIOReturnDeviceError: return "kIOReturnDeviceError	";
    case kIOReturnNoCompletion: return "kIOReturnNoCompletion	";
    case kIOReturnAborted: return "kIOReturnAborted	";
    case kIOReturnNoBandwidth: return "kIOReturnNoBandwidth	";
    case kIOReturnNotResponding: return "kIOReturnNotResponding	";
    case kIOReturnIsoTooOld: return "kIOReturnIsoTooOld	";
    case kIOReturnIsoTooNew: return "kIOReturnIsoTooNew	";
    case kIOReturnNotFound: return "kIOReturnNotFound";
    case kIOReturnInvalid: return "kIOReturnInvalid";
    }
    return "INVALID";
}

static void PrintStatusForDevice( CFStringRef name, UInt8 status )
{
#define CBUFLEN 128
    char buf[CBUFLEN];
    CFStringGetCString( name, buf, CBUFLEN, kCFStringEncodingUTF8 );

    char * msg = "<unknown>";
    switch ( status )
    {
    case kATASMARTSelfTestStatusNoError:
        msg = "No Error";
        break;

    case kATASMARTSelfTestStatusAbortedByHost:
        msg = "Test aborted by host";
        break;

    case kATASMARTSelfTestStatusInterruptedByReset:
        msg = "Interrupted by reset";
        break;

    case kATASMARTSelfTestStatusFatalError:
        msg = "Fatal error prevented test completion";
        break;

    case kATASMARTSelfTestStatusPreviousTestUnknownFailure:
        msg = "Error in unknown test module";
        break;

    case kATASMARTSelfTestStatusPreviousTestElectricalFailure:
        msg = "Electrical test failed";
        break;

    case kATASMARTSelfTestStatusPreviousTestServoFailure:
        msg = "Servo test failed";
        break;

    case kATASMARTSelfTestStatusPreviousTestReadFailure:
        msg = "Read test failed";
        break;

    case kATASMARTSelfTestStatusInProgress:
        msg = "Test in progress";
        break;

    default:
        break;
    }

    fprintf( stdout, "%s: %s\n", buf, msg );
}

static CFStringRef BSDNameForBlockStorageDevice( io_registry_entry_t service )
{
    // This should be an IOBlockStorageDevice. It'll have one child, an IOBlockStorageDriver,
    // which will have one child of its own, an IOMedia. We get the BSD name from the IOMedia.

    static CFStringRef kUnknownDiskBSDName = CFSTR("disk[??]");
    kern_return_t kr = KERN_SUCCESS;

    io_registry_entry_t driver = MACH_PORT_NULL;
    kr = IORegistryEntryGetChildEntry( service, kIOServicePlane, &driver );
    if ( driver == MACH_PORT_NULL )
        return ( kUnknownDiskBSDName );

    if ( IOObjectConformsTo(driver, kIOBlockStorageDriverClass) == FALSE )
        return ( kUnknownDiskBSDName );

    io_registry_entry_t media = MACH_PORT_NULL;
    kr = IORegistryEntryGetChildEntry( driver, kIOServicePlane, &media );
    IOObjectRelease( driver );
    if ( media == MACH_PORT_NULL )
        return ( kUnknownDiskBSDName );

    if ( IOObjectConformsTo(media, kIOMediaClass) == FALSE )
        return ( kUnknownDiskBSDName );

    CFStringRef str = IORegistryEntryCreateCFProperty( media, CFSTR("BSD Name"), NULL, 0 );
    if ( str == NULL )
        str = kUnknownDiskBSDName;

    IOObjectRelease( media );
    return ( str );
}

static void CheckSMARTStatus( io_service_t service )
{
    IOCFPlugInInterface ** pluginInterface = NULL;
    IOReturn kr = kIOReturnSuccess;
    HRESULT hr = S_FALSE;
    SInt32 score = 0;
    const char *msg;
    CFStringRef bsdName;
    char buf[CBUFLEN];

    // create the interface object
    kr = IOCreatePlugInInterfaceForService( service, kIOATASMARTUserClientTypeID, kIOCFPlugInInterfaceID,
        &pluginInterface, &score );
    if ( kr != kIOReturnSuccess )
        goto ErrorExit;

    // get the right type of interface
    IOATASMARTInterface **ppSmart = NULL;
    hr = (*pluginInterface)->QueryInterface( pluginInterface, CFUUIDGetUUIDBytes(kIOATASMARTInterfaceID),
        (LPVOID) &ppSmart );
    (*pluginInterface)->Release( pluginInterface );

    if ( hr != S_OK )
        goto ErrorExit;

    ATASMARTData smartData;
    bzero( &smartData, sizeof(ATASMARTData) );
    kr = (*ppSmart)->SMARTReadData( ppSmart, &smartData );
    (*ppSmart)->Release( ppSmart );

    if ( kr != kIOReturnSuccess )
        goto ErrorExit;

    // got data, now get a BSD name
    bsdName = BSDNameForBlockStorageDevice( service );
    PrintStatusForDevice( bsdName, smartData.selfTestExecutionStatus );
    CFRelease( bsdName );
    return;

ErrorExit:
    msg = getStatus(kr);
    bsdName = BSDNameForBlockStorageDevice( service );
    CFStringGetCString( bsdName, buf, CBUFLEN, kCFStringEncodingUTF8 );
    fprintf( stdout, "%s: %s (%x)\n", buf, msg, kr );
    CFRelease( bsdName );
}

int main (int argc, const char * argv[])
{
    CFMutableDictionaryRef match = IOServiceMatching( kIOBlockStorageDeviceClass );
    CFDictionaryAddValue( match, CFSTR("SMART Capable"), kCFBooleanTrue );

    io_iterator_t iterator = MACH_PORT_NULL;
    if ( IOServiceGetMatchingServices(kIOMasterPortDefault, match, &iterator) != kIOReturnSuccess )
        return ( EX_OSERR );

    io_service_t service = MACH_PORT_NULL;
    while ( (service = IOIteratorNext(iterator)) != MACH_PORT_NULL )
    {
        CheckSMARTStatus( service );
        IOObjectRelease( service );
    }

    IOObjectRelease( iterator );

    return ( 0 );
}
