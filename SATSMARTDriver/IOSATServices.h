/*
 *  IOSATServices.h
 *  IOSATDriver
 *
 *  Created by Vieras on 3.2.2012.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
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
	
	OSSet *								fClients;
	
	//virtual void	free ( void );			
	
    // Reserve space for future expansion.
    struct IOSATServicesExpansionData { };
    IOSATServicesExpansionData * fIOSATServicesReserved;
	
public:
	
	virtual bool		attach ( IOService * provider );
	virtual void		detach ( IOService * provider );
    virtual IOReturn	newUserClient (
									   task_t			owningTask,
									   void *			securityID,
									   UInt32			type,
									   OSDictionary * 	properties,
									   IOUserClient **	handler );
	
    virtual bool		handleOpen ( IOService * client, IOOptionBits options, void * access );
	virtual void		handleClose ( IOService * client, IOOptionBits options );
    virtual bool		handleIsOpen ( const IOService * client ) const;
	
public:
	
	virtual IOReturn	sendSMARTCommand ( IOSATCommand * command );
	
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

#endif	/* defined(KERNEL) && defined(__cplusplus) */

#endif /* _IOKIT_IO_SAT_SERVICES_H_ */