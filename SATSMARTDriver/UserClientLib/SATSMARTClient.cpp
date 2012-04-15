/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Includes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// Private includes
///Developer/SDKs/MacOSX10.6.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/
#include <storage/ata/ATASMARTLib.h>
//#include "ATASMARTLib.h"
#include "SATSMARTClient.h"
#include "SATSMARTLibPriv.h"

#include <stdio.h>


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constants
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


enum
{
    kATASMARTLogDirectoryEntry      = 0x00
};

enum
{
    kATADefaultSectorSize = 512
};


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Macros
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#ifdef DEBUG
#define SAT_SMART_DEBUGGING_LEVEL 1
#else
#define SAT_SMART_DEBUGGING_LEVEL 0
#endif

#if ( SAT_SMART_DEBUGGING_LEVEL > 0 )
#define PRINT(x)        printf x
#else
#define PRINT(x)
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Static variable initialization
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

UInt32 SATSMARTClient::sFactoryRefCount = 0;


IOCFPlugInInterface
SATSMARTClient::sIOCFPlugInInterface =
{
    0,
    &SATSMARTClient::sQueryInterface,
    &SATSMARTClient::sAddRef,
    &SATSMARTClient::sRelease,
    1, 0,     // version/revision
    &SATSMARTClient::sProbe,
    &SATSMARTClient::sStart,
    &SATSMARTClient::sStop
};

IOATASMARTInterface
SATSMARTClient::sATASMARTInterface =
{
    0,
    &SATSMARTClient::sQueryInterface,
    &SATSMARTClient::sAddRef,
    &SATSMARTClient::sRelease,
    1, 0,     // version/revision
    &SATSMARTClient::sSMARTEnableDisableOperations,
    &SATSMARTClient::sSMARTEnableDisableAutosave,
    &SATSMARTClient::sSMARTReturnStatus,
    &SATSMARTClient::sSMARTExecuteOffLineImmediate,
    &SATSMARTClient::sSMARTReadData,
    &SATSMARTClient::sSMARTValidateReadData,
    &SATSMARTClient::sSMARTReadDataThresholds,
    &SATSMARTClient::sSMARTReadLogDirectory,
    &SATSMARTClient::sSMARTReadLogAtAddress,
    &SATSMARTClient::sSMARTWriteLogAtAddress,
    &SATSMARTClient::sGetATAIdentifyData
};


