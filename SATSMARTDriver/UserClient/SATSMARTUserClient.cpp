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

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Includes
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// Private includes
#include "SATSMARTUserClient.h"
///Developer/SDKs/MacOSX10.6.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/
#include <storage/ata/ATASMARTLib.h>
//#include <storage/ata/ATASMARTLib.h>
#include "../UserClientLib/SATSMARTLibPriv.h"
//#include "ATASMARTLibPriv.h"

// IOKit includes
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOWorkLoop.h>

// IOKit ATA includes
#include <IOKit/ata/IOATADevice.h>
#include <IOKit/ata/IOATATypes.h>
#include "IOSATCommand.h"
#include "IOSATBusCommand.h"

#include "IOSATServices.h"

#ifdef DEBUG
#define DEBUG_LOG IOLog
#else
#define DEBUG_LOG(...)
#endif

#define ERROR_LOG IOLog

#define getClassName() "SATSMARTUserClient"

#define super IOUserClient
OSDefineMetaClassAndStructors ( SATSMARTUserClient, IOUserClient );


#if 0
#pragma mark -
#pragma mark Constants
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Enums
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

// S.M.A.R.T command code
enum
{
    kATAcmdSMART    = 0xB0
};

// S.M.A.R.T command features register codes
enum
{
    kFeaturesRegisterReadData                                       = 0xD0,
    kFeaturesRegisterReadDataThresholds                     = 0xD1,
    kFeaturesRegisterEnableDisableAutoSave          = 0xD2,
    // Reserved
    kFeaturesRegisterExecuteOfflineImmed            = 0xD4,
    kFeaturesRegisterReadLogAtAddress                       = 0xD5,
    kFeaturesRegisterWriteLogAtAddress                      = 0xD6,
    // Reserved
    kFeaturesRegisterEnableOperations                       = 0xD8,
    kFeaturesRegisterDisableOperations                      = 0xD9,
    kFeaturesRegisterReturnStatus                           = 0xDA
};

// S.M.A.R.T 'magic' values
enum
{
    kSMARTMagicCylinderLoValue      = 0x4F,
    kSMARTMagicCylinderHiValue      = 0xC2
};

// S.M.A.R.T Return Status validity values
enum
{
    kSMARTReturnStatusValidLoValue  = 0xF4,
    kSMARTReturnStatusValidHiValue  = 0x2C
};

// S.M.A.R.T Auto-Save values
enum
{
    kSMARTAutoSaveEnable    = 0xF1,
    kSMARTAutoSaveDisable   = 0x00
};

enum
{
    kATAThirtySecondTimeoutInMS     = 30000
};


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constants
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOExternalMethod
SATSMARTUserClient::sMethods[kIOATASMARTMethodCount] =
{
    {
        // Method #0 EnableDisableOperations
        0,
        ( IOMethod ) &SATSMARTUserClient::EnableDisableOperations,
        kIOUCScalarIScalarO,
        1,
        0
    },
    {
        // Method #1 EnableDisableAutoSave
        0,
        ( IOMethod ) &SATSMARTUserClient::EnableDisableAutoSave,
        kIOUCScalarIScalarO,
        1,
        0
    },
    {
        // Method #2 ReturnStatus
        0,
        ( IOMethod ) &SATSMARTUserClient::ReturnStatus,
        kIOUCScalarIScalarO,
        0,
        1
    },
    {
        // Method #3 ExecuteOfflineImmediate
        0,
        ( IOMethod ) &SATSMARTUserClient::ExecuteOfflineImmediate,
        kIOUCScalarIScalarO,
        1,
        0
    },
    {
        // Method #4 ReadData
        0,
        ( IOMethod ) &SATSMARTUserClient::ReadData,
        kIOUCScalarIScalarO,
        1,
        0
    },
    {
        // Method #5 ReadDataThresholds
        0,
        ( IOMethod ) &SATSMARTUserClient::ReadDataThresholds,
        kIOUCScalarIScalarO,
        1,
        0
    },
    {
        // Method #6 ReadLogAtAddress
        0,
        ( IOMethod ) &SATSMARTUserClient::ReadLogAtAddress,
        kIOUCStructIStructO,
        sizeof ( ATASMARTReadLogStruct ),
        kIOUCVariableStructureSize
    },
    {
        // Method #7 WriteLogAtAddress
        0,
        ( IOMethod ) &SATSMARTUserClient::WriteLogAtAddress,
        kIOUCScalarIStructI,
        0,
        sizeof ( ATASMARTWriteLogStruct )
    },
    {
        // Method #8 GetIdentifyData
        0,
        ( IOMethod ) &SATSMARTUserClient::GetIdentifyData,
        kIOUCStructIStructO,
        0,
        kATADefaultSectorSize
    }

};


