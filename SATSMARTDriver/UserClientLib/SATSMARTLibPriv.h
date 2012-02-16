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


#ifndef __SAT_SMART_LIB_PRIV_H__
#define __SAT_SMART_LIB_PRIV_H__


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Structures
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

typedef struct ATASMARTReadLogStruct
{
	UInt8			numSectors;
	UInt8			logAddress;
	void *			buffer;
	UInt32			bufferSize;
} ATASMARTReadLogStruct;

typedef struct ATASMARTWriteLogStruct
{
	UInt8			numSectors;
	UInt8			logAddress;
	const void *	buffer;
	UInt32			bufferSize;
} ATASMARTWriteLogStruct;

typedef struct ATAGetIdentifyDataStruct
{
	const void *	buffer;
	UInt32			bufferSize;
} ATAGetIdentifyDataStruct;


//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	Constants
//ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

enum
{
	kIOATASMARTUserClientAccessBit		= 16,
	kIOATASMARTUserClientAccessMask		= (1 << kIOATASMARTUserClientAccessBit)
};

enum
{
	kIOATASMARTLibConnection = 12
};

enum
{
	kIOATASMARTEnableDisableOperations				= 0,	// kIOUCScalarIScalarO, 1, 0
	kIOATASMARTEnableDisableAutoSave				= 1,	// kIOUCScalarIScalarO, 1, 0
	kIOATASMARTReturnStatus							= 2,	// kIOUCScalarIScalarO, 0, 1
	kIOATASMARTExecuteOffLineImmediate				= 3,	// kIOUCScalarIScalarO, 1, 0
	kIOATASMARTReadData								= 4,	// kIOUCScalarIStructO, 1, 0
	kIOATASMARTReadDataThresholds					= 5,	// kIOUCScalarIStructO, 1, 0
	kIOATASMARTReadLogAtAddress						= 6,	// kIOUCScalarIStructI, 0, sizeof (ATASMARTReadLogStruct)
	kIOATASMARTWriteLogAtAddress					= 7,	// kIOUCScalarIStructI, 0, sizeof (ATASMARTWriteLogStruct)
	kIOATASMARTGetIdentifyData						= 8,	// kIOUCStructIStructO, sizeof (ATAGetIdentifyDataStruct), sizeof (UInt32)
	kIOATASMARTMethodCount
};


#define kATASMARTUserClientClassKey			"SATSMARTUserClient"
#define kATASMARTUserClientTypeIDKey		"24514B7A-2804-11D6-8A02-003065704866"
//#define kATASMARTUserClientLibLocationKey	"IOATAFamily.kext/Contents/PlugIns/IOATABlockStorage.kext/Contents/PlugIns/ATASMARTLib.plugin"
#define kSATSMARTUserClientLibLocationKey	"SATSMARTLib.plugin"
//#define kSATSMARTUserClientLibLocationKey	"SMARTLib.plugin"


#endif	/* __SAT_SMART_LIB_PRIV_H__ */