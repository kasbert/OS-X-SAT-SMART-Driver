/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * © Copyright 2003 Apple Computer, Inc. All rights reserved.
 *
 * IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc. (“Apple”) in
 * consideration of your agreement to the following terms, and your use, installation,
 * modification or redistribution of this Apple software constitutes acceptance of these
 * terms.  If you do not agree with these terms, please do not use, install, modify or
 * redistribute this Apple software.
 *
 * In consideration of your agreement to abide by the following terms, and subject to these
 * terms, Apple grants you a personal, non exclusive license, under Apple’s copyrights in this
 * original Apple software (the “Apple Software”), to use, reproduce, modify and redistribute
 * the Apple Software, with or without modifications, in source and/or binary forms; provided
 * that if you redistribute the Apple Software in its entirety and without modifications, you
 * must retain this notice and the following text and disclaimers in all such redistributions
 * of the Apple Software.  Neither the name, trademarks, service marks or logos of Apple
 * Computer, Inc. may be used to endorse or promote products derived from the Apple Software
 * without specific prior written permission from Apple. Except as expressly stated in this
 * notice, no other rights or licenses, express or implied, are granted by Apple herein,
 * including but not limited to any patent rights that may be infringed by your derivative
 * works or by other works in which the Apple Software may be incorporated.
 *
 * The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-
 * INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE
 * SOFTWARE OR ITS USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 * IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE,
 * REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND
 * WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
 * OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef DEBUG_ASSERT_PRODUCTION_CODE
#undef DEBUG_ASSERT_PRODUCTION_CODE
#endif

#define DEBUG_ASSERT_PRODUCTION_CODE 1

//—————————————————————————————————————————————————————————————————————————————
//	Includes
//—————————————————————————————————————————————————————————————————————————————

#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/param.h>
#include <sys/time.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_init.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>
#include <IOKit/storage/ata/IOATAStorageDefines.h>
#include <IOKit/storage/ata/ATASMARTLib.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <CoreFoundation/CoreFoundation.h>


//—————————————————————————————————————————————————————————————————————————————
//	Macros
//—————————————————————————————————————————————————————————————————————————————

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 5
#ifdef DEBUG_ASSERT_COMPONENT_NAME_STRING
#undef DEBUG_ASSERT_COMPONENT_NAME_STRING
#endif
#ifdef DEBUG_ASSERT_MESSAGE
#undef DEBUG_ASSERT_MESSAGE
#endif
#define DEBUG_ASSERT_COMPONENT_NAME_STRING "ATASMARTSample"
#define DEBUG_ASSERT_MESSAGE(componentNameString,       \
assertionString,               \
exceptionLabelString,  \
errorString,                   \
fileName,                              \
lineNumber,                    \
errorCode)                             \
_DebugAssert(componentNameString,                                       \
assertionString,                             \
exceptionLabelString,                \
errorString,                                 \
fileName,                                    \
lineNumber,                                  \
errorCode)                                   \

static void
_DebugAssert ( const char *     componentNameString,
              const char *  assertionString,
              const char *  exceptionLabelString,
              const char *  errorString,
              const char *  fileName,
              long lineNumber,
              int errorCode )
{
    
    if ( ( assertionString != NULL ) && ( *assertionString != '\0' ) )
        printf ( "Assertion failed: %s: %s\n", componentNameString, assertionString );
    else
        printf ( "Check failed: %s:\n", componentNameString );
    if ( exceptionLabelString != NULL )
        printf ( "	 %s\n", exceptionLabelString );
    if ( errorString != NULL )
        printf ( "	 %s\n", errorString );
    if ( fileName != NULL )
        printf ( "	 file: %s\n", fileName );
    if ( lineNumber != 0 )
        printf ( "	 line: %ld\n", lineNumber );
    if ( errorCode != 0 )
        printf ( "	 error: %d\n", errorCode );
    
}

#include <AssertMacros.h>


//—————————————————————————————————————————————————————————————————————————————
//	Constants
//—————————————————————————————————————————————————————————————————————————————

#define kATADefaultSectorSize                   512
#define kIOATABlockStorageDeviceClass   "IOSATServices"

enum
{
    kUnknown                        = -1,
    kAllDevices             = 0,
    kSpecificDevice         = 1
};


//—————————————————————————————————————————————————————————————————————————————
//	Prototypes
//—————————————————————————————————————————————————————————————————————————————

static IOReturn
ValidateArguments ( int argc, const char * argv[], int * selector );

static void
PrintUsage ( void );

static IOReturn
PrintSMARTDataForAllDrives ( void );

static IOReturn
PrintSMARTDataForBSDNode ( const char * bsdNode );

static IOReturn
PrintSMARTDataForDevice ( io_object_t ataDevice );