#if 0
#pragma mark -
#pragma mark Public Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ init - Initializes member variables							[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


bool
SATSMARTUserClient::init ( OSDictionary * dictionary )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    if ( !super::init ( dictionary ) )
        return false;

    fTask                                   = NULL;
    fProvider                               = NULL;
    fOutstandingCommands    = 0;

    return true;
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ start - Start providing services								[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

bool
SATSMARTUserClient::start ( IOService * provider )
{
    IOWorkLoop *    workLoop = NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    if ( fProvider != NULL )
    {

        ERROR_LOG ( "fProvider != NULL, returning false\n" );
        return false;

    }

    fProvider = OSDynamicCast ( IOSATServices, provider );
    if ( fProvider == NULL )
    {

        ERROR_LOG ( "Provider not IOSATServices\n" );
        return false;

    }

    if ( !super::start ( provider ) )
    {

        ERROR_LOG ( "super rejected provider in start\n" );
        return false;

    }

    fCommandGate = IOCommandGate::commandGate ( this );
    if ( fCommandGate == NULL )
    {

        ERROR_LOG ( "Command gate creation failed\n" );
        return false;

    }

    workLoop = getWorkLoop ( );
    if ( workLoop == NULL )
    {

        ERROR_LOG ( "workLoop == NULL\n" );
        fCommandGate->release ( );
        fCommandGate = NULL;
        return false;

    }

    workLoop->addEventSource ( fCommandGate );

    if ( !fProvider->open ( this, kIOATASMARTUserClientAccessMask, 0 ) )
    {

        ERROR_LOG ( "Open failed\n" );
        fCommandGate->release ( );
        fCommandGate = NULL;
        return false;

    }

    fWorkLoop = workLoop;

    // Yes, we found an object to use as our interface
    return true;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ initWithTask - Save task_t and validate the connection type	[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

bool
SATSMARTUserClient::initWithTask ( task_t owningTask,
                                   void *       securityToken,
                                   UInt32 type )
{
    DEBUG_LOG("%s[%p]::%s task = %p\n", getClassName(), this, __FUNCTION__, owningTask);

    if ( type != kIOATASMARTLibConnection )
        return false;

    fTask = owningTask;
    return true;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ clientClose - Close down services.							[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::clientClose ( void )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    if ( fProvider != NULL )
        HandleTerminate ( fProvider );

    return super::clientClose ( );
}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ free - Releases any items we need to release.					[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTUserClient::free ( void )
{

    // Remove the command gate from the workloop
    if ( fWorkLoop != NULL )
    {

        fWorkLoop->removeEventSource ( fCommandGate );
        fWorkLoop = NULL;

    }

    // Release the command gate
    if ( fCommandGate != NULL )
    {

        fCommandGate->release ( );
        fCommandGate = NULL;

    }

    super::free ( );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ message - Handles termination messages.						[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::message ( UInt32 type, IOService * provider, void * arg )
{
    IOReturn status = kIOReturnSuccess;

    DEBUG_LOG("%s[%p]::%s type = %u, provider = %p\n", getClassName(), this, __FUNCTION__, (unsigned int)type, provider );

    switch ( type )
    {

    case kIOMessageServiceIsRequestingClose:
        break;

    case kIOMessageServiceIsTerminated:
        status = HandleTerminate ( provider );
        break;

    default:
        status = super::message ( type, provider, arg );
        break;
    }

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ EnableDisableOperations - Enables/Disables SMART operations.	[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::EnableDisableOperations ( UInt32 enable )
{

    IOReturn status = kIOReturnSuccess;
    IOSATCommand *  command;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    if ( enable == 0 )
    {

        // They want to disable SMART operations.
        command->setFeatures ( kFeaturesRegisterDisableOperations );

    }

    else
    {

        // The want to enable SMART operations.
        command->setFeatures ( kFeaturesRegisterEnableOperations );

    }

    command->setOpcode              ( kATAFnExecIO );
    command->setTimeoutMS   ( kATAThirtySecondTimeoutInMS );
    command->setCylLo               ( kSMARTMagicCylinderLoValue );
    command->setCylHi               ( kSMARTMagicCylinderHiValue );
    command->setCommand             ( kATAcmdSMART );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "Enable/Disable unsupported\n" );
            status = kIOReturnUnsupported;

        }

    }

    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ EnableDisableAutoSave - Enables/Disables SMART autosave.		[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::EnableDisableAutoSave ( UInt32 enable )
{

    IOReturn status = kIOReturnSuccess;
    IOSATCommand *  command;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    if ( enable == 0 )
    {

        // They want to disable SMART autosave operations.
        command->setSectorCount ( kSMARTAutoSaveDisable );

    }

    else
    {

        // They want to enable SMART autosave operations.
        command->setSectorCount ( kSMARTAutoSaveEnable );

    }

    command->setFeatures    ( kFeaturesRegisterEnableDisableAutoSave );
    command->setOpcode              ( kATAFnExecIO );
    command->setTimeoutMS   ( kATAThirtySecondTimeoutInMS );
    command->setCylLo               ( kSMARTMagicCylinderLoValue );
    command->setCylHi               ( kSMARTMagicCylinderHiValue );
    command->setCommand             ( kATAcmdSMART );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "Enable/Disable autosave unsupported\n" );
            status = kIOReturnUnsupported;

        }

    }

    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ ReturnStatus - Returns SMART status.							[PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::ReturnStatus ( UInt32 * exceededCondition )
{

    IOReturn status  = kIOReturnSuccess;
    IOSATCommand *  command = NULL;
    UInt8 lbaMid  = kSMARTMagicCylinderLoValue;
    UInt8 lbaHigh = kSMARTMagicCylinderHiValue;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    command->setFeatures    ( kFeaturesRegisterReturnStatus );
    command->setOpcode              ( kATAFnExecIO );
    command->setTimeoutMS   ( kATAThirtySecondTimeoutInMS );
    command->setCylLo               ( lbaMid );
    command->setCylHi               ( lbaHigh );
    command->setCommand             ( kATAcmdSMART );
    command->setRegMask             ( ( ataRegMask ) ( mATACylinderHiValid | mATACylinderLoValid ) );
    command->setFlags               ( mATAFlagTFAccessResult );

    status = SendSMARTCommand ( command );

    lbaMid  = command->getCylLo ( );
    lbaHigh = command->getCylHi ( );

    if ( status == kIOReturnSuccess )
    {

        // Check if threshold exceeded
        if ( ( lbaMid == kSMARTReturnStatusValidLoValue ) &&
             ( lbaHigh == kSMARTReturnStatusValidHiValue ) )
        {
            *exceededCondition = 1;
        }

        else
        {
            *exceededCondition = 0;
        }

    }

    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "Return Status unsupported\n" );
            status = kIOReturnUnsupported;

        }

    }

    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ ExecuteOfflineImmediate - Executes self test offline immediately. [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::ExecuteOfflineImmediate ( UInt32 extendedTest )
{

    IOReturn status  = kIOReturnSuccess;
    IOSATCommand *  command = NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    command->setFeatures            ( kFeaturesRegisterExecuteOfflineImmed );
    command->setOpcode                      ( kATAFnExecIO );
    command->setTimeoutMS           ( kATAThirtySecondTimeoutInMS );
    command->setSectorNumber        ( ( extendedTest == 0 ) ? 0x01 : 0x02 );
    command->setCylLo                       ( kSMARTMagicCylinderLoValue );
    command->setCylHi                       ( kSMARTMagicCylinderHiValue );
    command->setCommand                     ( kATAcmdSMART );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "Execute Offline Immediate unsupported\n" );
            status = kIOReturnUnsupported;

        }

    }

    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ ReadData - Reads SMART data.                                                                     [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::ReadData ( vm_address_t data )
{

    IOReturn status  = kIOReturnSuccess;
    IOSATCommand *                  command = NULL;
    IOMemoryDescriptor *    buffer  = NULL;
    DEBUG_LOG("%s[%p]::%s data = %p\n", getClassName(), this, __FUNCTION__, (void*)data);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {
        status = kIOReturnNoResources;
        goto ReleaseProvider;
    }

    buffer = IOMemoryDescriptor::withAddressRange (      data,
                                              sizeof ( ATASMARTData ),
                                              kIODirectionIn,
                                              fTask );
    
    if ( buffer == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseCommand;

    }

    status = buffer->prepare ( );
    DEBUG_LOG("%s[%p]::%s prepare %p\n", getClassName(), this,  __FUNCTION__, (void*)status);
    if ( status != kIOReturnSuccess )
    {

        goto ReleaseBuffer;

    }

    command->setBuffer                      ( buffer );
    command->setByteCount           ( sizeof ( ATASMARTData ) );
    command->setFeatures            ( kFeaturesRegisterReadData );
    command->setOpcode                      ( kATAFnExecIO );
    command->setTimeoutMS           ( kATAThirtySecondTimeoutInMS );
    command->setCylLo                       ( kSMARTMagicCylinderLoValue );
    command->setCylHi                       ( kSMARTMagicCylinderHiValue );
    command->setCommand                     ( kATAcmdSMART );
    command->setFlags                       ( mATAFlagIORead );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "ReadData unsupported\n" );
            status = kIOReturnUnsupported;

        }

        if ( command->getEndErrorReg ( ) & 0x10 )
        {

            ERROR_LOG ( "ReadData Not readable\n" );
            status = kIOReturnNotReadable;

        }

    }

    buffer->complete ( );


ReleaseBuffer:


    buffer->release ( );
    buffer = NULL;


ReleaseCommand:


    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ ReadDataThresholds - Reads SMART data thresholds.                        [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::ReadDataThresholds ( vm_address_t data )
{

    IOReturn status  = kIOReturnSuccess;
    IOSATCommand *                  command = NULL;
    IOMemoryDescriptor *    buffer  = NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    buffer = IOMemoryDescriptor::withAddressRange (      data,
        sizeof ( ATASMARTDataThresholds ),
        kIODirectionIn,
        fTask );

    if ( buffer == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseCommand;

    }

    status = buffer->prepare ( );
    if ( status != kIOReturnSuccess )
    {

        goto ReleaseBuffer;

    }

    command->setBuffer                      ( buffer );
    command->setByteCount           ( sizeof ( ATASMARTDataThresholds ) );
    command->setFeatures            ( kFeaturesRegisterReadDataThresholds );
    command->setOpcode                      ( kATAFnExecIO );
    command->setTimeoutMS           ( kATAThirtySecondTimeoutInMS );
    command->setCylLo                       ( kSMARTMagicCylinderLoValue );
    command->setCylHi                       ( kSMARTMagicCylinderHiValue );
    command->setCommand                     ( kATAcmdSMART );
    command->setFlags                       ( mATAFlagIORead );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "ReadDataThresholds unsupported\n" );
            status = kIOReturnUnsupported;

        }

        if ( command->getEndErrorReg ( ) & 0x10 )
        {

            ERROR_LOG ( "ReadDataThresholds Not readable\n" );
            status = kIOReturnNotReadable;

        }

    }

    buffer->complete ( );


ReleaseBuffer:


    buffer->release ( );
    buffer = NULL;


ReleaseCommand:


    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ ReadLogAtAddress - Reads the SMART log at the specified address. [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::ReadLogAtAddress ( ATASMARTReadLogStruct * structIn,
                                      void * structOut,
                                      IOByteCount inStructSize,
                                      IOByteCount *outStructSize)
{

    IOReturn status                  = kIOReturnSuccess;
    IOSATCommand *                  command                 = NULL;
    //IOMemoryDescriptor *    buffer                  = NULL;
    IOBufferMemoryDescriptor * buffer =NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    DEBUG_LOG("%s[%p]::%s %p(%ld) %p(%ld)\n", getClassName(), this, __FUNCTION__, structIn, (long)inStructSize, structOut, (long)(outStructSize));

    if ( inStructSize != sizeof ( ATASMARTReadLogStruct )  || !outStructSize || *outStructSize < 1) {
        return kIOReturnBadArgument;
    }

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    buffer = IOBufferMemoryDescriptor::withCapacity ( *outStructSize, kIODirectionIn, false );
    if ( buffer == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseCommand;

    }

    status = buffer->prepare ( );
    DEBUG_LOG("%s[%p]::%s status %p\n", getClassName(), this, __FUNCTION__, (void *)status);
    if ( status != kIOReturnSuccess )
    {

        goto ReleaseBuffer;

    }
    
    command->setBuffer                      ( buffer );
    command->setByteCount           ( buffer->getLength());
    command->setFeatures            ( kFeaturesRegisterReadLogAtAddress );
    command->setOpcode                      ( kATAFnExecIO );
    command->setTimeoutMS           ( kATAThirtySecondTimeoutInMS );
    command->setSectorCount         ( structIn->numSectors );
    command->setSectorNumber        ( structIn->logAddress );
    command->setCylLo                       ( kSMARTMagicCylinderLoValue );
    command->setCylHi                       ( kSMARTMagicCylinderHiValue );
    command->setCommand                     ( kATAcmdSMART );
    command->setFlags                       ( mATAFlagIORead );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "ReadLogAtAddress %d unsupported\n", structIn->logAddress );
            status = kIOReturnUnsupported;

        }

        if ( command->getEndErrorReg ( ) & 0x10 )
        {

            ERROR_LOG ( "ReadLogAtAddress %d unreadable\n", structIn->logAddress );
            status = kIOReturnNotReadable;

        }

    }

    memcpy(structOut, buffer->getBytesNoCopy ( ), buffer->getLength());
    *outStructSize = buffer->getLength();
    
    buffer->complete ( );
    


ReleaseBuffer:


    buffer->release ( );
    buffer = NULL;


ReleaseCommand:


    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ WriteLogAtAddress - Writes the SMART log at the specified address. [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::WriteLogAtAddress ( ATASMARTWriteLogStruct *        writeLogData,
                                        UInt32 inStructSize )
{

    IOReturn status                  = kIOReturnSuccess;
    IOSATCommand *                  command                 = NULL;
    //IOMemoryDescriptor *    buffer                  = NULL;
    IOBufferMemoryDescriptor * buffer = NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    if ( inStructSize != sizeof ( ATASMARTWriteLogStruct ) )
        return kIOReturnBadArgument;

    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    buffer = IOBufferMemoryDescriptor::withBytes (writeLogData->buffer, writeLogData->bufferSize, kIODirectionOut, false );
    
    if ( buffer == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseCommand;

    }

    status = buffer->prepare ( );
    if ( status != kIOReturnSuccess )
    {

        goto ReleaseBuffer;

    }

    command->setBuffer                      ( buffer );
    command->setByteCount           ( writeLogData->bufferSize );
    command->setFeatures            ( kFeaturesRegisterWriteLogAtAddress );
    command->setOpcode                      ( kATAFnExecIO );
    command->setTimeoutMS           ( kATAThirtySecondTimeoutInMS );
    command->setSectorCount         ( writeLogData->numSectors );
    command->setSectorNumber        ( writeLogData->logAddress );
    command->setCylLo                       ( kSMARTMagicCylinderLoValue );
    command->setCylHi                       ( kSMARTMagicCylinderHiValue );
    command->setCommand                     ( kATAcmdSMART );
    command->setFlags                       ( mATAFlagIOWrite );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnIOError )
    {

        if ( command->getEndErrorReg ( ) & 0x04 )
        {

            ERROR_LOG ( "WriteLogAtAddress %d unsupported\n", writeLogData->logAddress );
            status = kIOReturnUnsupported;

        }

        if ( command->getEndErrorReg ( ) & 0x10 )
        {

            ERROR_LOG ( "WriteLogAtAddress %d unwriteable\n", writeLogData->logAddress );
            status = kIOReturnNotWritable;

        }

    }

    buffer->complete ( );


ReleaseBuffer:


    buffer->release ( );
    buffer = NULL;


ReleaseCommand:


    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ GetIdentifyData - Gets identify data.							   [PUBLIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::GetIdentifyData (UInt32 * dataOut,
				     IOByteCount * outputSize)
{

    IOReturn status                  = kIOReturnSuccess;
    IOSATCommand *                          command                 = NULL;
    IOBufferMemoryDescriptor *      buffer                  = NULL;
    UInt8 *                                         identifyDataPtr = NULL;
    DEBUG_LOG("%s[%p]::%s %p(%ld)\n", getClassName(), this, __FUNCTION__, dataOut, (long)(outputSize));

    if (!outputSize || *outputSize < kATADefaultSectorSize ) {
        return kIOReturnBadArgument;
    }
        
    fOutstandingCommands++;

    if ( isInactive ( ) )
    {

        status = kIOReturnNoDevice;
        goto ErrorExit;

    }

    fProvider->retain ( );

    command = AllocateCommand ( );
    if ( command == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseProvider;

    }

    buffer = IOBufferMemoryDescriptor::withCapacity ( kATADefaultSectorSize, kIODirectionIn, false );
    if ( buffer == NULL )
    {

        status = kIOReturnNoResources;
        goto ReleaseCommand;

    }

    identifyDataPtr = ( UInt8 * ) buffer->getBytesNoCopy ( );

    status = buffer->prepare ( );
    if ( status != kIOReturnSuccess )
    {

        goto ReleaseBuffer;

    }
    
    command->setBuffer                              ( buffer );
    command->setByteCount                   ( kATADefaultSectorSize );
    command->setTransferChunkSize   ( kATADefaultSectorSize );
    command->setOpcode                              ( kATAFnExecIO );
    command->setTimeoutMS                   ( kATAThirtySecondTimeoutInMS );
    command->setCommand                             ( kATAcmdDriveIdentify );
    command->setFlags                               ( mATAFlagIORead );
    command->setRegMask                             ( ( ataRegMask ) ( mATAErrFeaturesValid | mATAStatusCmdValid ) );

    status = SendSMARTCommand ( command );
    if ( status == kIOReturnSuccess )
    {

        UInt8 *         bufferToCopy = identifyDataPtr;

                #if defined(__BIG_ENDIAN__)

        // The identify device info needs to be byte-swapped on big-endian (ppc)
        // systems becuase it is data that is produced by the drive, read across a
        // 16-bit little-endian PCI interface, directly into a big-endian system.
        // Regular data doesn't need to be byte-swapped because it is written and
        // read from the host and is intrinsically byte-order correct.

        IOByteCount index;
        UInt8 temp;
        UInt8 *                 firstBytePtr;

        for ( index = 0; index < buffer->getLength ( ); index += 2 )
        {

            firstBytePtr            = identifyDataPtr;                          // save pointer
            temp                            = *identifyDataPtr++;               // Save Byte0, point to Byte1
            *firstBytePtr           = *identifyDataPtr;                         // Byte0 = Byte1
            *identifyDataPtr++      = temp;                                             // Byte1 = Byte0

        }

                #endif

        // Write to user
        *outputSize = buffer->getLength ( );
        memcpy(dataOut, bufferToCopy, buffer->getLength ( ));
        DEBUG_LOG("%s[%p]::%s cpy %p %p\n", getClassName(), this,  __FUNCTION__, (void*)*outputSize, (void*)buffer->getLength());
    }


ReleaseBufferPrepared:

    buffer->complete ( );


ReleaseBuffer:


    buffer->release ( );
    buffer = NULL;


ReleaseCommand:


    DeallocateCommand ( command );
    command = NULL;


ReleaseProvider:


    fProvider->release ( );


ErrorExit:


    fOutstandingCommands--;

    DEBUG_LOG("%s[%p]::%s result %p\n", getClassName(), this,  __FUNCTION__, (void*)status);
    return status;

}


#if 0
#pragma mark -
#pragma mark Protected Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ getTargetAndMethodForIndex -  Returns a pointer to the target of the
//									method call and the method vector itself
//									based on the provided index.	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOExternalMethod *
SATSMARTUserClient::getTargetAndMethodForIndex ( IOService ** target, UInt32 index )
{

    DEBUG_LOG("%s[%p]::%s index %d\n", getClassName(), this, __FUNCTION__, (int)index);
    if ( index >= kIOATASMARTMethodCount )
        return NULL;

    if ( isInactive ( ) )
        return NULL;

    *target = this;

    return &sMethods[index];

}

//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ GatedWaitForCommand -	Waits for signal to wake up. It must hold the
//							workloop lock in order to call commandSleep()
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::GatedWaitForCommand ( IOSATCommand * command )
{

    IOReturn status = kIOReturnSuccess;
    SATSMARTRefCon *        refCon;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    refCon = ( SATSMARTRefCon * ) command->refCon;

    // Check to make sure the command hasn't been completed and called the
    // callback handler before the stack unwinds and we get the workloop
    // lock. This usually won't happen since the callback runs on the
    // secondary interrupt context (involves going through the scheduler),
    // but it is still good to do this sanity check.
    while ( refCon->isDone != true )
    {

        fCommandGate->commandSleep ( &refCon->sleepOnIt, THREAD_UNINT );

    }

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ HandleTerminate -	Handles terminating our object and any resources it
//						allocated.									[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::HandleTerminate ( IOService * provider )
{

    IOReturn status = kIOReturnSuccess;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    while ( fOutstandingCommands != 0 )
    {
        IOSleep ( 10 );
    }

    // Check if we have our provider open.
    if ( provider->isOpen ( this ) )
    {
        // Yes we do, so close the connection
        DEBUG_LOG("%s[%p]::%s closing provider\n", getClassName(), this, __FUNCTION__);
        provider->close ( this, kIOATASMARTUserClientAccessMask );
    }

    // Decouple us from the IORegistry.
    detach ( provider );
    fProvider = NULL;

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ SendSMARTCommand - Sends a command to the hardware synchronously.
//						 This is a helper method for all our non-exclusive
//						 methods.									[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::SendSMARTCommand ( IOSATCommand * command )
{

    SATSMARTRefCon refCon;
    IOReturn status  = kIOReturnSuccess;

    bzero ( &refCon, sizeof ( refCon ) );

    refCon.isDone   = false;
    refCon.self             = this;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    command->setCallbackPtr ( &SATSMARTUserClient::sCommandCallback );
    command->refCon = ( void * ) &refCon;

    // Retain this object. It will be released by CommandCallback method.
    retain ( );

    status = fProvider->sendSMARTCommand ( command );
    if ( status == kIOReturnSuccess )
    {

        fCommandGate->runAction ( ( IOCommandGate::Action ) &SATSMARTUserClient::sWaitForCommand,
            ( void * ) command );

        switch ( command->getResult ( ) )
        {

        case kATANoErr:
            status = kIOReturnSuccess;
            break;

        case kATATimeoutErr:
            status = kIOReturnTimeout;
            break;

        default:
            status = kIOReturnIOError;
            break;

        }

    }

    else
    {

        // Error path, release this object since we called retain()
        release ( );

    }

    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, status);
    return status;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ CommandCallback -     This method is called by sCommandCallback as the
//							completion routine for all IOSATCommands.
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTUserClient::CommandCallback ( IOSATCommand * command )
{

    SATSMARTRefCon *                refCon  = NULL;

    refCon = ( SATSMARTRefCon * ) command->refCon;
    fCommandGate->commandWakeup ( &refCon->sleepOnIt, true );
    release ( );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ AllocateCommand -     This method allocates an IOSATCommand object.
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOSATCommand *
SATSMARTUserClient::AllocateCommand ( void )
{

    //IOSATCommand *	command = NULL;
    //IOATADevice *	device	= NULL;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    IOSATBusCommand64* cmd = IOSATBusCommand64::allocateCmd32();

    return (IOSATCommand*) cmd;

    //device = ( IOATADevice * ) ( fProvider->getProvider ( )->getProvider ( ) );
    //command = device->allocCommand ( );

    //return command;

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ DeallocateCommand -   This method deallocates an IOSATCommand object.
//																	[PROTECTED]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTUserClient::DeallocateCommand ( IOSATCommand * command )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);

    if ( command != NULL )
    {
        //IOATADevice *   device  = NULL;
        //device = ( IOATADevice * ) ( fProvider->getProvider ( )->getProvider ( ) );
        //device->freeCommand ( command );
        command->release();
    }

}


#if 0
#pragma mark -
#pragma mark Static Methods
#pragma mark -
#endif


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sWaitForCommand - Called by runAction and holds the workloop lock.
//																	[STATIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

IOReturn
SATSMARTUserClient::sWaitForCommand ( void *                    userClient,
                                      IOSATCommand *        command )
{

    return ( ( SATSMARTUserClient * ) userClient )->GatedWaitForCommand ( command );

}


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥ sCommandCallback -	Static completion routine. Calls CommandCallback.
//							It holds the workloop lock as well since it is on
//							the completion chain from the controller.
//																	[STATIC]
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void
SATSMARTUserClient::sCommandCallback ( IOSATCommand * command )
{

    SATSMARTRefCon *                refCon  = NULL;
    DEBUG_LOG("%s::%s command %p\n", getClassName(), __FUNCTION__, command);

    refCon = ( SATSMARTRefCon * ) command->refCon;
    if ( refCon == NULL )
    {
        ERROR_LOG ( "SATSMARTUserClient::sCommandCallback refCon == NULL.\n" );
        //IOPanic ( "SATSMARTUserClient::sCommandCallback refCon == NULL." );
        return;
    }

    refCon->isDone = true;
    refCon->self->CommandCallback ( command );

}
