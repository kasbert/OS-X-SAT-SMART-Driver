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

#define DEBUG 5
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
#define kIOATABlockStorageDeviceClass   "IOATABlockStorageDevice"
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


//—————————————————————————————————————————————————————————————————————————————
//	•	main - Our main entry point
//—————————————————————————————————————————————————————————————————————————————

int
main ( int argc, const char * argv[] )
{

    int result          = 0;
    int selector        = 0;

    result = ValidateArguments ( argc, argv, &selector );
    require_noerr_action ( result, ErrorExit, PrintUsage ( ) );

    printf ( "\n" );

    switch ( selector )
    {

    case kAllDevices:
        result = PrintSMARTDataForAllDrives ( );
        require_action ( ( result == kIOReturnSuccess ), ErrorExit, result = -1 );
        break;

    case kSpecificDevice:
        result = PrintSMARTDataForBSDNode ( argv[argc - 1] );
        require_action ( ( result == kIOReturnSuccess ), ErrorExit, result = -1 );
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

    require ( ( argc > 1 ), ErrorExit );
    require ( ( argc < 4 ), ErrorExit );

    if ( argc == 2 )
    {

        require ( ( strcmp ( argv[1], "-a" ) == 0 ), ErrorExit );
        *selector = kAllDevices;

    }

    else if ( argc == 3 )
    {

        require ( ( strcmp ( argv[1], "-d" ) == 0 ), ErrorExit );
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
        iter = NULL;

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
    CFNumberRef features        = NULL;
    UInt32 value           = 0;
    IOReturn err                     = kIOReturnNoResources;
    Boolean result          = false;

    err = IORegistryEntryCreateCFProperties ( ataDevice,
        &dict,
        kCFAllocatorDefault,
        kNilOptions );

    require ( ( err == kIOReturnSuccess ), ErrorExit );

    result = CFDictionaryGetValueIfPresent ( dict,
        CFSTR ( kIOPropertyDeviceCharacteristicsKey ),
        ( const void ** ) &deviceDict );

    require_action ( result, ReleaseProperties, err = kIOReturnError );

#if 0
    result = CFDictionaryGetValueIfPresent ( deviceDict,
        CFSTR ( "ATA Features" ),
        ( const void ** ) &features );

    require_action ( result, ReleaseProperties, err = kIOReturnError );

    result = CFNumberGetValue ( features, kCFNumberLongType, &value );
    require_action ( result, ReleaseProperties, err = kIOReturnError );

    require_action ( ( value & kIOATAFeatureSMART ), ReleaseProperties, err = kIOReturnError );
#endif

    err = PrintSMARTData ( ataDevice );


ReleaseProperties:


    require_quiet ( ( dict != NULL ), ErrorExit );
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

    require_action ( ( strncmp ( bsdName, "disk", 4 ) == 0 ), ErrorExit, PrintUsage ( ) );

    object = IOServiceGetMatchingService (  kIOMasterPortDefault,
        IOBSDNameMatching ( kIOMasterPortDefault, 0, bsdName ) );

    require ( ( object != MACH_PORT_NULL ), ErrorExit );

    parent = object;
    while ( IOObjectConformsTo ( object, kIOATABlockStorageDeviceClass ) == false )
    {

                #if DEBUG

        io_name_t className;

        error = IOObjectGetClass ( object, className );
        printf ( "Object class = %s\n", ( char * ) className );

                #endif

        error = IORegistryEntryGetParentEntry ( object, kIOServicePlane, &parent );
        require ( ( error == kIOReturnSuccess ), ReleaseObject );
        require ( ( parent != MACH_PORT_NULL ), ReleaseObject );

        IOObjectRelease ( object );
        object = parent;

    }

    if ( IOObjectConformsTo ( object, kIOATABlockStorageDeviceClass ) )
    {

        PrintSMARTDataForDevice ( object );
        found = true;

    }


ReleaseObject:


    require ( ( object != MACH_PORT_NULL ), ErrorExit );
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

    err = IOCreatePlugInInterfaceForService (       service,
        kIOATASMARTUserClientTypeID,
        kIOCFPlugInInterfaceID,
        &cfPlugInInterface,
        &score );

    require_string ( ( err == kIOReturnSuccess ), ErrorExit, "IOCreatePlugInInterfaceForService" );
    herr = ( *cfPlugInInterface )->QueryInterface (
        cfPlugInInterface,
        CFUUIDGetUUIDBytes ( kIOATASMARTInterfaceID ),
        ( LPVOID ) &smartInterface );

    require_string ( ( herr == S_OK ), ReleasePlugIn, "QueryInterface" );


    require_string ( ( smartInterface != NULL ), ReleasePlugIn, "smartInterface" );

    err = ( *smartInterface )->SMARTEnableDisableOperations ( smartInterface, true );
    require_string ( ( err == kIOReturnSuccess ), ReleaseInterface, "SMARTEnableDisableOperations" );

    err = ( *smartInterface )->SMARTEnableDisableAutosave ( smartInterface, true );
    require ( ( err == kIOReturnSuccess ), ReleaseInterface );

    err = ( *smartInterface )->SMARTReturnStatus ( smartInterface, &conditionExceeded );
    require ( ( err == kIOReturnSuccess ), ReleaseInterface );

    description = GetDriveDescription ( service );
    printf ( "SAT Drive: " );
    fflush ( stdout );
    CFShow ( description );
    CFRelease ( description );

    if ( conditionExceeded )
    {
        printf ( "SMART condition exceeded, drive will fail soon\n" );
    }

    else
    {
        printf ( "SMART condition not exceeded, drive OK\n" );
    }


ReleaseInterface:


    ( *smartInterface )->Release ( smartInterface );
    smartInterface = NULL;


ReleasePlugIn:


    err = IODestroyPlugInInterface ( cfPlugInInterface );
    require ( ( err == kIOReturnSuccess ), ErrorExit );


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

    require ( ( service != MACH_PORT_NULL ), Exit );

    err = IORegistryEntryCreateCFProperties (       service,
        &dict,
        kCFAllocatorDefault,
        0 );
    check ( err == kIOReturnSuccess );

    deviceDict = ( CFDictionaryRef ) CFDictionaryGetValue ( dict, CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );
    require ( ( deviceDict != 0 ), Exit );

    product = ( CFStringRef ) CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyProductNameKey ) );

    description = CFStringCreateMutableCopy ( kCFAllocatorDefault, 0, product );
    require ( ( description != 0 ), Exit );


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