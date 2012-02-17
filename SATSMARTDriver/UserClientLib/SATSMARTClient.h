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

#ifndef __SAT_SMART_CLIENT_H__
#define __SAT_SMART_CLIENT_H__

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Includes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// IOKit includes
#include <IOKit/IOCFPlugIn.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void * SATSMARTLibFactory ( CFAllocatorRef allocator, CFUUIDRef typeID );

#ifdef __cplusplus
}
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Class Declarations
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

class SATSMARTClient
{

public:

// Default constructor
SATSMARTClient ( );

typedef struct InterfaceMap
{
    IUnknownVTbl *          pseudoVTable;
    SATSMARTClient *        obj;
} InterfaceMap;

private:

// Disable copy constructor
SATSMARTClient ( SATSMARTClient& src );
void operator = ( SATSMARTClient& src );

protected:

// utility function to get "this" pointer from interface
static inline SATSMARTClient * getThis ( void * self )
{
    return ( SATSMARTClient * ) ( ( InterfaceMap * ) self )->obj;
};

// Static functions (C->C++ Glue Code)
static UInt32 sFactoryRefCount;
static void                     sFactoryAddRef ( void );
static void                     sFactoryRelease ( void );

static HRESULT          sQueryInterface ( void * self, REFIID iid, void ** ppv );
static UInt32           sAddRef ( void * self );
static UInt32           sRelease ( void * self );

static IOReturn         sProbe ( void * self, CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
static IOReturn         sStart ( void * self, CFDictionaryRef propertyTable, io_service_t service );
static IOReturn         sStop ( void * self );

// Destructor
virtual ~SATSMARTClient ( void );

virtual IOReturn        Probe ( CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
virtual IOReturn        Start ( CFDictionaryRef propertyTable, io_service_t service );
virtual IOReturn        Stop ( void );

static IOReturn        sSMARTEnableDisableOperations ( void * interface, Boolean enable );
static IOReturn        sSMARTEnableDisableAutosave ( void * interface, Boolean enable );
static IOReturn        sSMARTReturnStatus ( void * interface, Boolean * exceededCondition );
static IOReturn        sSMARTExecuteOffLineImmediate ( void * interface, Boolean extendedTest );
static IOReturn        sSMARTReadData ( void * interface, ATASMARTData * data );
static IOReturn        sSMARTValidateReadData ( void * interface, const ATASMARTData * data );
static IOReturn        sSMARTReadDataThresholds ( void * interface, ATASMARTDataThresholds * data );
static IOReturn        sSMARTReadLogDirectory ( void * interface, ATASMARTLogDirectory * logData );
static IOReturn        sSMARTReadLogAtAddress ( void * interface, UInt32 address, void * buffer, UInt32 size );
static IOReturn        sSMARTWriteLogAtAddress ( void * interface, UInt32 address, const void * buffer, UInt32 size );

static IOReturn        sGetATAIdentifyData ( void * interface, void * buffer, UInt32 inSize, UInt32 * outSize );

IOReturn        SMARTEnableDisableOperations ( Boolean enable );
IOReturn        SMARTEnableDisableAutosave ( Boolean enable );
IOReturn        SMARTReturnStatus ( Boolean * exceededCondition );
IOReturn        SMARTExecuteOffLineImmediate ( Boolean extendedTest );
IOReturn        SMARTReadData ( ATASMARTData * data );
IOReturn        SMARTReadDataThresholds ( ATASMARTDataThresholds * data );
IOReturn        SMARTReadLogDirectory ( ATASMARTLogDirectory * logData );
IOReturn        SMARTReadLogAtAddress ( UInt32 address, void * buffer, UInt32 size );
IOReturn        SMARTWriteLogAtAddress ( UInt32 address, const void * buffer, UInt32 size );

IOReturn        GetATAIdentifyData ( void * buffer, UInt32 inSize, UInt32 * outSize );

static IOCFPlugInInterface sIOCFPlugInInterface;
static IOATASMARTInterface sATASMARTInterface;

UInt32 fRefCount;
InterfaceMap fCFPlugInInterfaceMap;
InterfaceMap fATASMARTInterfaceMap;
io_service_t fService;
io_connect_t fConnection;

public:

// Static allocation methods
static IOCFPlugInInterface **           alloc ( void );

// Subclasses must add this method.
virtual HRESULT QueryInterface ( REFIID iid, void ** ppv );
virtual UInt32  AddRef ( void );
virtual UInt32  Release ( void );

};

#endif  /* __ATA_SMART_CLIENT_H__ */