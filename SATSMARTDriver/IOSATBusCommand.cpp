/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

/*
 *
 *	IOSATBusCommand.cpp
 *
 */


#include <IOKit/IOTypes.h>
#ifndef APPLE_KEXT_DEPRECATED
#define APPLE_KEXT_DEPRECATED  __attribute__((deprecated))
#endif
#include "IOSyncer.h"
//#include "IOATATypes.h"
#include <IOKit/ata/IOATATypes.h>
#include "IOSATCommand.h"
#include "IOSATBusCommand.h"


#ifdef DLOG
#undef DLOG
#endif

#ifdef  DEBUG
#define DLOG(fmt, args ...)  IOLog(fmt, ## args)
#else
#define DLOG(fmt, args ...)
#endif


//---------------------------------------------------------------------------

#define super IOSATCommand

OSDefineMetaClassAndStructors( IOSATBusCommand, IOSATCommand )
OSMetaClassDefineReservedUnused(IOSATBusCommand, 0);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 1);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 2);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 3);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 4);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 5);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 6);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 7);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 8);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 9);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 10);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 11);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 12);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 13);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 14);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 15);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 16);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 17);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 18);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 19);
OSMetaClassDefineReservedUnused(IOSATBusCommand, 20);


/*-----------------------------------------------------------------------------
 *  Static allocator.
 *
 *-----------------------------------------------------------------------------*/
IOSATBusCommand*
IOSATBusCommand::allocateCmd(void)
{
    IOSATBusCommand* cmd = new IOSATBusCommand;
    
    if( cmd == NULL)
        return NULL;
    
    
    if( !cmd->init() )
    {
        cmd->free();
        return NULL;
    }
    
    return cmd;
    
}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSATBusCommand::init()
{
    if( !super::init() )
        return false;
    
    zeroCommand();
    
    return true;
    
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSATBusCommand::zeroCommand(void)
{
    queue_init( &queueChain );
    state = 0;
    syncer = NULL;
    
    super::zeroCommand();
    
}



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the command opcode
ataOpcode
IOSATBusCommand::getOpcode( void )
{
    
    return _opCode;
    
}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// get the command flags
ataFlags
IOSATBusCommand::getFlags ( void )
{
    
    return (ataFlags) _flags;
    
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
ataRegMask
IOSATBusCommand::getRegMask( void )
{
    return _regMask;
    
}



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the unit id (0 master, 1 slave)
ataUnitID
IOSATBusCommand::getUnit( void )
{
    
    return _unit;
    
}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the timeout value for this command
UInt32
IOSATBusCommand::getTimeoutMS (void )
{
    
    return _timeoutMS;
    
}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the callback pointer
IOSATCompletionFunction*
IOSATBusCommand::getCallbackPtr (void )
{
    
    return _callback;
    
}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// call the completion callback function
void
IOSATBusCommand::executeCallback(void)
{
    _inUse = false;
    
    if(_callback != NULL)
    {
        (*_callback)(this);
        
    } else if( syncer != NULL ) {
        
        syncer->signal();
        syncer = NULL;
        
    }
    
    
}

/*-----------------------------------------------------------------------------
 * get the number of bytes between intervening interrupts for this transfer.
 *
 *-----------------------------------------------------------------------------*/
IOByteCount
IOSATBusCommand::getTransferChunkSize(void)
{
    
    return _logicalChunkSize;
    
}

ataTaskFile*
IOSATBusCommand::getTaskFilePtr(void)
{
    return &(_taskFile.taskFile);
}

UInt16
IOSATBusCommand::getPacketSize(void)
{
    return _packet.atapiPacketSize;
}


UInt16*
IOSATBusCommand::getPacketData(void)
{
    return _packet.atapiCommandByte;
}

IOByteCount
IOSATBusCommand::getByteCount (void)
{
    
    return _byteCount;
}


IOByteCount
IOSATBusCommand::getPosition (void)
{
    
    return _position;
    
}



IOMemoryDescriptor*
IOSATBusCommand::getBuffer ( void)
{
    return _desc;
}


void
IOSATBusCommand::setActualTransfer ( IOByteCount bytesTransferred )
{
    _actualByteCount = bytesTransferred;
}

void
IOSATBusCommand::setResult( IOReturn inResult)
{
    
    _result = inResult;
}


void
IOSATBusCommand::setCommandInUse( bool inUse /* = true */)
{
    
    _inUse = inUse;
    
}

#pragma mark -- IOSATBusCommand64 --

#undef super

#define super IOSATBusCommand

OSDefineMetaClassAndStructors( IOSATBusCommand64, super )

/*-----------------------------------------------------------------------------
 *  Static allocator.
 *
 *-----------------------------------------------------------------------------*/
IOSATBusCommand64*
IOSATBusCommand64::allocateCmd32(void)
{
    IOSATBusCommand64* cmd = new IOSATBusCommand64;
    
    if( cmd == NULL)
        return NULL;
    
    
    if( !cmd->init() )
    {
        cmd->free();
        return NULL;
    }
    
    
    return cmd;
    
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSATBusCommand64::init()
{
    if( !super::init() )
        return false;
    
    zeroCommand();
    
    _dmaCmd = IODMACommand::withSpecification(IODMACommand::OutputHost32,
                                              32,
                                              0x10000,
                                              IODMACommand::kMapped,
                                              512 * 2048,
                                              2);
    
    
    if( !_dmaCmd )
    {
        return false;
    }
    
    
    return true;
    
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSATBusCommand64::zeroCommand(void)
{
    if(_dmaCmd != NULL)
    {
        if( _dmaCmd->getMemoryDescriptor() != NULL)
        {
            _dmaCmd->clearMemoryDescriptor();
        }
    }
    
    super::zeroCommand();
    
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATBusCommand64::free()
{
    
    if( _dmaCmd != NULL )
    {
        _dmaCmd->clearMemoryDescriptor();
        _dmaCmd->release();
        _dmaCmd = NULL;
    }
    
    super::free();
    
}

void
IOSATBusCommand64::setBuffer ( IOMemoryDescriptor* inDesc)
{
    
    super::setBuffer( inDesc );
    
}

void
IOSATBusCommand64::executeCallback(void)
{
    if( _dmaCmd != NULL
       && _desc != NULL
       && ( _flags & mATAFlagUseDMA ) )
    {
        _dmaCmd->complete();
    }
    
    _dmaCmd->clearMemoryDescriptor();
    
    super::executeCallback();
    
}


IODMACommand*
IOSATBusCommand64::GetDMACommand( void )
{
    
    return _dmaCmd;
    
}

void
IOSATBusCommand64::setCommandInUse( bool inUse /* = true */)
{
    
    if( inUse )
    {
        
        if( _dmaCmd != NULL
           && _desc != NULL
           && ( _flags & mATAFlagUseDMA ) )
        {
            _dmaCmd->setMemoryDescriptor( _desc, true );
            
            
        }
        
        
    }
    
    super::setCommandInUse( inUse);
    
}
