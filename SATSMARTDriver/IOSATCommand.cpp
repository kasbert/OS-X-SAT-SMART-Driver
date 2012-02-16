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
 *	IOSATCommand.cpp
 *
 */
 
 
#include<IOKit/IOTypes.h>
#include <IOKit/IOLib.h>

//#include"IOATATypes.h"
#include <IOKit/ata/IOATATypes.h>
#include"IOSATCommand.h"

#ifdef DLOG
#undef DLOG
#endif

#ifdef  ATA_DEBUG
#define DLOG(fmt, args...)  IOLog(fmt, ## args)
#else
#define DLOG(fmt, args...)
#endif

//---------------------------------------------------------------------------

#define super IOCommand

OSDefineMetaClass( IOSATCommand, IOCommand )
OSDefineAbstractStructors( IOSATCommand, IOCommand )

    OSMetaClassDefineReservedUsed(IOSATCommand, 0)  //setendResult()
    OSMetaClassDefineReservedUsed(IOSATCommand, 1) // getExtendedLBAPtr()
    OSMetaClassDefineReservedUnused(IOSATCommand, 2);
    OSMetaClassDefineReservedUnused(IOSATCommand, 3);
    OSMetaClassDefineReservedUnused(IOSATCommand, 4);
    OSMetaClassDefineReservedUnused(IOSATCommand, 5);
    OSMetaClassDefineReservedUnused(IOSATCommand, 6);
    OSMetaClassDefineReservedUnused(IOSATCommand, 7);
    OSMetaClassDefineReservedUnused(IOSATCommand, 8);
    OSMetaClassDefineReservedUnused(IOSATCommand, 9);
    OSMetaClassDefineReservedUnused(IOSATCommand, 10);
    OSMetaClassDefineReservedUnused(IOSATCommand, 11);
    OSMetaClassDefineReservedUnused(IOSATCommand, 12);
    OSMetaClassDefineReservedUnused(IOSATCommand, 13);
    OSMetaClassDefineReservedUnused(IOSATCommand, 14);
    OSMetaClassDefineReservedUnused(IOSATCommand, 15);
    OSMetaClassDefineReservedUnused(IOSATCommand, 16);
    OSMetaClassDefineReservedUnused(IOSATCommand, 17);
    OSMetaClassDefineReservedUnused(IOSATCommand, 18);
    OSMetaClassDefineReservedUnused(IOSATCommand, 19);
    OSMetaClassDefineReservedUnused(IOSATCommand, 20);



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSATCommand::init()
{
	fExpansionData = (ExpansionData*) IOMalloc( sizeof( ExpansionData) ); 
	fExpansionData->extLBA = IOSATExtendedLBA::createIOSATExtendedLBA( this );
	
	if( ! super::init() || fExpansionData == NULL || fExpansionData->extLBA == NULL )
		return false;

	zeroCommand();
	
	return true;

}

/*---------------------------------------------------------------------------
 *	free() - the pseudo destructor. Let go of what we don't need anymore.
 *
 *
 ---------------------------------------------------------------------------*/
