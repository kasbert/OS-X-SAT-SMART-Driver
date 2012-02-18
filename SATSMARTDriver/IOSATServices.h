/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * Copyright (c) 1998-2008 Apple Inc. All rights reserved.
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

#ifndef _IOKIT_IO_SAT_SERVICES_H_
#define _IOKIT_IO_SAT_SERVICES_H_

#if defined(KERNEL) && defined(__cplusplus)


//—————————————————————————————————————————————————————————————————————————————
//	Includes
//—————————————————————————————————————————————————————————————————————————————

// IOKit includes
#include <IOKit/IOTypes.h>

#include <IOKit/scsi/IOBlockStorageServices.h>

#include "IOSATDriver.h"

class IOSATCommand;

//—————————————————————————————————————————————————————————————————————————————
//	Class Declaration
//—————————————————————————————————————————————————————————————————————————————

class IOSATServices : public IOBlockStorageServices
{

OSDeclareDefaultStructors ( IOSATServices )

protected:

OSSet *                                                         fClients;

//virtual void	free ( void );

// Reserve space for future expansion.
struct IOSATServicesExpansionData { };
IOSATServicesExpansionData * fIOSATServicesReserved;

public:

virtual bool            attach ( IOService * provider );
virtual void            detach ( IOService * provider );
virtual IOReturn    newUserClient (
    task_t owningTask,
    void *                       securityID,
    UInt32 type,
    OSDictionary *       properties,
    IOUserClient **      handler );

virtual bool                handleOpen ( IOService * client, IOOptionBits options, void * access );
virtual void            handleClose ( IOService * client, IOOptionBits options );
virtual bool                handleIsOpen ( const IOService * client ) const;

public:

virtual IOReturn        sendSMARTCommand ( IOSATCommand * command );

// Binary Compatibility reserved method space
OSMetaClassDeclareReservedUnused ( IOSATServices, 0 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 1 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 2 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 3 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 4 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 5 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 6 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 7 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 8 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 9 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 10 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 11 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 12 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 13 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 14 );
OSMetaClassDeclareReservedUnused ( IOSATServices, 15 );


};

#endif  /* defined(KERNEL) && defined(__cplusplus) */

#endif /* _IOKIT_IO_SAT_SERVICES_H_ */