static IOReturn
PrintSMARTData ( io_service_t service );

static CFStringRef
GetDriveDescription ( io_object_t service );


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

void swapbytes(UInt16 *ptr, int len) {
    int i;
    for (i = 0; i < len / sizeof(UInt16); i++) {
        ptr[i]=Endian16_Swap(ptr[i]);
    }
}

//—————————————————————————————————————————————————————————————————————————————
//	•	main - Our main entry point
//—————————————————————————————————————————————————————————————————————————————

int
main ( int argc, const char * argv[] )
{
    
    int result          = 0;
    int selector        = 0;
    
    result = ValidateArguments ( argc, argv, &selector );
    __Require_noErr_Action ( result, ErrorExit, PrintUsage ( ) );
    
    printf ( "\n" );
    
    switch ( selector )
    {
            
        case kAllDevices:
            result = PrintSMARTDataForAllDrives ( );
            __Require_Action ( ( result == kIOReturnSuccess ), ErrorExit, result = -1 );
            break;
            
        case kSpecificDevice:
            result = PrintSMARTDataForBSDNode ( argv[argc - 1] );
            __Require_Action ( ( result == kIOReturnSuccess ), ErrorExit, result = -1 );
            break;
            
        default:
            break;
            
    }
    
    
ErrorExit:
    
    
    return result;
    
}


//———————————————————————————————————————————————————————————————————————————
//	ValidateArguments - Validates arguments
//———————————————————————————————————————————————————————————————————————————

static IOReturn
ValidateArguments ( int argc, const char * argv[], int * selector )
{
    
    IOReturn result = kIOReturnError;
    
    __Require ( ( argc > 1 ), ErrorExit );
    __Require ( ( argc < 4 ), ErrorExit );
    
    if ( argc == 2 )
    {
        
        __Require ( ( strcmp ( argv[1], "-a" ) == 0 ), ErrorExit );
        *selector = kAllDevices;
        
    }
    
    else if ( argc == 3 )
    {
        
        __Require ( ( strcmp ( argv[1], "-d" ) == 0 ), ErrorExit );
        *selector = kSpecificDevice;
        
    }
    
    result = kIOReturnSuccess;
    
    
ErrorExit:
    
    
    return result;
    
}


//———————————————————————————————————————————————————————————————————————————
//	PrintSMARTDataForAllDrives - Prints S.M.A.R.T. data for all ATA drives
//———————————————————————————————————————————————————————————————————————————

static IOReturn
PrintSMARTDataForAllDrives ( void )
{
    
    IOReturn error           = kIOReturnSuccess;
    io_iterator_t iter            = MACH_PORT_NULL;
    io_object_t ataDevice       = MACH_PORT_NULL;
    bool foundOne        = false;
    
    error = IOServiceGetMatchingServices (  kIOMasterPortDefault,
                                          IOServiceMatching ( kIOATABlockStorageDeviceClass ),
                                          &iter );
    
    if ( error == kIOReturnSuccess )
    {
        ataDevice = IOIteratorNext ( iter );
        
        while ( ataDevice != MACH_PORT_NULL )
        {
            
            error = PrintSMARTDataForDevice ( ataDevice );
            IOObjectRelease ( ataDevice );
            ataDevice = IOIteratorNext ( iter );
            
            if ( foundOne == false )
                foundOne = true;
            
        }
        
        IOObjectRelease ( iter );
        iter = 0;
        
    }
    
    if ( foundOne == false )
    {
        
        printf ( "No ATA devices found\n" );
        
    }
    
    return error;
    
}


//———————————————————————————————————————————————————————————————————————————
//	PrintSMARTDataForDevice - Prints S.M.A.R.T. data for an ATA device
//———————————————————————————————————————————————————————————————————————————

static IOReturn
PrintSMARTDataForDevice ( io_object_t ataDevice )
{
    
    CFMutableDictionaryRef dict            = NULL;
    CFDictionaryRef deviceDict      = NULL;
#if 0
    CFNumberRef features        = NULL;
    UInt32 value           = 0;
#endif
    IOReturn err                     = kIOReturnNoResources;
    Boolean result          = false;
    
    err = IORegistryEntryCreateCFProperties ( ataDevice,
                                             &dict,
                                             kCFAllocatorDefault,
                                             kNilOptions );
    
    __Require ( ( err == kIOReturnSuccess ), ErrorExit );
    
    result = CFDictionaryGetValueIfPresent ( dict,
                                            CFSTR ( kIOPropertyDeviceCharacteristicsKey ),
                                            ( const void ** ) &deviceDict );
    
    __Require_Action ( result, ReleaseProperties, err = kIOReturnError );
    
#if 0
    result = CFDictionaryGetValueIfPresent ( deviceDict,
                                            CFSTR ( "ATA Features" ),
                                            ( const void ** ) &features );
    
    __Require_Action ( result, ReleaseProperties, err = kIOReturnError );
    
    result = CFNumberGetValue ( features, kCFNumberLongType, &value );
    __Require_Action ( result, ReleaseProperties, err = kIOReturnError );
    
    __Require_Action ( ( value & kIOATAFeatureSMART ), ReleaseProperties, err = kIOReturnError );
#endif
    
    err = PrintSMARTData ( ataDevice );
    
    
ReleaseProperties:
    
    
    __Require_Quiet ( ( dict != NULL ), ErrorExit );
    CFRelease ( dict );
    dict = NULL;
    
    
ErrorExit:
    
    
    return err;
    
}