void
IOSATCommand::free()
{

	getExtendedLBA()->release();
	IOFree( fExpansionData, sizeof( ExpansionData) );
	super::free();

}
/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void 
IOSATCommand::zeroCommand(void)
{

	_opCode	= kATANoOp;			
	_flags = 0;		 		
	_unit = kATAInvalidDeviceID;
	_timeoutMS = 0;			
	_desc = 0L;        		
	_position = 0;			
	_byteCount = 0;			
	_regMask = (ataRegMask) 0;		
	_callback = 0L ;  
 	_result = 0;			
	_actualByteCount = 0;  	
 	_status = 0;			
 	_errReg = 0;
	_logicalChunkSize = kATADefaultSectorSize;
 	_inUse = false;			
	
	_taskFile.ataDataRegister = 0x0000;
	_taskFile.ataAltSDevCReg = 0x00;
	_taskFile.taskFile.ataTFFeatures = 0;   		
	_taskFile.taskFile.ataTFCount  = 0;  		
	_taskFile.taskFile.ataTFSector  = 0; 		
	_taskFile.taskFile.ataTFCylLo  = 0;  		
	_taskFile.taskFile.ataTFCylHigh  = 0;  		
	_taskFile.taskFile.ataTFSDH  = 0;  		
	_taskFile.taskFile.ataTFCommand  = 0;  		
	
	
	for( int i = 0; i < 16 ; i += 2 )
	{
		_packet.atapiCommandByte[ i ] = 0x000;
	}

	_packet.atapiPacketSize = 0;
	
	getExtendedLBA()->zeroData();

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void 
IOSATCommand::setOpcode( ataOpcode inCode)
{

	_opCode = inCode;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void 
IOSATCommand::setFlags( UInt32 inFlags)
{

	_flags = inFlags;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void 
IOSATCommand::setUnit( ataUnitID inUnit)
{

	_unit = inUnit;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void 
IOSATCommand::setTimeoutMS( UInt32 inMS)
{

	_timeoutMS = inMS;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void 
IOSATCommand::setCallbackPtr (IOSATCompletionFunction* inCompletion)
{

	_callback = inCompletion;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void 
IOSATCommand::setRegMask( ataRegMask mask)
{

	_regMask = mask;

}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setBuffer ( IOMemoryDescriptor* inDesc)
{

	_desc = inDesc;

}



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setPosition (IOByteCount fromPosition)
{

	_position = fromPosition;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setByteCount (IOByteCount numBytes)
{

	_byteCount = numBytes;

}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setTransferChunkSize (IOByteCount numBytes)
{

	_logicalChunkSize = numBytes;

}



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setFeatures( UInt8 in)
{

	_taskFile.taskFile.ataTFFeatures = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getErrorReg (void )
{

	return _taskFile.taskFile.ataTFFeatures;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
	
void
IOSATCommand::setSectorCount( UInt8 in)
{

	_taskFile.taskFile.ataTFCount = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getSectorCount (void )
{

	return  _taskFile.taskFile.ataTFCount;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setSectorNumber( UInt8 in)
{

	_taskFile.taskFile.ataTFSector = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getSectorNumber (void )
{

	return _taskFile.taskFile.ataTFSector;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
	
void
IOSATCommand::setCylLo ( UInt8 in)
{

	_taskFile.taskFile.ataTFCylLo = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/


UInt8
IOSATCommand::getCylLo (void )
{

	return _taskFile.taskFile.ataTFCylLo;

}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSATCommand::setCylHi( UInt8 in)
{

	_taskFile.taskFile.ataTFCylHigh = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getCylHi (void )
{

	return _taskFile.taskFile.ataTFCylHigh;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
	
void
IOSATCommand::setDevice_Head( UInt8 in)
{

	_taskFile.taskFile.ataTFSDH = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getDevice_Head (void )
{

	return _taskFile.taskFile.ataTFSDH;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
	
void
IOSATCommand::setCommand ( UInt8 in)
{

	_taskFile.taskFile.ataTFCommand = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSATCommand::getStatus (void )
{

	return _taskFile.taskFile.ataTFCommand;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/



IOReturn 
IOSATCommand::setPacketCommand( UInt16 packetSizeBytes, UInt8* packetBytes)
{
//	IOLog("ATACommand::setPacket size %d  bytePtr = %lx\n", packetSizeBytes, packetBytes);
	
	if( ( packetSizeBytes > 16 ) || (packetBytes == 0L))
		return -1;  

	UInt8* cmdBytes = (UInt8*) _packet.atapiCommandByte;

	for( int i = 0; i < packetSizeBytes; i++ )
	{
		cmdBytes[ i ] = packetBytes[ i ];
	}

	_packet.atapiPacketSize = packetSizeBytes;
	
	return kATANoErr;

}



/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSATCommand::setDataReg ( UInt16 in)
{

	_taskFile.ataDataRegister = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/


UInt16 
IOSATCommand::getDataReg (void )
{

	return _taskFile.ataDataRegister;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/



void
IOSATCommand::setControl ( UInt8 in)
{

	_taskFile.ataAltSDevCReg = in;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/


UInt8
IOSATCommand::getAltStatus (void )
{

	return _taskFile.ataAltSDevCReg;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/




IOReturn 
IOSATCommand::getResult (void)
{

	return _result;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/


IOMemoryDescriptor* 
IOSATCommand::getBuffer ( void )
{

	return _desc;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

 
IOByteCount 
IOSATCommand::getActualTransfer ( void )
{

	return _actualByteCount;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/


UInt8	
IOSATCommand::getEndStatusReg (void)
{

	return _status;

}


/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/



UInt8
IOSATCommand::getEndErrorReg( void )
{

	return _errReg;

}


/*-----------------------------------------------------------------------------
 * returns true if IOATAController is using the command.
 *
 *-----------------------------------------------------------------------------*/

bool	
IOSATCommand::getCommandInUse( void )
{


	return _inUse;


}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOReturn 
IOSATCommand::setLBA28( UInt32 lba, ataUnitID inUnit)
{
	// param check the inputs
	
	if( (lba & 0xF0000000) != 0x00000000 
		|| !(inUnit == kATADevice0DeviceID || inUnit == kATADevice1DeviceID) )
	{
		//param out of range
		return -1;		
	}
	
	
	setSectorNumber( (lba &      0xFF) );  //LBA 7:0
	setCylLo( ((lba &          0xFF00) >> 8) );	// LBA 15:8
	setCylHi( ((lba &      0x00FF0000) >> 16) );  // LBA 23:16
	setDevice_Head(((lba & 0x0F000000) >> 24 ) |  mATALBASelect |( ((UInt8) inUnit) << 4));  //LBA 27:24

	return kATANoErr;

}

void 
IOSATCommand::setEndResult(UInt8 inStatus, UInt8 endError  )
{
	_status = inStatus;
	_errReg = endError;
}



IOSATExtendedLBA* 
IOSATCommand::getExtendedLBA(void)
{

	return fExpansionData->extLBA;

}



////////////////////////////////////////////////////////////////////////
#pragma mark IOSATExtendedLBA
#undef super

#define super OSObject
OSDefineMetaClassAndStructors( IOSATExtendedLBA, OSObject )

OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 0)
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 1)
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 2);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 3);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 4);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 5);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 6);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 7);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 8);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 9);
OSMetaClassDefineReservedUnused(IOSATExtendedLBA, 10);





IOSATExtendedLBA* 
IOSATExtendedLBA::createIOSATExtendedLBA(IOSATCommand* inOwner)
{

	IOSATExtendedLBA* me = new IOSATExtendedLBA;
	if( me == NULL)
	{	
		return NULL;
	}
	
	me->owner = inOwner;
	me->zeroData();
	
	return me;

}


	
void 
IOSATExtendedLBA::setLBALow16( UInt16 inLBALow)
{
	lbaLow =  inLBALow ;

}


UInt16 
IOSATExtendedLBA::getLBALow16 (void)
{

	return lbaLow ;

}
	
void 
IOSATExtendedLBA::setLBAMid16 (UInt16 inLBAMid)
{

	lbaMid =  inLBAMid ;
}


UInt16 
IOSATExtendedLBA::getLBAMid16( void )
{

	return lbaMid;


}
	
void 
IOSATExtendedLBA::setLBAHigh16( UInt16 inLBAHigh )
{
	lbaHigh =  inLBAHigh ;
}


UInt16 
IOSATExtendedLBA::getLBAHigh16( void )
{

	return lbaHigh;

}
	
void 
IOSATExtendedLBA::setSectorCount16( UInt16 inSectorCount )
{

	sectorCount =  inSectorCount ;

}

UInt16 
IOSATExtendedLBA::getSectorCount16( void )
{
	return sectorCount;
}
	
void 
IOSATExtendedLBA::setFeatures16( UInt16 inFeatures )
{
	features =  inFeatures;
}

UInt16 
IOSATExtendedLBA::getFeatures16( void )
{

	return features;

}

void 
IOSATExtendedLBA::setDevice( UInt8 inDevice )
{

	device = inDevice;

}


UInt8 
IOSATExtendedLBA::getDevice( void )
{


	return device;

}

void 
IOSATExtendedLBA::setCommand( UInt8 inCommand )
{


	command = inCommand;

}


UInt8 
IOSATExtendedLBA::getCommand( void )
{

	return command;
}


	
void 
IOSATExtendedLBA::setExtendedLBA( UInt32 inLBAHi, UInt32 inLBALo, ataUnitID inUnit, UInt16 extendedCount, UInt8 extendedCommand )
{

	UInt8 lba7, lba15, lba23, lba31, lba39, lba47;
	lba7 = (inLBALo & 0xff);
	lba15 = (inLBALo & 0xff00) >> 8;
	lba23 = (inLBALo & 0xff0000) >> 16;
	lba31 = (inLBALo & 0xff000000) >> 24;
	lba39 = (inLBAHi & 0xff);
	lba47 = (inLBAHi & 0xff00) >> 8;

	setLBALow16(  lba7 | (lba31 << 8) );
	setLBAMid16(  lba15 | (lba39 << 8));
	setLBAHigh16(  lba23 | (lba47 << 8));
	
	setSectorCount16( extendedCount );
	setCommand( extendedCommand );
	setDevice(   mATALBASelect |( ((UInt8) inUnit) << 4)); // set the LBA bit and device select bits. The rest are reserved in extended addressing.

}

void 
IOSATExtendedLBA::getExtendedLBA( UInt32* outLBAHi, UInt32* outLBALo )
{



	*outLBALo = (getLBALow16() & 0xFF) | ( (getLBAMid16() & 0xff) << 8) | ((getLBAHigh16() & 0xff) << 16) | ((getLBALow16() & 0xff00) << 16) ; 
	
	*outLBAHi = getLBAHigh16() & 0xff00 | ((getLBAMid16() & 0xff00) >> 8) ; 

}

void 
IOSATExtendedLBA::zeroData(void)
{
	lbaLow = lbaMid = lbaHigh = sectorCount = features = device = command = 0;
}
	

