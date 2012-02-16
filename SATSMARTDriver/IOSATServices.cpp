/*
 *  IOSATServices.cpp
 *  IOSATDriver
 *
 *  Created by Vieras on 3.2.2012.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

/*
 * Copyright (c) 1998-2003 Apple Computer, Inc. All rights reserved.
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

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>
#include <IOKit/IOPlatformExpert.h>

#include <IOKit/storage/ata/IOATAStorageDefines.h>

#include </usr/include/AssertMacros.h>

#include "IOSATServices.h"
#include "UserClientLib/SATSMARTLibPriv.h"
#include "UserClient/SATSMARTUserClient.h"
#include </Developer/SDKs/MacOSX10.6.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/storage/ata/ATASMARTLib.h>


#define super IOBlockStorageServices
OSDefineMetaClassAndStructors ( IOSATServices, IOBlockStorageServices );


#if ( ATA_BLOCK_STORAGE_DEVICE_DEBUGGING_LEVEL >= 1 )
#define PANIC_NOW(x)			IOPanic x
#else
#define PANIC_NOW(x)
#endif

#if ( ATA_BLOCK_STORAGE_DEVICE_DEBUGGING_LEVEL >= 2 )
#define ERROR_LOG(x)			IOLog x
#else
#define ERROR_LOG(x)
#endif

#if ( ATA_BLOCK_STORAGE_DEVICE_DEBUGGING_LEVEL >= 3 )
#define STATUS_LOG(x)			IOLog x
#else
#define STATUS_LOG(x)
#endif


//---------------------------------------------------------------------------
// attach to provider.

bool
IOSATServices::attach ( IOService * provider )
{
	IOLog("IOSATServices::attach %p %p\n", this, provider);

	bool	result = false;
	OSDictionary *	dictionary = NULL;
	
	require_string ( super::attach ( provider ), ErrorExit,
					"Superclass didn't attach" );
	
	org_dungeon_driver_IOSATDriver *		fProvider;
	fProvider = OSDynamicCast ( org_dungeon_driver_IOSATDriver, provider );
	require_string ( fProvider, ErrorExit, "Incorrect provider type\n" );
	
	//setProperty ( kIOPropertyProtocolCharacteristicsKey,
	//			 fProvider->GetProtocolCharacteristicsDictionary ( ) );
	//setProperty ( kIOPropertyDeviceCharacteristicsKey,
	//			 fProvider->GetDeviceCharacteristicsDictionary ( ) );
	
	setProperty ( "Humppa", "hei!" );

	dictionary = OSDictionary::withDictionary(fProvider->GetDeviceCharacteristicsDictionary ( ) ,0);
	require_string ( dictionary, ErrorExit, "No device characteristics\n" );

	OSNumber *	value;
	UInt32		features;
		
	dictionary->setObject ( kIOATASupportedFeaturesKey, fProvider->getProperty ( kIOATASupportedFeaturesKey ) );
	value = OSDynamicCast ( OSNumber, fProvider->getProperty ( kIOATASupportedFeaturesKey ) );
	if ( value != NULL )
	{
		
		features = value->unsigned32BitValue ( );
		if ( features & kIOATAFeatureSMART )
		{
			
			setProperty(kIOPropertySMARTCapableKey, true);
			
			OSDictionary *	userClientDict = OSDictionary::withCapacity ( 1 );
			OSString *		string2 = 0;
			
			string2 = OSString::withCString ( kSATSMARTUserClientLibLocationKey );
			userClientDict->setObject ( kATASMARTUserClientTypeIDKey, string2 );
			
			setProperty ( kIOUserClientClassKey, kATASMARTUserClientClassKey );
			setProperty ( kIOCFPlugInTypesKey, userClientDict );
			
			if ( userClientDict != NULL )
			{
				userClientDict->release ( );
				userClientDict = NULL;
			}
			
			if ( string2 != NULL )
			{
				string2->release ( );
				string2 = NULL;
			}
		}
		
		setProperty ( "kIOPropertyDeviceCharacteristicsKey", dictionary );
		
		dictionary->release ( );
	}
	
	result = true;

ErrorExit:
	return result;
}


//---------------------------------------------------------------------------
// detach from provider.

void
IOSATServices::detach ( IOService * provider )
{
	IOLog("IOSATServices::detach %p %p\n", this, provider);

	if ( fClients != NULL )
	{
		fClients->release ( );
		fClients = NULL;
	}
	
	super::detach ( provider );
}

IOReturn	IOSATServices::newUserClient (
								   task_t			owningTask,
								   void *			securityID,
								   UInt32			type,
								   OSDictionary * 	properties,
								   IOUserClient **	handler ) {
	IOLog("newUserClient %d %p\n",(int)type,properties);
	
	//IOReturn err = super::newUserClient(owningTask, securityID, type, properties, handler);
	//IOLog("newUserClient super %x %s\n", err, stringFromReturn(err));
	
	
	const OSSymbol *userClientClass = 0;
    IOUserClient *client;
    OSObject *temp;
	
    // First try my own properties for a user client class name
    temp = getProperty(gIOUserClientClassKey);
    if (temp) {
        if (OSDynamicCast(OSSymbol, temp))
            userClientClass = (const OSSymbol *) temp;
        else if (OSDynamicCast(OSString, temp)) {
            userClientClass = OSSymbol::withString((OSString *) temp);
        }
    }
	
    // Didn't find one so lets just bomb out now without further ado.
    IOLog ("userClientClass %p %p\n", temp, userClientClass);
    // Didn't find one so lets just bomb out now without further ado.
    if (!userClientClass)
        return kIOReturnUnsupported;
	
    // This reference is consumed by the IOServiceOpen call
    temp = OSMetaClass::allocClassWithName(userClientClass);
    IOLog ("alloc %p\n", temp);
    if (!temp)
        return kIOReturnNoMemory;
	
    if (OSDynamicCast(IOUserClient, temp))
        client = (IOUserClient *) temp;
    else {
        temp->release();
		IOLog ("client not\n");
        return kIOReturnUnsupported;
    }
    IOLog ("client %p\n", client);
	
    if ( !client->initWithTask(owningTask, securityID, type, properties) ) {
        client->release();
        return kIOReturnBadArgument;
    }
	
    if ( !client->attach(this) ) {
        client->release();
        return kIOReturnUnsupported;
    }
	
    if ( !client->start(this) ) {
        client->detach(this);
        client->release();
        return kIOReturnUnsupported;
    }
	
    *handler = client;
    return kIOReturnSuccess;

	IOReturn		err = kIOReturnSuccess;

	
ErrorExit:
	IOLog("newUserClient %x\n", err);
	return err;

}



//---------------------------------------------------------------------------
// handleOpen from client.

bool
IOSATServices::handleOpen ( IOService * client, IOOptionBits options, void * access )
{
	IOLog("IOSATServices::handleOpen %p\n", this);

	// If this isn't a user client, pass through to superclass.
	if ( ( options & kIOATASMARTUserClientAccessMask ) == 0 )
	{
		return super::handleOpen ( client, options, access );
	}
	
	// It's the user client, so add it to the set
	
	if ( fClients == NULL )
		fClients = OSSet::withCapacity ( 1 );
	
	if ( fClients == NULL )
		return false;
	
	// Check if we already have a user client open
	if ( fClients->getCount ( ) != 0 )
	{
		ERROR_LOG ( ( "User client already open\n" ) );
		return false;
	}
	
	fClients->setObject ( client );
	
	return true;
	
}


//---------------------------------------------------------------------------
// handleClose from client.

void
IOSATServices::handleClose ( IOService * client, IOOptionBits options )
{
	IOLog("IOSATServices::handleClose\n");

	// If this isn't a user client, pass through to superclass.
	if ( ( options & kIOATASMARTUserClientAccessMask ) == 0 )
		super::handleClose ( client, options );
	
	else
	{
		STATUS_LOG ( ( "Removing user client\n" ) );
		fClients->removeObject ( client );
		if ( fClients->getCount ( ) != 0 )
		{
			ERROR_LOG ( ( "Removed client and still have count = %d\n", fClients->getCount ( ) ) );
			return;
		}
	}
	
}


//---------------------------------------------------------------------------
// handleIsOpen from client.

bool
IOSATServices::handleIsOpen ( const IOService * client ) const
{
	IOLog("IOSATServices::handleIsOpen\n");

	// General case (is anybody open)
	if ( client == NULL )
	{
		
		STATUS_LOG ( ( "IOSATServices::handleIsOpen, client is NULL\n" ) );
		
		if ( ( fClients != NULL ) && ( fClients->getCount ( ) > 0 ) )
			return true;
		
		STATUS_LOG ( ( "calling super\n" ) );
		
		return super::handleIsOpen ( client );
		
	}
	
	STATUS_LOG ( ( "IOSATServices::handleIsOpen, client = %p\n", client ) );
	
	// specific case (is this client open)
	if ( ( fClients != NULL ) && ( fClients->containsObject ( client ) ) )
		return true;
	
	STATUS_LOG ( ( "calling super\n" ) );
	
	return super::handleIsOpen ( client );
	
}


//---------------------------------------------------------------------------
// 
IOReturn
IOSATServices::sendSMARTCommand ( IOSATCommand * command )
{
	IOLog("IOSATServices::sendSMARTCommand\n");

	// Block incoming I/O if we have been terminated
	if ( isInactive ( ))
	{
		return kIOReturnNotAttached;
	}
	
	if ( fProvider == NULL )
	{
		return kIOReturnInvalid;
	}
	
	org_dungeon_driver_IOSATDriver *		fSATProvider;
	fSATProvider = OSDynamicCast ( org_dungeon_driver_IOSATDriver, fProvider );
	if ( fSATProvider == NULL )
	{
		ERROR_LOG ( ( "IOSATServices: attach; wrong provider type!\n" ) );
		return false;
	}
	
	return fSATProvider->sendSMARTCommand ( command );
	
}


// binary compatibility reserved method space
OSMetaClassDefineReservedUnused ( IOSATServices, 0 );
OSMetaClassDefineReservedUnused ( IOSATServices, 1 );
OSMetaClassDefineReservedUnused ( IOSATServices, 2 );
OSMetaClassDefineReservedUnused ( IOSATServices, 3 );
OSMetaClassDefineReservedUnused ( IOSATServices, 4 );
OSMetaClassDefineReservedUnused ( IOSATServices, 5 );
OSMetaClassDefineReservedUnused ( IOSATServices, 6 );
OSMetaClassDefineReservedUnused ( IOSATServices, 7 );
OSMetaClassDefineReservedUnused ( IOSATServices, 8 );
OSMetaClassDefineReservedUnused ( IOSATServices, 9 );
OSMetaClassDefineReservedUnused ( IOSATServices, 10 );
OSMetaClassDefineReservedUnused ( IOSATServices, 11 );
OSMetaClassDefineReservedUnused ( IOSATServices, 12 );
OSMetaClassDefineReservedUnused ( IOSATServices, 13 );
OSMetaClassDefineReservedUnused ( IOSATServices, 14 );
OSMetaClassDefineReservedUnused ( IOSATServices, 15 );

//--------------------------------------------------------------------------------------
//							End				Of				File
//--------------------------------------------------------------------------------------

