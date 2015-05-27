/*
 * Modified by Jarkko Sonninen 2012
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

#include <AssertMacros.h>

#include "IOSATServices.h"
#include "UserClientLib/SATSMARTLibPriv.h"
#include "UserClient/SATSMARTUserClient.h"

#define kIOPropertySMARTCapableKey              "SMART Capable"

#define super IOBlockStorageServices
OSDefineMetaClassAndStructors ( IOSATServices, IOBlockStorageServices );


#ifdef DEBUG
#define DEBUG_LOG IOLog
#else
#define DEBUG_LOG(...)
#endif

#define ERROR_LOG IOLog

#define getClassName() "IOSATServices"


//---------------------------------------------------------------------------
// attach to provider.

bool
IOSATServices::attach ( IOService * provider )
{
    DEBUG_LOG("%s[%p]::%s %p\n", getClassName(), this, __FUNCTION__, provider);
    
    bool result = false;
    //OSDictionary *      dictionary = NULL;
    
    __Require_String ( super::attach ( provider ), ErrorExit,
                    "Superclass didn't attach" );
    
    fi_dungeon_driver_IOSATDriver *            fProvider;
    fProvider = OSDynamicCast ( fi_dungeon_driver_IOSATDriver, provider );
    __Require_String ( fProvider, ErrorExit, "Incorrect provider type\n" );
    
    OSNumber *  value;
    UInt32 features;
    
    value = OSDynamicCast ( OSNumber, fProvider->getProperty ( kIOATASupportedFeaturesKey ) );
    if ( value != NULL )
    {
        
        features = value->unsigned32BitValue ( );
        if ( features & kIOATAFeatureSMART )
        {
            
            setProperty(kIOPropertySMARTCapableKey, true);
            
            OSDictionary *      userClientDict = OSDictionary::withCapacity ( 1 );
            OSString *          string2 = NULL;
            
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
        
    }
    
    result = true;
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}


//---------------------------------------------------------------------------
// detach from provider.

void
IOSATServices::detach ( IOService * provider )
{
    DEBUG_LOG("%s[%p]::%s %p\n", getClassName(), this, __FUNCTION__, provider);
    
    if ( fClients != NULL )
    {
        if ( fClients->getCount ( ) != 0 )
        {
            ERROR_LOG ("Detaching and still have client count = %d\n", fClients->getCount ( ) );
            return;
        }
        fClients->release ( );
        fClients = NULL;
    }
    
    super::detach ( provider );
}

IOReturn IOSATServices::newUserClient (
                                       task_t owningTask,
                                       void *                    securityID,
                                       UInt32 type,
                                       OSDictionary *    properties,
                                       IOUserClient **   handler ) {
    DEBUG_LOG("%s[%p]::%s type %d\n", getClassName(), this, __FUNCTION__, (int)type);
    IOReturn err;
    
    const OSSymbol *userClientClass = NULL;
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
    if (!userClientClass) {
        err = kIOReturnUnsupported;
	goto ErrorExit;
    }
    DEBUG_LOG("%s[%p]::%s client class %p\n", getClassName(), this, __FUNCTION__, userClientClass);
    
    // This reference is consumed by the IOServiceOpen call
    temp = OSMetaClass::allocClassWithName(userClientClass);
    if (!temp) {
        err = kIOReturnNoMemory;
	goto ErrorExit;
    }
    
    client = OSDynamicCast(IOUserClient, temp);
    if (!client) {
        temp->release();
	ERROR_LOG("Client class is not a IOUserClient\n");
        err =  kIOReturnUnsupported;
	goto ReleaseClient;
    }
    DEBUG_LOG("%s[%p]::%s client %p\n", getClassName(), this, __FUNCTION__, client);
    
    if ( !client->initWithTask(owningTask, securityID, type, properties) ) {
        client->release();
        err = kIOReturnBadArgument;
	goto ReleaseClient;
    }
    
    if ( !client->attach(this) ) {
        err = kIOReturnUnsupported;
	goto ReleaseClient;
    }
    
    if ( !client->start(this) ) {
        err = kIOReturnUnsupported;
	goto DetachClient;
    }
    
    *handler = client;
    
    err = kIOReturnSuccess;
    goto Exit;
    
DetachClient:
    client->detach(this);
ReleaseClient:
    client->release();
ErrorExit:
Exit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, err);
    return err;
}



//---------------------------------------------------------------------------
// handleOpen from client.

bool
IOSATServices::handleOpen ( IOService * client, IOOptionBits options, void * access )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    bool result = true;
    // If this isn't a user client, pass through to superclass.
    if ( ( options & kIOATASMARTUserClientAccessMask ) == 0 )
    {
        result = super::handleOpen ( client, options, access );
    } else {
        
        // It's the user client, so add it to the set
        
        if ( fClients == NULL )
            fClients = OSSet::withCapacity ( 1 );
        
        if ( fClients == NULL )
            return false;
        
        // Check if we already have too many user clients open
        if ( fClients->getCount ( ) > 100 )
        {
            ERROR_LOG ( "User client already open\n" );
            return false;
        }
        
        fClients->setObject ( client );
    }
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}


//---------------------------------------------------------------------------
// handleClose from client.

void
IOSATServices::handleClose ( IOService * client, IOOptionBits options )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    // If this isn't a user client, pass through to superclass.
    if ( ( options & kIOATASMARTUserClientAccessMask ) == 0 ) {
        super::handleClose ( client, options );
        
    } else {
        fClients->removeObject ( client );
    }
}


//---------------------------------------------------------------------------
// handleIsOpen from client.

bool
IOSATServices::handleIsOpen ( const IOService * client ) const
{
    DEBUG_LOG("%s[%p]::%s client %p\n", getClassName(), this, __FUNCTION__, client );
    // General case (is anybody open)
    if ( client == NULL ) {
        if ( ( fClients != NULL ) && ( fClients->getCount ( ) > 0 ) )
            return true;
    } else {
        // specific case (is this client open)
        if ( ( fClients != NULL ) && ( fClients->containsObject ( client ) ) )
            return true;
    }
    return super::handleIsOpen ( client );
}


//---------------------------------------------------------------------------
//
IOReturn
IOSATServices::sendSMARTCommand ( IOSATCommand * command )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    IOReturn result;
    
    // Block incoming I/O if we have been terminated
    if ( isInactive ( ))
    {
        return kIOReturnNotAttached;
    }
    
    if ( fProvider == NULL )
    {
        return kIOReturnInvalid;
    }
    
    fi_dungeon_driver_IOSATDriver *            fSATProvider;
    fSATProvider = OSDynamicCast ( fi_dungeon_driver_IOSATDriver, fProvider );
    if ( fSATProvider == NULL )
    {
        ERROR_LOG ( "%s::%s: attach; wrong provider type!\n", getClassName(), __FUNCTION__  );
        return kIOReturnInvalid;
    }
    
    result = fSATProvider->sendSMARTCommand ( command );
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
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