//———————————————————————————————————————————————————————————————————————————
//	PrintSMARTDataForBSDNode - Prints S.M.A.R.T data for a particular BSD Node
//———————————————————————————————————————————————————————————————————————————

static IOReturn
PrintSMARTDataForBSDNode ( const char * bsdNode )
{
    
    
    IOReturn error   = kIOReturnError;
    io_object_t object  = MACH_PORT_NULL;
    io_object_t parent  = MACH_PORT_NULL;
    bool found   = false;
    char *                  bsdName = NULL;
    char deviceName[MAXPATHLEN];
    
    sprintf ( deviceName, "%s", bsdNode );
    
    if ( !strncmp ( deviceName, "/dev/r", 6 ) )
    {
        
        // Strip off the /dev/r from /dev/rdiskX
        bsdName = &deviceName[6];
        
    }
    
    else if ( !strncmp ( deviceName, "/dev/", 5 ) )
    {
        
        // Strip off the /dev/r from /dev/rdiskX
        bsdName = &deviceName[5];
        
    }
    
    else
    {
        
        bsdName = deviceName;
        
    }
    
    __Require_Action ( ( strncmp ( bsdName, "disk", 4 ) == 0 ), ErrorExit, PrintUsage ( ) );
    
    object = IOServiceGetMatchingService (  kIOMasterPortDefault,
                                          IOBSDNameMatching ( kIOMasterPortDefault, 0, bsdName ) );
    
    __Require ( ( object != MACH_PORT_NULL ), ErrorExit );
    
    parent = object;
    while ( IOObjectConformsTo ( object, kIOATABlockStorageDeviceClass ) == false )
    {
        
#if DEBUG
        
        io_name_t className;
        
        error = IOObjectGetClass ( object, className );
        printf ( "Object class = %s\n", ( char * ) className );
        
#endif
        
        error = IORegistryEntryGetParentEntry ( object, kIOServicePlane, &parent );
        __Require ( ( error == kIOReturnSuccess ), ReleaseObject );
        __Require ( ( parent != MACH_PORT_NULL ), ReleaseObject );
        
        IOObjectRelease ( object );
        object = parent;
        
    }
    
    if ( IOObjectConformsTo ( object, kIOATABlockStorageDeviceClass ) )
    {
        
        PrintSMARTDataForDevice ( object );
        found = true;
        
    }
    
    
ReleaseObject:
    
    
    __Require ( ( object != MACH_PORT_NULL ), ErrorExit );
    IOObjectRelease ( object );
    object = MACH_PORT_NULL;
    
    
ErrorExit:
    
    
    if ( found == false )
    {
        printf ( "No S.M.A.R.T. capable device at %s\n", bsdNode );
    }
    
    return error;
    
}


//———————————————————————————————————————————————————————————————————————————
//	PrintSMARTData - Prints S.M.A.R.T data
//———————————————————————————————————————————————————————————————————————————