#if 0
#pragma mark -
#pragma mark Methods associated with ATASMARTLib factory
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SATSMARTLibFactory - Factory method. Exported via plist		[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void *
SATSMARTLibFactory ( CFAllocatorRef allocator, CFUUIDRef typeID )
{

    PRINT ( ( "SATSMARTLibFactory called\n" ) );

    if ( CFEqual ( typeID, kIOATASMARTUserClientTypeID ) )
        return ( void * ) SATSMARTClient::alloc ( );

    else
        return NULL;

}



//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ alloc - Used to allocate an instance of SATSMARTClient		[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOCFPlugInInterface **
SATSMARTClient::alloc ( void )
{

    SATSMARTClient *                        userClient;
    IOCFPlugInInterface **          interface = NULL;

    PRINT ( ( "SATSMARTClient::alloc called\n" ) );

    userClient = new SATSMARTClient;
    if ( userClient != NULL )
    {

        interface = ( IOCFPlugInInterface ** ) &userClient->fCFPlugInInterfaceMap.pseudoVTable;

    }

    return interface;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sFactoryAddRef -      Static method to increment the refcount associated with
//						the CFPlugIn factory
//																	[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTClient::sFactoryAddRef ( void )
{

    if ( sFactoryRefCount++ == 0 )
    {

        CFUUIDRef factoryID = kIOATASMARTLibFactoryID;

        CFRetain ( factoryID );
        CFPlugInAddInstanceForFactory ( factoryID );

    }

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sFactoryRelease - Static method to decrement the refcount associated with
//						the CFPlugIn factory and release it when the refcount
//						becomes zero.								[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTClient::sFactoryRelease ( void )
{

    if ( sFactoryRefCount-- == 1 )
    {

        CFUUIDRef factoryID = kIOATASMARTLibFactoryID;

        CFPlugInRemoveInstanceForFactory ( factoryID );
        CFRelease ( factoryID );

    }

    else if ( sFactoryRefCount < 0 )
    {
        sFactoryRefCount = 0;
    }

}


#if 0
#pragma mark -
#pragma mark Public Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Constructor. Called by subclasses.							[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

SATSMARTClient::SATSMARTClient ( void ) : fRefCount ( 1 )
{

    fCFPlugInInterfaceMap.pseudoVTable      = ( IUnknownVTbl * ) &sIOCFPlugInInterface;
    fCFPlugInInterfaceMap.obj                       = this;

    fATASMARTInterfaceMap.pseudoVTable      = ( IUnknownVTbl * ) &sATASMARTInterface;
    fATASMARTInterfaceMap.obj                       = this;

    sFactoryAddRef ( );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Destructor													[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

SATSMARTClient::~SATSMARTClient ( void )
{
    sFactoryRelease ( );
}


#if 0
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ QueryInterface - Called to obtain the presence of an interface
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

HRESULT
SATSMARTClient::QueryInterface ( REFIID iid, void ** ppv )
{

    CFUUIDRef uuid    = CFUUIDCreateFromUUIDBytes ( NULL, iid );
    HRESULT result  = S_OK;

    PRINT ( ( "SATSMARTClient : QueryInterface called\n" ) );

    if ( CFEqual ( uuid, IUnknownUUID ) )
    {

        PRINT ( ( "IUnknownUUID requested\n" ) );

        *ppv = &fCFPlugInInterfaceMap;
        AddRef ( );

    }

    else if (CFEqual ( uuid, kIOCFPlugInInterfaceID ) )
    {

        PRINT ( ( "kIOCFPlugInInterfaceID requested\n" ) );

        *ppv = &fCFPlugInInterfaceMap;
        AddRef ( );

    }

    else if ( CFEqual ( uuid, kIOATASMARTInterfaceID ) )
    {

        PRINT ( ( "kIOATASMARTInterfaceID requested\n" ) );

        *ppv = &fATASMARTInterfaceMap;
        AddRef ( );

    }

    else
    {

        PRINT ( ( "unknown interface requested\n" ) );
        *ppv = 0;
        result = E_NOINTERFACE;

    }

    CFRelease ( uuid );

    return result;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ AddRef	-	Increments refcount associated with the object.	[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

UInt32
SATSMARTClient::AddRef ( void )
{

    fRefCount += 1;
    return fRefCount;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Release	-	Decrements refcount associated with the object, freeing it
//					when the refcount is zero.						[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

UInt32
SATSMARTClient::Release ( void )
{

    UInt32 returnValue = fRefCount - 1;

    if ( returnValue > 0 )
    {
        fRefCount = returnValue;
    }

    else if ( returnValue == 0 )
    {

        fRefCount = returnValue;
        delete this;

    }

    else
    {
        returnValue = 0;
    }

    return returnValue;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Probe -       Called by IOKit to ascertain whether we can drive the provided
//				io_service_t										[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::Probe ( CFDictionaryRef propertyTable,
                        io_service_t inService,
                        SInt32 *                order )
{

    CFMutableDictionaryRef dict    = 0;
    IOReturn status  = kIOReturnBadArgument;

    PRINT ( ( "SATSMARTClient::Probe called\n" ) );

    // Sanity check
    if ( inService == 0 )
    {
        goto Exit;
    }

    status = IORegistryEntryCreateCFProperties ( inService, &dict, NULL, 0 );
    if ( status != kIOReturnSuccess )
    {
        goto Exit;
    }

    if ( !CFDictionaryContainsKey ( dict, CFSTR ( "IOCFPlugInTypes" ) ) )
    {
        goto Exit;
    }


    status = kIOReturnSuccess;


Exit:


    if ( dict != 0 )
    {

        CFRelease ( dict );
        dict = 0;

    }


    PRINT ( ( "SATSMARTClient::Probe called %x\n",status ) );
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Start - Called to start providing our services.				[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::Start ( CFDictionaryRef propertyTable, io_service_t service )
{

    IOReturn status = kIOReturnSuccess;

    PRINT ( ( "SATSMARTClient : Start\n" ) );

    fService = service;
    status = IOServiceOpen ( fService,
        mach_task_self ( ),
        kIOATASMARTLibConnection,
        &fConnection );

    if ( !fConnection )
        status = kIOReturnNoDevice;

    PRINT ( ( "SATSMARTClient : IOServiceOpen status = 0x%08lx, connection = %d\n",
              ( UInt32 ) status, fConnection ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ Stop - Called to stop providing our services.					[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::Stop ( void )
{

    PRINT ( ( "SATSMARTClient : Stop\n" ) );

    if ( fConnection )
    {

        PRINT ( ( "SATSMARTClient : IOServiceClose connection = %d\n", fConnection ) );
        IOServiceClose ( fConnection );
        fConnection = MACH_PORT_NULL;

    }

    return kIOReturnSuccess;

}


#if 0
#pragma mark -
#pragma mark SMART Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTEnableDisableOperations - Enables/Disables SMART operations
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTEnableDisableOperations ( Boolean enable )
{

    IOReturn status          = kIOReturnSuccess;
    uint64_t selection       = ( enable == true ) ? 1 : 0;

    PRINT ( ( "SATSMARTClient::SMARTEnableDisableOperations called\n" ) );

    status = IOConnectCallScalarMethod ( fConnection,
                                        kIOATASMARTEnableDisableOperations,
                                        &selection, 1,
                                        0, 0);
    
    PRINT ( ( "SATSMARTClient::SMARTEnableDisableOperations status = %d\n", status ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTEnableDisableAutosave - Enables/Disables SMART AutoSave
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTEnableDisableAutosave ( Boolean enable )
{

    IOReturn status          = kIOReturnSuccess;
    uint64_t selection       = ( enable == true ) ? 1 : 0;

    PRINT ( ( "SATSMARTClient::SMARTEnableDisableAutosave called\n" ) );

    status = IOConnectCallScalarMethod ( fConnection,
        kIOATASMARTEnableDisableAutoSave,
        &selection, 1,
        0, 0);

    PRINT ( ( "SATSMARTClient::SMARTEnableDisableAutosave status = %d\n", status ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTReturnStatus - Returns SMART status
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTReturnStatus ( Boolean * exceededCondition )
{

    IOReturn status          = kIOReturnSuccess;
    uint64_t condition       = 0;
    uint32_t  outputCnt = 1;

    PRINT ( ( "SATSMARTClient::SMARTReturnStatus called\n" ) );

    status = IOConnectCallScalarMethod ( fConnection,
        kIOATASMARTReturnStatus,
        0, 0, 
        &condition, &outputCnt);

    if ( status == kIOReturnSuccess )
    {

        *exceededCondition = ( condition != 0 ) ? true : false;
        PRINT ( ( "exceededCondition = %ld\n", (long)condition ) );

    }

    PRINT ( ( "SATSMARTClient::SMARTReturnStatus status = %d outputCnt = %d\n", status, (int)outputCnt ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTExecuteOffLineImmediate - Executes an off-line immediate SMART test
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTExecuteOffLineImmediate ( Boolean extendedTest )
{

    IOReturn status          = kIOReturnSuccess;
    uint64_t selection       = ( extendedTest == true ) ? 1 : 0;;

    PRINT ( ( "SATSMARTClient::SMARTExecuteOffLineImmediate called\n" ) );

    status = IOConnectCallScalarMethod ( fConnection,
        kIOATASMARTExecuteOffLineImmediate,
        &selection, 1,
        0, 0);

    PRINT ( ( "SATSMARTClient::SMARTExecuteOffLineImmediate status = %d\n", status ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTReadData - Reads the SMART data
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTReadData ( ATASMARTData * data )
{

    IOReturn status;

    PRINT ( ( "SATSMARTClient::SMARTReadData called\n" ) );

    status = IOConnectCallScalarMethod (        fConnection,
        kIOATASMARTReadData,
        ( uint64_t * ) &data, 1,
        0, 0);

    PRINT ( ( "SATSMARTClient::SMARTReadData status = %d\n", status ) );

        #if 0
    if ( status == kIOReturnSuccess )
    {

        UInt8 *         ptr = ( UInt8 * ) data;

        printf ( "ATA SMART DATA\n" );

        for ( UInt8 index = 0; ( index < sizeof ( ATASMARTData ) ); index += 8 )
        {

            printf ( "0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7] );

        }

    }
        #endif

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTReadDataThresholds - Reads the SMART data thresholds
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTReadDataThresholds ( ATASMARTDataThresholds * data )
{

    IOReturn status;

    PRINT ( ( "SATSMARTClient::SMARTReadDataThresholds called\n" ) );

    status = IOConnectCallScalarMethod (        fConnection,
        kIOATASMARTReadDataThresholds,
        ( uint64_t * ) &data, 1,
        0, 0);

    PRINT ( ( "SATSMARTClient::SMARTReadDataThresholds status = %d\n", status ) );

        #if 0
    if ( status == kIOReturnSuccess )
    {

        UInt8 *         ptr = ( UInt8 * ) data;

        printf ( "ATA SMART DATA THRESHOLDS\n" );

        for ( UInt8 index = 0; ( index < sizeof ( ATASMARTDataThresholds ) ); index += 8 )
        {

            printf ( "0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7] );

        }

    }
        #endif

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTReadLogDirectory - Reads the SMART Log Directory
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTReadLogDirectory ( ATASMARTLogDirectory * log )
{

    IOReturn status;

    status = SMARTReadLogAtAddress ( kATASMARTLogDirectoryEntry,
        ( void * ) log,
        sizeof ( ATASMARTLogDirectory ) );

    PRINT ( ( "SATSMARTClient::SMARTReadLogDirectory status = %d\n", status ) );

        #if 0
    if ( status == kIOReturnSuccess )
    {

        UInt8 *         ptr = ( UInt8 * ) log;

        printf ( "ATA SMART Log Directory\n" );

        for ( UInt8 index = 0; ( index < sizeof ( ATASMARTLogDirectory ) ); index += 8 )
        {

            printf ( "0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7] );

        }

    }
        #endif

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTReadLogAtAddress -       Reads from the SMART Log at specified address
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTReadLogAtAddress ( UInt32 address,
                                        void *          buffer,
                                        UInt32 bufferSize )
{

    IOReturn status;
    IOByteCount byteCount;
    ATASMARTReadLogStruct params;

    PRINT ( ( "SATSMARTClient::SMARTReadLogAtAddress called\n" ) );

    if ( ( address > 0xFF ) || ( buffer == NULL ) )
    {

        status = kIOReturnBadArgument;
        goto Exit;

    }

    byteCount = sizeof ( ATASMARTReadLogStruct );

    params.numSectors       = bufferSize / kATADefaultSectorSize;
    params.logAddress       = address & 0xFF;
    params.buffer           = buffer;
    params.bufferSize       = bufferSize;

    // Can't read or write more than 16 sectors
    if ( params.numSectors > 16 )
    {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    PRINT ( ( "SATSMARTClient::SMARTReadLogAtAddress address = %ld\n", address ) );

    status = IOConnectCallStructMethod (  fConnection,
        kIOATASMARTReadLogAtAddress,
        ( void * ) &params,  byteCount,
        0, 0);

Exit:


    PRINT ( ( "SATSMARTClient::SMARTReadLogAtAddress status = %d\n", status ) );

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SMARTWriteLogAtAddress - Writes to the SMART Log at specified address
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::SMARTWriteLogAtAddress ( UInt32 address,
                                         const void *   buffer,
                                         UInt32 bufferSize )
{

    IOReturn status;
    IOByteCount byteCount;
    ATASMARTWriteLogStruct params;

    PRINT ( ( "SATSMARTClient::SMARTWriteLogAtAddress called\n" ) );

    if ( ( address > 0xFF ) || ( buffer == NULL ) )
    {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    byteCount = sizeof ( ATASMARTWriteLogStruct );

    params.numSectors       = bufferSize / kATADefaultSectorSize;
    params.logAddress       = address & 0xFF;
    params.buffer           = buffer;
    params.bufferSize       = bufferSize;

    // Can't read or write more than 16 sectors
    if ( params.numSectors > 16 )
    {

        status = kIOReturnBadArgument;
        goto Exit;

    }

    PRINT ( ( "SATSMARTClient::SMARTWriteLogAtAddress address = %ld\n", address ) );

    status = IOConnectCallStructMethod (  fConnection,
                                         kIOATASMARTWriteLogAtAddress,
                                        ( void * ) &params , byteCount,
                                        0, 0);

Exit:


    PRINT ( ( "SATSMARTClient::SMARTWriteLogAtAddress status = %d\n", status ) );

    return status;

}


#if 0
#pragma mark -
#pragma mark Additional Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ GetATAIdentifyData - Gets ATA Identify Data.					[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::GetATAIdentifyData ( void * buffer, UInt32 inSize, UInt32 * outSize )
{

    IOReturn status                          = kIOReturnBadArgument;
    size_t byteCount                       = 0;
    ATAGetIdentifyDataStruct params                          = { 0 };
    UInt32 bytesTransferred        = 0;

    if ( ( buffer == NULL ) || ( inSize > 512 ) || ( inSize == 0 ) )
    {

        status = kIOReturnBadArgument;
        goto Exit;

    }

    byteCount = sizeof ( UInt32 );

    params.buffer           = buffer;
    params.bufferSize       = inSize;

    PRINT ( ( "SATSMARTClient::GetATAIdentifyData\n" ) );

    status = IOConnectCallStructMethod ( fConnection,
                                        kIOATASMARTGetIdentifyData,
                                        ( void * ) &params,
                                        sizeof ( ATAGetIdentifyDataStruct ),
                                        ( void * ) &bytesTransferred, 
                                        &byteCount
                                        );
    
    if ( outSize != NULL )
    {
        *outSize = bytesTransferred;
    }


Exit:


    PRINT ( ( "SATSMARTClient::GetATAIdentifyData status = %d\n", status ) );

    return status;

}


#if 0
#pragma mark -
#pragma mark Static C->C++ Glue Functions
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sQueryInterface - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

HRESULT
SATSMARTClient::sQueryInterface ( void * self, REFIID iid, void ** ppv )
{

    SATSMARTClient *        obj = ( ( InterfaceMap * ) self )->obj;
    return obj->QueryInterface ( iid, ppv );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sAddRef - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

UInt32
SATSMARTClient::sAddRef ( void * self )
{

    SATSMARTClient *        obj = ( ( InterfaceMap * ) self )->obj;
    return obj->AddRef ( );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sRelease - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

UInt32
SATSMARTClient::sRelease ( void * self )
{

    SATSMARTClient *        obj = ( ( InterfaceMap * ) self )->obj;
    return obj->Release ( );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sProbe - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sProbe ( void *                         self,
                         CFDictionaryRef propertyTable,
                         io_service_t service,
                         SInt32 *                       order )
{
    return getThis ( self )->Probe ( propertyTable, service, order );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sStart - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sStart ( void *                         self,
                         CFDictionaryRef propertyTable,
                         io_service_t service )
{
    return getThis ( self )->Start ( propertyTable, service );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sStop - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sStop ( void * self )
{
    return getThis ( self )->Stop ( );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTEnableDisableOperations - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTEnableDisableOperations ( void * self, Boolean enable )
{
    return getThis ( self )->SMARTEnableDisableOperations ( enable );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTEnableDisableAutosave - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTEnableDisableAutosave ( void * self, Boolean enable )
{
    return getThis ( self )->SMARTEnableDisableAutosave ( enable );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTReturnStatus - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTReturnStatus ( void * self, Boolean * exceededCondition )
{
    return getThis ( self )->SMARTReturnStatus ( exceededCondition );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTExecuteOffLineImmediate - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTExecuteOffLineImmediate ( void * self, Boolean extendedTest )
{
    return getThis ( self )->SMARTExecuteOffLineImmediate ( extendedTest );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTReadData - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTReadData ( void * self, ATASMARTData * data )
{
    return getThis ( self )->SMARTReadData ( data );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTValidateReadData - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTValidateReadData ( void * self, const ATASMARTData * data )
{

    SInt8 checksum        = 0;
    UInt32 index           = 0;
    SInt8 *         ptr                     = ( SInt8 * ) data;
    IOReturn status          = kIOReturnError;

    PRINT ( ( "sSMARTValidateReadData called\n" ) );

    // Checksum the 511 bytes of the structure;
    for ( index = 0; index < ( sizeof ( ATASMARTData ) - 1 ); index++ )
    {

        checksum += ptr[index];

    }

    PRINT ( ( "Checksum = %d\n", checksum ) );
    PRINT ( ( "ptr[511] = %d\n", ptr[511] ) );

    if ( ( checksum + ptr[511] ) == 0 )
    {

        PRINT ( ( "Checksum is valid\n" ) );
        status = kIOReturnSuccess;

    }

    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTReadDataThresholds - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTReadDataThresholds (
    void *                                          self,
    ATASMARTDataThresholds *        data )
{
    return getThis ( self )->SMARTReadDataThresholds ( data );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTReadLogDirectory - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTReadLogDirectory ( void * self, ATASMARTLogDirectory * log )
{
    return getThis ( self )->SMARTReadLogDirectory ( log );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTReadLogAtAddress - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTReadLogAtAddress ( void *         self,
                                         UInt32 address,
                                         void *         buffer,
                                         UInt32 size )
{
    return getThis ( self )->SMARTReadLogAtAddress ( address, buffer, size );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sSMARTWriteLogAtAddress - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sSMARTWriteLogAtAddress ( void *                self,
                                          UInt32 address,
                                          const void *  buffer,
                                          UInt32 size )
{
    return getThis ( self )->SMARTWriteLogAtAddress ( address, buffer, size );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sGetATAIdentifyData - Static function for C->C++ glue
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTClient::sGetATAIdentifyData ( void * self, void * buffer, UInt32 inSize, UInt32 * outSize )
{
    return getThis ( self )->GetATAIdentifyData ( buffer, inSize, outSize );
}