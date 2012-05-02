/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * Copyright (c) 2002-2003 Apple Computer, Inc. All rights reserved.
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

#ifndef _IOKIT_SAT_SMART_USER_CLIENT_H_
#define _IOKIT_SAT_SMART_USER_CLIENT_H_


#if defined(KERNEL) && defined(__cplusplus)


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Includes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// IOKit includes
#include <IOKit/IOLib.h>
#include <IOKit/IOUserClient.h>

// Private includes
//#include <storage/ata/ATASMARTLib.h>
#include "../UserClientLib/SATSMARTLibPriv.h"
#include "IOSATServices.h"

#include "IOSATCommand.h"

// Forward class declaration
class ATASMARTReadLogStruct;


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Typedefs
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

struct SATSMARTRefCon
{
    class SATSMARTUserClient *      self;
    bool isDone;
    bool sleepOnIt;
};
typedef struct SATSMARTRefCon SATSMARTRefCon;


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Class Declarations
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


class SATSMARTUserClient : public IOUserClient
{

OSDeclareDefaultStructors ( SATSMARTUserClient );

public:

virtual bool     initWithTask           ( task_t owningTask, void * securityToken, UInt32 type );

virtual bool         init                           ( OSDictionary * dictionary = 0 );
virtual bool         start                          ( IOService * provider );
virtual void     free                           ( void );
virtual IOReturn message                    ( UInt32 type, IOService * provider, void * arg );

virtual IOReturn clientClose                ( void );

IOReturn        EnableDisableOperations ( UInt32 enable );
IOReturn        EnableDisableAutoSave   ( UInt32 enable );
IOReturn        ReturnStatus                    ( UInt32 * exceedsCondition );
IOReturn        ExecuteOfflineImmediate ( UInt32 extendedTest );
    IOReturn        ReadData                                ( UInt32 * dataOut,
                                                             IOByteCount * outputSize );
    IOReturn        ReadDataThresholds              ( UInt32 * dataOut,
                                                     IOByteCount * outputSize );
    IOReturn        ReadLogAtAddress                ( ATASMARTReadLogStruct * structIn,
                                                     void * structOut,
                                                     IOByteCount inStructSize,
                                                     IOByteCount *outStructSize );
IOReturn        WriteLogAtAddress               ( ATASMARTWriteLogStruct *              writeLogData,
                                                  UInt32 inStructSize );
 IOReturn        GetIdentifyData                 (UInt32 * dataOut, IOByteCount * outputSize);

protected:

static IOExternalMethod sMethods[kIOATASMARTMethodCount];

static IOReturn         sWaitForCommand         ( void * userClient, IOSATCommand * command );
static void             sCommandCallback        ( IOSATCommand * command );

IOReturn                        GatedWaitForCommand     ( IOSATCommand * command );
void                            CommandCallback         ( IOSATCommand * command );

task_t fTask;
IOSATServices *                 fProvider;
IOCommandGate *                                         fCommandGate;
IOWorkLoop *                                            fWorkLoop;
UInt32 fOutstandingCommands;

virtual IOExternalMethod *                      getTargetAndMethodForIndex ( IOService **       target,
                                                                             UInt32 index );

IOReturn                HandleTerminate         ( IOService * provider );
IOReturn                SendSMARTCommand        ( IOSATCommand * command );
IOSATCommand *  AllocateCommand         ( void );
void                    DeallocateCommand       ( IOSATCommand * command );

};


#endif  /* defined(KERNEL) && defined(__cplusplus) */

#endif /* ! _IOKIT_SAT_SMART_USER_CLIENT_H_ */