IOReturn
PrintSMARTData ( io_service_t service )
{
    
    IOCFPlugInInterface     **              cfPlugInInterface       = NULL;
    IOATASMARTInterface     **              smartInterface          = NULL;
    HRESULT herr                            = S_OK;
    IOReturn err                                     = kIOReturnSuccess;
    SInt32 score                           = 0;
    Boolean conditionExceeded       = false;
    CFStringRef description                     = NULL;
    UInt8 buffer[512];
    UInt32 bytesRead;
    ATASMARTLogDirectory logData;
    
    err = IOCreatePlugInInterfaceForService (       service,
                                             kIOATASMARTUserClientTypeID,
                                             kIOCFPlugInInterfaceID,
                                             &cfPlugInInterface,
                                             &score );
    
    __Require_String ( ( err == kIOReturnSuccess ), ErrorExit, "IOCreatePlugInInterfaceForService" );
    herr = ( *cfPlugInInterface )->QueryInterface (
                                                   cfPlugInInterface,
                                                   CFUUIDGetUUIDBytes ( kIOATASMARTInterfaceID ),
                                                   ( LPVOID ) &smartInterface );
    
    __Require_String ( ( herr == S_OK ), ReleasePlugIn, "QueryInterface" );
    
    
    __Require_String ( ( smartInterface != NULL ), ReleasePlugIn, "smartInterface" );
    
    description = GetDriveDescription ( service );
    printf ( "SAT Drive: " );
    fflush ( stdout );
    CFShow ( description );
    CFRelease ( description );
    
    err = ( *smartInterface )->SMARTEnableDisableOperations ( smartInterface, true );
    __Require_String ( ( err == kIOReturnSuccess ), ReleaseInterface, "SMARTEnableDisableOperations" );
    
    err = ( *smartInterface )->SMARTEnableDisableAutosave ( smartInterface, true );
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
    
    err = ( *smartInterface )->SMARTReturnStatus ( smartInterface, &conditionExceeded );
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
    
    if ( conditionExceeded )
    {
        printf ( "SMART condition exceeded, drive will fail soon\n" );
    }
    
    else
    {
        printf ( "SMART condition not exceeded, drive OK\n" );
    }
    
    err = ( *smartInterface )->GetATAIdentifyData (smartInterface, &buffer, sizeof buffer, &bytesRead );
    swapbytes((UInt16*)buffer, bytesRead);
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
    __Require ( ( bytesRead == sizeof buffer ), ReleaseInterface );
    buffer[2 * kATAIdentifyModelNumber + 40 ] = 0;
    printf ( "Model: '%s'\n", buffer + 2 * kATAIdentifyModelNumber ); // FIXME not null terminated
    
    err = ( *smartInterface )->SMARTReadLogDirectory (smartInterface, &logData );
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
#if 0
    for (int i = 0; i < 255; i++) {
        if (logData.entries[i].numberOfSectors > 0)
            printf ( "entry[%d]: %d\n", i, logData.entries[i].numberOfSectors);
    }
    err = ( *smartInterface )->SMARTReadLogAtAddress ( smartInterface,
                                                      224,
                                                      buffer,
                                                      sizeof buffer);
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
    for (int i = 0; i < -512; i++) {
        if (buffer[i])
            printf ( "buffer[%d]: %d\n", i, buffer[i]);
    }
    err = ( *smartInterface )->SMARTWriteLogAtAddress ( smartInterface,
                                                       224,
                                                       buffer,
                                                       sizeof buffer);
    __Require ( ( err == kIOReturnSuccess ), ReleaseInterface );
#endif
    
ReleaseInterface:
    
    printf ("status %s\n", getStatus(err));
    
    ( *smartInterface )->Release ( smartInterface );
    smartInterface = NULL;
    
    
ReleasePlugIn:
    
    
    err = IODestroyPlugInInterface ( cfPlugInInterface );
    __Require ( ( err == kIOReturnSuccess ), ErrorExit );
    
    
ErrorExit:
    return err;
    
}


//—————————————————————————————————————————————————————————————————————————————
//	•	GetDriveDescription - Creates a drive description. Caller must call
//							  CFRelease on returned CFStringRef if non-NULL.
//—————————————————————————————————————————————————————————————————————————————

static CFStringRef
GetDriveDescription ( io_object_t service )
{
    
    CFMutableStringRef description = NULL;
    CFDictionaryRef deviceDict      = NULL;
    CFStringRef product         = NULL;
    CFMutableDictionaryRef dict            = NULL;
    IOReturn err                     = kIOReturnSuccess;
    
    __Require ( ( service != MACH_PORT_NULL ), Exit );
    
    err = IORegistryEntryCreateCFProperties (       service,
                                             &dict,
                                             kCFAllocatorDefault,
                                             0 );
    __Check ( err == kIOReturnSuccess );
    
    deviceDict = ( CFDictionaryRef ) CFDictionaryGetValue ( dict, CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );
    __Require ( ( deviceDict != 0 ), Exit );
    
    product = ( CFStringRef ) CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyProductNameKey ) );
    
    description = CFStringCreateMutableCopy ( kCFAllocatorDefault, 0L, product );
    __Require ( ( description != NULL ), Exit );
    
    
Exit:
    
    
    return description;
    
}


//———————————————————————————————————————————————————————————————————————————
//	•	PrintUsage - Prints out usage
//———————————————————————————————————————————————————————————————————————————

void
PrintUsage ( void )
{
    
    printf ( "\n" );
    printf ( "Usage: ATASMARTSample [-a] [-d label]\n" );
    printf ( "\t\t" );
    printf ( "-a Check S.M.A.R.T status for all available ATA devices\n" );
    printf ( "\t\t" );
    printf ( "-d Enter a specific bsd-style disk path to use" );
    printf ( "\t\t" );
    printf ( "e.g. /dev/disk0, /dev/rdisk2, disk3" );
    printf ( "\n" );
    
}
