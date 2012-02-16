
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/scsi/SCSICmds_INQUIRY_Definitions.h>
#include <IOKit/scsi/SCSICommandOperationCodes.h>
#include <IOKit/scsi/SCSITask.h>
#include <IOKit/IOKitKeys.h>
#include </usr/include/AssertMacros.h>

#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/scsi/IOSCSIProtocolServices.h>
#include <IOKit/scsi/IOSCSIPeripheralDeviceNub.h>

#include <IOKit/storage/ata/IOATAStorageDefines.h>

#include "IOSATBusCommand.h"
#include "IOSATDriver.h"
#include "IOSATServices.h"

#define DEBUG 1

#ifdef DEBUG
#define DEBUG_LOG IOLog
#else
#define DEBUG_LOG(...)
#endif

#define ERROR_LOG IOLog



#define Endian16_Swap(value) \
((((UInt16)((value) & 0x00FF)) << 8) | \
(((UInt16)((value) & 0xFF00)) >> 8))

#define kSCSICmd_PASS_THROUGH_16 0x85
#define kSCSICmd_PASS_THROUGH_12 0xA1


#define super IOSCSIPeripheralDeviceType00
OSDefineMetaClassAndStructors(org_dungeon_driver_IOSATDriver, IOSCSIPeripheralDeviceType00)


/////////////
static void hexdump16(UInt16 *data, int start, int len) {
	int i, j;
	for (i = start; i < start + len; i+=8) {
		IOLog ( "%d:", i);
		for (j = 0; j < 8 && i + j < start + len; j++) {
			IOLog ( " %04x", (int)data[i+j]);
		}
		IOLog ( "\n");
	}
}

static void trimcpy(char * target, const char *source, int maxlen) {
	const char *p;
	char *q;
	int i;
	p = source;
	q = target;
	for (i = 0; *p == ' ' && i < maxlen-1; i++, p++) ;
	for (; i < maxlen-1; i++, p++, q++) *q = *p;
	*q = 0;
	i--;q--;
	for(; *q == ' ' && i >= 0; i--, q--) *q = 0; 
}

void swapbytes(IOBufferMemoryDescriptor *        buffer) {
	int i;
	UInt16 *ptr = ( UInt16 * ) buffer->getBytesNoCopy ( );
	for (i = 0; i < buffer->getLength()/sizeof(UInt16); i++) {
		ptr[i]=Endian16_Swap(ptr[i]);
	}
}

UInt8 checksum(IOBufferMemoryDescriptor *        buffer) {
	int i;
	UInt8 sum = 0;
	UInt8 *ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
	for (i = 0; i < buffer->getLength(); i++) {
		sum += ptr[i];
	}
	return sum;
}

///////


bool org_dungeon_driver_IOSATDriver::init(OSDictionary *dict)
{
    bool result = super::init(dict);
    IOLog("Initializing %p\n", this);
	strncpy(serial, "vendor", sizeof(serial)-1);
	strncpy(model, "product", sizeof(model)-1);
	strncpy(revision, "revision",sizeof(revision)-1);
    return result;
}

void org_dungeon_driver_IOSATDriver::free(void)
{
    IOLog("Freeing %p\n", this);
    super::free();
}

IOService *org_dungeon_driver_IOSATDriver::probe(IOService *provider,
												 SInt32 *score)
{
    IOLog("Probing %p\n", this);
    IOService *result = super::probe(provider, score);
	if (result != NULL) {
		// the following doesn not work
		//SendCreatedPASS_THROUGH();
		//if (!smartOk) result = 0;
	}
    return result;
}

bool org_dungeon_driver_IOSATDriver::start(IOService *provider)
{
    bool result = super::start(provider);
    IOLog("Starting %p %d\n", this, (int)result);
    return result;
}

void org_dungeon_driver_IOSATDriver::stop(IOService *provider)
{
    IOLog("Stopping %p\n", this);
    super::stop(provider);
}


// This function will be called when the user process calls IORegistryEntrySetCFProperties on
// this driver. You can add your custom functionality to this function.
IOReturn org_dungeon_driver_IOSATDriver::setProperties(OSObject* properties)
{
    OSDictionary*   dict;
    OSNumber*       number;
	
    IOLog("setProperties %p *%s*\n", this, kMyPropertyKey);
	
    dict = OSDynamicCast(OSDictionary, properties);
    if (!dict) {
		IOLog("setProperties NULL\n");
        return kIOReturnBadArgument;
    }
    
    number = OSDynamicCast(OSNumber, dict->getObject(kMyPropertyKey));
    if (!number) {
        return super::setProperties(properties);
	}
	UInt32 value= number->unsigned32BitValue();
	
	DEBUG_LOG("%s[%p]::%s(%p) got value %lu\n", getName(), this, __FUNCTION__, properties, value);
	
	switch (value) {
		case 1:
			SendBuiltInINQUIRY ( );
			break;
		case 2:
			SendCreatedINQUIRY ( );
			break;
		case 3:
			SendBuiltInLOG_SENSE();
			break;
		case 4:
			Send_ATA_IDENTIFY();
			break;
		case 5:
			Send_ATA_DEVICE_CONFIGURATION_IDENTIFY();
			break;
		case 6:
			Send_ATA_SMART_READ_DATA();
			break;
		case 20:
			Send_ATA_SEND_SOFT_RESET();
			break;
	}		
	
	return kIOReturnSuccess;
}


bool
org_dungeon_driver_IOSATDriver::attach ( IOService * provider )
{
    IOLog("attach %p\n", this);
	bool	result = false;
	unsigned long features = kIOATAFeatureSMART;
	OSNumber *number;
	
	require_string ( super::attach ( provider ), ErrorExit,
					"Superclass didn't attach" );
	
	setProperty ( "Humppa", "paraati" );
	
	number = OSNumber::withNumber(features, 32);
	require_string(number, ErrorExit, "OSNumber::withNumber");
	setProperty ( kIOATASupportedFeaturesKey, number );
	number->release();
	
	result = true;
	
ErrorExit:
	return result;
}

void
org_dungeon_driver_IOSATDriver::detach ( IOService * provider )
{
    IOLog("detach %p\n",this);
	
	super::detach ( provider );
}


void
org_dungeon_driver_IOSATDriver::CreateStorageServiceNub ( void )
{
	IOLog("CreateStorageServiceNub %p\n", this);
	IOService * 	nub = NULL;
	
	//nub = OSTypeAlloc ( IOBlockStorageServices );
	nub = OSTypeAlloc ( IOSATServices );
	
	require_quiet ( nub, ErrorExit );
	
	nub->init ( );
	require ( nub->attach ( this ), ErrorExit );
	
	nub->registerService ( );
	nub->release ( );
	
	return;
	
ErrorExit:
	
	
	IOLog ( "org_dungeon_driver_IOSATDriver::CreateStorageServiceNub failed"  );
	super::CreateStorageServiceNub();
	return;
	
}

IOReturn  org_dungeon_driver_IOSATDriver::sendSMARTCommand ( IOSATCommand * command ) 
{
    IOLog("org_dungeon_driver_IOSATDriver::sendSMARTCommand %p\n", this);

	IOSATBusCommand* cmd = OSDynamicCast( IOSATBusCommand, command);
	
	if( !cmd ) {
		IOLog("Command is not a IOSATBusCommand");
		return kIOReturnBadArgument;
	}
	
	IOLog("buffer %p\n", cmd->getBuffer());
	IOLog("bytecount %d\n", (int)cmd->getByteCount());
	IOLog("features %x\n", cmd->getErrorReg());
	IOLog("opcode %x\n", cmd->getOpcode());
	IOLog("timeout %d\n", (int)cmd->getTimeoutMS());
	IOLog("sector count %x\n", cmd->getSectorCount());
	IOLog("sector num %x\n", cmd->getSectorNumber());
	IOLog("cyllo %x\n", cmd->getCylLo());
	IOLog("cylhi %x\n", cmd->getCylHi());
	IOLog("device %x\n", cmd->getDevice_Head());
	IOLog("command %x\n", cmd->getStatus());
	IOLog("flags %x\n", cmd->getFlags());

	/*
	command->setBuffer 			( buffer );
	command->setByteCount 		( sizeof ( ATASMARTData ) );
	command->setFeatures 		( kFeaturesRegisterReadData );
	command->setOpcode			( kATAFnExecIO );
	command->setTimeoutMS		( kATAThirtySecondTimeoutInMS );
	command->setCylLo			( kSMARTMagicCylinderLoValue );
	command->setCylHi			( kSMARTMagicCylinderHiValue );
	command->setCommand			( kATAcmdSMART );
	command->setFlags 			( mATAFlagIORead );
	*/
	
	SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier                request            = NULL;
	int ataResult = kATAErrUnknownType;
	IOReturn err = kIOReturnInvalid;
	int direction = (cmd->getFlags() & mATAFlagIORead) ? 1 : 0;
	int count = 0;
	if (cmd->getBuffer() && cmd->getByteCount() == 512) count = 1;
	int protocol = 3;
	if (count) {
		protocol = direction ? 4: 5;
	}
	IOLog("%d %d %d\n", direction, count, protocol);
	
    IOLog("Send_ATA_SMART\n");
	ataResult = kATANoErr;
	err=kIOReturnSuccess;
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12 ( request,
						  cmd->getBuffer(),
						  0,//     MULTIPLE_COUNT,
						  protocol,//     PROTOCOL,
						  0,//     EXTEND,
						  0,//     OFF_LINE,
						  0,//     CK_COND,
						  direction,//     T_DIR,
						  1,//     BYT_BLOK,
						  count ? 0x02 : 0,//     T_LENGTH,
						  cmd->getErrorReg(), //	FEATURES
						  count,//	SECTOR_COUNT,
						  cmd->getSectorNumber(), //	LBA_LOW,
						  cmd->getCylLo(), //	LBA_MID,
						  cmd->getCylHi(), //	LBA_HIGH,
						  0, //	DEVICE,
						  cmd->getStatus(),//	COMMAND, smart
						  0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, cmd->getTimeoutMS() );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( " sendSMARTCommand command succeeded\n");
		ataResult = kATANoErr;
		err=kIOReturnSuccess;
	}
    else
    {
        IOLog ( "sendSMARTCommand command failed %d %d\n", serviceResponse,GetTaskStatus ( request ) );
		if (GetTaskStatus ( request ) == kSCSITaskStatus_TaskTimeoutOccurred) {
			ataResult = kATATimeoutErr;
			err=kIOReturnSuccess;
		} else {
			ataResult = kATAErrUnknownType;
			err=kIOReturnError;
		}
    }
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:

ErrorExit:

	if (err == kIOReturnSuccess) {
		cmd->setResult(ataResult);
		cmd->executeCallback();
	}
	
	IOLog("sendSMARTCommand ret %d %d %s\n", err, ataResult, stringFromReturn(err));
	return err;
}


IOReturn 	org_dungeon_driver_IOSATDriver::setPowerState ( UInt32			powerStateOrdinal,
								   IOService * 	whatDevice ) {
    IOLog("setPowerState %p %d\n", this, (int)powerStateOrdinal);
	IOReturn err= super::setPowerState(powerStateOrdinal, whatDevice);
    IOLog("setPowerState %p %d %x\n", this, (int)powerStateOrdinal, err);
	return err;
}


bool
org_dungeon_driver_IOSATDriver::InitializeDeviceSupport ( void )
{
    bool    result = false;
    IOLog("InitializeDeviceSupport %p\n", this);
    result = super::InitializeDeviceSupport ( );
    if ( result == true ) {
		//SendBuiltInINQUIRY ( );
        //SendCreatedINQUIRY ( );
		//SendBuiltInLOG_SENSE();
		Send_ATA_IDENTIFY();
		//Send_ATA_DEVICE_CONFIGURATION_IDENTIFY();
		Send_ATA_SMART_READ_DATA();
		//if (!smartOk) result = false;

		setProperty (kIOPropertyVendorNameKey, GetVendorString() );
		setProperty ( kIOPropertyProductNameKey, GetProductString() );
		setProperty ( kIOPropertyProductRevisionLevelKey, GetRevisionString());
		
	}
    return result;
}


char *		org_dungeon_driver_IOSATDriver::GetVendorString ( void ) {
	serial[sizeof(serial)-1]=0;
	if (*serial) return serial;
	//strncpy(serial, "vendor", sizeof(serial)-1);
	//return serial;
	return super::GetVendorString();
}

char *		org_dungeon_driver_IOSATDriver::GetProductString ( void ) {
	model[sizeof(model)-1]=0;
	if (*model) return model;
	//strncpy(model, "product", sizeof(model)-1);
	//return model;
	return super::GetProductString();
}

char *		org_dungeon_driver_IOSATDriver::GetRevisionString ( void ) {
	revision[sizeof(revision)-1] =0;
	if (*revision) return revision;
	//strncpy(revision, "revision",sizeof(revision)-1);
	//return revision;
	return super::GetRevisionString();
}

void
org_dungeon_driver_IOSATDriver::SendBuiltInLOG_SENSE ( void )
{
    // The Service Response represents the execution status of a service request.
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *         buffer  = NULL;
    SCSITaskIdentifier                 request = NULL;
    UInt8 *                            ptr     = NULL;
	//int i;
	SCSI_Sense_Data * 	data = NULL;
	
    IOLog("SendBLogSense\n");
    buffer = IOBufferMemoryDescriptor::withCapacity ( 512, kIODirectionIn, false );
    require ( ( buffer != NULL ), ErrorExit );
	
    // Get the address of the beginning of the buffer and zero-fill the buffer.
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
	
    // Create a new SCSITask object; if unsuccessful, release
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    if ( LOG_SENSE (  request,
					buffer,
					0, //PPC
					0, //SP 
					0x01, // PC
					0, // PAGE CODE
					0, // PARAMETER POINTER
					buffer->getLength(), // ALLOCATION LEN
					0 ) == true )
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
	
    // Check the SendCommand method's return value and the status of the SCSITask object.
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( "LOG_SENSE succeeded\n");
		data = (SCSI_Sense_Data *)(buffer->getBytesNoCopy());
		IOLog ( "LOG_SENSE succeeded %d %d %d\n", data->VALID_RESPONSE_CODE, data->SEGMENT_NUMBER, data->SENSE_KEY );
    }
    else
    {
        IOLog ( "LOG_SENSE failed %d %d\n",serviceResponse,GetTaskStatus ( request ) );
    }
	
    buffer->complete ( );
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}

void
org_dungeon_driver_IOSATDriver::Send_ATA_IDENTIFY ( void )
{
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier                request            = NULL;
    UInt8 *                            ptr                = NULL;
	UInt16 * ataIdentify = NULL;
	
    IOLog("Send_ATA_IDENTIFY\n");
	
    // Get a new IOBufferMemoryDescriptor object with a buffer large enough
    // to hold the SCSICmd_INQUIRY_StandardData structure (defined in
    // SCSICmds_INQUIRY_Definitions.h).
    buffer = IOBufferMemoryDescriptor::withCapacity ( 512, kIODirectionIn, false );
	
    // Return immediately if the buffer wasn't created.
    require ( ( buffer != NULL ), ErrorExit );
	
    // Get the address of the beginning of the buffer and zero-fill the buffer.
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
	
    // Create a new SCSITask object; if unsuccessful, release the buffer and return.
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
	
    // Prepare the buffer for an I/O transaction. This call must be
    // balanced by a call to the complete method (shown just before
    // ReleaseTask).
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    // The BuildINQUIRY function shows how you can design and use a
    // command-building function to create a custom command to send
    // to your device. Although the BuildINQUIRY function builds a standard INQUIRY
    // command from the passed-in values, you do not create a custom function to
    // build a standard command in a real driver. Instead, you use the SCSI
    // Architecture Model family's built-in command-building functions. The
    // BuildINQUIRY function uses INQUIRY as an example merely because
    // it is a well-understood command.
    if ( PASS_THROUGH_12 ( request,
						  buffer,
						  0,//     MULTIPLE_COUNT,
						  4,//     PROTOCOL,
						  0,//     EXTEND,
						  0,//     OFF_LINE,
						  0,//     CK_COND,
						  1,//     T_DIR,
						  1,//     BYT_BLOK,
						  0x02,//     T_LENGTH,
						  0, //	FEATURES,
						  1,//	SECTOR_COUNT,
						  0, //	LBA_LOW,
						  0, //	LBA_MID,
						  0, //	LBA_HIGH,
						  0, //	DEVICE,
						  0xEC,//	COMMAND, identify
						  0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( "Send_ATA_IDENTIFY command succeeded %x\n", checksum(buffer) );
		swapbytes(buffer);
		ataIdentify = ( UInt16 * ) buffer->getBytesNoCopy ( );
		hexdump16(ataIdentify, 80, 8);
		
		trimcpy(serial, (char*)(ataIdentify+kATAIdentifySerialNumber), sizeof(serial)); 
		trimcpy(revision, (char*)(ataIdentify+kATAIdentifyFirmwareRevision), sizeof(revision)) ;
		trimcpy(model, (char*)(ataIdentify+kATAIdentifyModelNumber), sizeof(model));
		
		IOLog("serial '%s'\n",serial);
		IOLog("revision '%s'\n",revision);
		IOLog("model '%s'\n",model);
		IOLog("capabilities %04x %04x %04x\n",ataIdentify[kATAIdentifyDriveCapabilities], 
			  ataIdentify[kATAIdentifyDriveCapabilitiesExtended], 
			  ataIdentify[kATAIdentifyCommandSetSupported]);
		capabilities = ataIdentify[kATAIdentifyDriveCapabilities]&0xffff;
		smartOk = true;
	}
    else
    {
        IOLog ( "Send_ATA_IDENTIFY command failed %d %d\n", serviceResponse,GetTaskStatus ( request ) );
		smartOk = false;
    }
	
    buffer->complete ( );
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}

void
org_dungeon_driver_IOSATDriver::Send_ATA_DEVICE_CONFIGURATION_IDENTIFY ( void )
{
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier                request            = NULL;
    UInt8 *                            ptr                = NULL;
	UInt16 * ptr16;
	
    IOLog("Send_ATA_DEVICE_CONFIGURATION_IDENTIFY\n");
	
    buffer = IOBufferMemoryDescriptor::withCapacity ( 512, kIODirectionIn, false );
	
    require ( ( buffer != NULL ), ErrorExit );
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
    ptr16 = ( UInt16 * ) buffer->getBytesNoCopy ( );
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    if ( PASS_THROUGH_12 ( request,
						  buffer,
						  0,//     MULTIPLE_COUNT,
						  4,//     PROTOCOL,
						  0,//     EXTEND,
						  0,//     OFF_LINE,
						  0,//     CK_COND,
						  1,//     T_DIR,
						  1,//     BYT_BLOK,
						  0x02,//     T_LENGTH,
						  0xc2, //	FEATURES, identify
						  1,//	SECTOR_COUNT,
						  0, //	LBA_LOW,
						  0, //	LBA_MID,
						  0, //	LBA_HIGH,
						  0, //	DEVICE,
						  0xB1,//	COMMAND, configuration
						  0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( " Send_ATA_DEVICE_CONFIGURATION_IDENTIFY command succeeded %x\n", checksum(buffer) );
		swapbytes(buffer);
		hexdump16(ptr16, 0, 12);
		hexdump16(ptr16, 20, 4);
		
	}
    else
    {
        IOLog ( "Send_ATA_DEVICE_CONFIGURATION_IDENTIFY command failed %d %d\n", serviceResponse,GetTaskStatus ( request ) );
    }
	
    buffer->complete ( );
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}


void
org_dungeon_driver_IOSATDriver::Send_ATA_SMART_READ_DATA ( void )
{
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier                request            = NULL;
    UInt8 *                            ptr                = NULL;
	UInt16 * ptr16;
	
    IOLog("Send_ATA_SMART_READ_DATA\n");
	
    buffer = IOBufferMemoryDescriptor::withCapacity ( 512, kIODirectionIn, false );
	
    require ( ( buffer != NULL ), ErrorExit );
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
    ptr16 = ( UInt16 * ) buffer->getBytesNoCopy ( );
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    if ( PASS_THROUGH_12 ( request,
						  buffer,
						  0,//     MULTIPLE_COUNT,
						  4,//     PROTOCOL,
						  0,//     EXTEND,
						  0,//     OFF_LINE,
						  0,//     CK_COND,
						  1,//     T_DIR,
						  1,//     BYT_BLOK,
						  0x02,//     T_LENGTH,
						  0xd0, //	FEATURES, read data
						  1,//	SECTOR_COUNT,
						  0, //	LBA_LOW,
						  0x4f, //	LBA_MID,
						  0xc2, //	LBA_HIGH,
						  0, //	DEVICE,
						  0xB0,//	COMMAND, smart
						  0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( " Send_ATA_SMART_READ_DATA command succeeded %x\n", checksum(buffer) );
		swapbytes(buffer);
		hexdump16(ptr16, 368, 10);
		
	}
    else
    {
        IOLog ( "Send_ATA_SMART_READ_DATA command failed %d %d\n", serviceResponse,GetTaskStatus ( request ) );
    }
	
    buffer->complete ( );
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}

void
org_dungeon_driver_IOSATDriver::Send_ATA_SEND_SOFT_RESET ( void )
{
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier                request            = NULL;
	
    IOLog("Send_ATA_SEND_SOFT_RESET\n");
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
	
    if ( PASS_THROUGH_12 ( request,
						  0, // buffer
						  0,//     MULTIPLE_COUNT,
						  1,//     PROTOCOL, soft reset
						  0,//     EXTEND,
						  2,//     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
						  0,//     CK_COND,
						  0,//     T_DIR,
						  0,//     BYT_BLOK,
						  0x00,//     T_LENGTH,
						  0x00, //	FEATURES
						  0,//	SECTOR_COUNT,
						  0, //	LBA_LOW,
						  0x00, //	LBA_MID,
						  0x00, //	LBA_HIGH,
						  0, //	DEVICE,
						  0x00,//	COMMAND, smart
						  0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( " Send_ATA_SEND_SOFT_RESET command succeeded\n");
	}
    else
    {
        IOLog ( "Send_ATA_SEND_SOFT_RESET command failed %d %d\n", serviceResponse,GetTaskStatus ( request ) );
    }
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
	
ErrorExit:
    return;
}

//////////////

/*
 
 PROTOCOL:
 
 0 Hard Reset
 1 SRST
 2 Reserved
 3 Non-data
 4 PIO Data-In
 5 PIO Data-Out
 6 DMA
 7 DMA Queued
 8 Device Diagnostic
 9 DEVICE RESET
 10 UDMA Data In
 11 UDMA Data Out
 12 FPDMA  
 a
 13, 14 Reserved
 15 Return Response Informatio 
 
 
 If T_DIR is set to zero the SATL shall transfer from the application client to the ATA device. If T_DIR is set to one 
 the SATL shall transfer from the ATA device to the application client.
 */

bool
org_dungeon_driver_IOSATDriver::PASS_THROUGH_12 (
												 SCSITaskIdentifier    request,
												 IOMemoryDescriptor *    dataBuffer,
												 SCSICmdField3Bit     MULTIPLE_COUNT,
												 SCSICmdField4Bit     PROTOCOL,
												 SCSICmdField1Bit     EXTEND,
												 SCSICmdField2Bit     OFF_LINE,
												 SCSICmdField1Bit     CK_COND,
												 SCSICmdField1Bit     T_DIR,
												 SCSICmdField1Bit     BYT_BLOK,
												 SCSICmdField2Bit     T_LENGTH,
												 SCSICmdField1Byte	FEATURES,
												 SCSICmdField1Byte	SECTOR_COUNT,
												 SCSICmdField1Byte	LBA_LOW,
												 SCSICmdField1Byte	LBA_MID,
												 SCSICmdField1Byte	LBA_HIGH,
												 SCSICmdField1Byte	DEVICE,
												 SCSICmdField1Byte	COMMAND,
												 SCSICmdField1Byte	CONTROL)
{
    bool result = false;
	int direction = kSCSIDataTransfer_NoDataTransfer;
	int transferCount = 0;
	
    // Validate the parameters here.
    require ( ( request != NULL ), ErrorExit );
    require ( ResetForNewTask ( request ), ErrorExit );
	
    // The helper functions ensure that the parameters fit within the
    // CDB fields and that the buffer passed in is large enough for
    // the transfer length.
    require ( IsParameterValid ( MULTIPLE_COUNT, kSCSICmdFieldMask3Bit ), ErrorExit );
    require ( IsParameterValid ( PROTOCOL, kSCSICmdFieldMask4Bit ), ErrorExit );
    require ( IsParameterValid ( EXTEND, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( OFF_LINE, kSCSICmdFieldMask2Bit ), ErrorExit );
    require ( IsParameterValid ( CK_COND, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( T_DIR, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( BYT_BLOK, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( T_LENGTH, kSCSICmdFieldMask2Bit ), ErrorExit );
	require ( IsParameterValid ( FEATURES, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( SECTOR_COUNT, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_LOW, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_MID, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_HIGH, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( DEVICE, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( COMMAND, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( CONTROL, kSCSICmdFieldMask1Byte ), ErrorExit );
	
	switch (PROTOCOL) {
		case 0: // Hard Reset
		case 1: // SRST
		case 9: // DEVICE RESET
		case 3: // Non-data
			transferCount = 0;
			direction = kSCSIDataTransfer_NoDataTransfer;
			break;
		case 4: // PIO Data-In
		case 10: // UDMA Data In
			require (T_DIR, ErrorExit);
			require (dataBuffer, ErrorExit);
			require ( IsMemoryDescriptorValid ( dataBuffer, dataBuffer->getLength() ), ErrorExit );
			transferCount = dataBuffer->getLength();
			direction =  kSCSIDataTransfer_FromTargetToInitiator;
			break;
		case 5: // PIO Data-Out
		case 11: // UDMA Data Out
			require (!T_DIR, ErrorExit);
			require (dataBuffer, ErrorExit);
			require ( IsMemoryDescriptorValid ( dataBuffer, dataBuffer->getLength() ), ErrorExit );
			transferCount = dataBuffer->getLength();
			direction = kSCSIDataTransfer_FromInitiatorToTarget;
			break;
		case 2: // Reserved
		case 6: // DMA
		case 7: // DMA Queued
		case 8: // Device Diagnostic
		case 12: //FPDMA  
		default:
			if (!dataBuffer) {
				direction = kSCSIDataTransfer_NoDataTransfer;
				transferCount = 0;
			} else {
				require (dataBuffer, ErrorExit);
				require ( IsMemoryDescriptorValid ( dataBuffer, dataBuffer->getLength() ), ErrorExit );
				transferCount = dataBuffer->getLength();
				if (T_DIR) {
					direction = kSCSIDataTransfer_FromTargetToInitiator;
				} else {
					direction = kSCSIDataTransfer_FromInitiatorToTarget;
				}
		}	
	}
	
    // This is a 16-byte command: fill out the CDB appropriately
    SetCommandDescriptorBlock ( request,
							   kSCSICmd_PASS_THROUGH_12,
							   (MULTIPLE_COUNT << 5) | (PROTOCOL << 1) | EXTEND,
							   (OFF_LINE<<6)|(CK_COND<<5)|(T_DIR<<3)|(BYT_BLOK<<2) | T_LENGTH,
							   (FEATURES & 0xff),
							   (SECTOR_COUNT & 0xff),
							   (LBA_LOW & 0xff),
							   (LBA_MID & 0xff),
							   (LBA_HIGH & 0xff),
							   DEVICE,
							   COMMAND,
							   0,
							   CONTROL);

	SetTimeoutDuration ( request, 0 );
	SetDataTransferDirection ( request, direction);
	SetRequestedDataTransferCount ( request, transferCount );
	if (transferCount > 0) {
		SetDataBuffer ( request, dataBuffer );
	}
	
    result = true;
	
ErrorExit:
    return result;
}

bool
org_dungeon_driver_IOSATDriver::PASS_THROUGH_16 (
												 SCSITaskIdentifier    request,
												 IOMemoryDescriptor *    dataBuffer,
												 SCSICmdField3Bit     MULTIPLE_COUNT,
												 SCSICmdField4Bit     PROTOCOL,
												 SCSICmdField1Bit     EXTEND,
												 SCSICmdField2Bit     OFF_LINE,
												 SCSICmdField1Bit     CK_COND,
												 SCSICmdField1Bit     T_DIR,
												 SCSICmdField1Bit     BYT_BLOK,
												 SCSICmdField2Bit     T_LENGTH,
												 SCSICmdField2Byte	FEATURES,
												 SCSICmdField2Byte	SECTOR_COUNT,
												 SCSICmdField2Byte	LBA_LOW,
												 SCSICmdField2Byte	LBA_MID,
												 SCSICmdField2Byte	LBA_HIGH,
												 SCSICmdField1Byte	DEVICE,
												 SCSICmdField1Byte	COMMAND,
												 SCSICmdField1Byte	CONTROL)
{
    bool result = false;
	
    // Validate the parameters here.
    require ( ( request != NULL ), ErrorExit );
    require ( ResetForNewTask ( request ), ErrorExit );
	
    // The helper functions ensure that the parameters fit within the
    // CDB fields and that the buffer passed in is large enough for
    // the transfer length.
    require ( IsParameterValid ( MULTIPLE_COUNT, kSCSICmdFieldMask3Bit ), ErrorExit );
    require ( IsParameterValid ( PROTOCOL, kSCSICmdFieldMask4Bit ), ErrorExit );
    require ( IsParameterValid ( EXTEND, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( OFF_LINE, kSCSICmdFieldMask2Bit ), ErrorExit );
    require ( IsParameterValid ( CK_COND, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( T_DIR, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( BYT_BLOK, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( T_LENGTH, kSCSICmdFieldMask2Bit ), ErrorExit );
	require ( IsParameterValid ( FEATURES, kSCSICmdFieldMask2Byte ), ErrorExit );
	require ( IsParameterValid ( SECTOR_COUNT, kSCSICmdFieldMask2Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_LOW, kSCSICmdFieldMask2Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_MID, kSCSICmdFieldMask2Byte ), ErrorExit );
	require ( IsParameterValid ( LBA_HIGH, kSCSICmdFieldMask2Byte ), ErrorExit );
	require ( IsParameterValid ( DEVICE, kSCSICmdFieldMask1Byte ), ErrorExit );
	require ( IsParameterValid ( COMMAND, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( CONTROL, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsMemoryDescriptorValid ( dataBuffer, dataBuffer->getLength() ), ErrorExit );
	
    // This is a 16-byte command: fill out the CDB appropriately
    SetCommandDescriptorBlock ( request,
							   kSCSICmd_PASS_THROUGH_16,
							   (MULTIPLE_COUNT << 5) | (PROTOCOL << 1) | EXTEND,
							   (OFF_LINE<<6)|(CK_COND<<5)|(T_DIR<<3)|(BYT_BLOK<<2) | T_LENGTH,
							   (FEATURES>>8),
							   (FEATURES & 0xff),
							   (SECTOR_COUNT>>8),
							   (SECTOR_COUNT & 0xff),
							   (LBA_LOW>>8),
							   (LBA_LOW & 0xff),
							   (LBA_MID>>8),
							   (LBA_MID & 0xff),
							   (LBA_HIGH>>8),
							   (LBA_HIGH & 0xff),
							   DEVICE,
							   COMMAND,
							   CONTROL);
	
    SetDataTransferDirection ( request, kSCSIDataTransfer_FromTargetToInitiator );
    SetTimeoutDuration ( request, 0 );
    SetDataBuffer ( request, dataBuffer );
    SetRequestedDataTransferCount ( request, dataBuffer->getLength() );
	
    result = true;
	
ErrorExit:
    return result;
}



void
org_dungeon_driver_IOSATDriver::SendBuiltInINQUIRY ( void )
{
    // The Service Response represents the execution status of a service request.
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *         buffer  = NULL;
    SCSITaskIdentifier                 request = NULL;
    UInt8 *                            ptr     = NULL;
	//int i;
	SCSICmd_INQUIRY_StandardDataAll * 	inquiryBuffer = NULL;
	//UInt8							inquiryBufferCount = sizeof ( SCSICmd_INQUIRY_StandardData );
	
    IOLog("SendBInquiry\n");
	//inquiryBuffer = ( SCSICmd_INQUIRY_StandardData * ) IOMalloc ( inquiryBufferCount );
	//if ( inquiryBuffer == NULL )
	//{
	//STATUS_LOG ( ( "%s: Couldn't allocate Inquiry buffer.\n", getName ( ) ) );
	//goto ErrorExit;
	//}
	
    // Get a new IOBufferMemoryDescriptor object with a buffer large enough
    // to hold the SCSICmd_INQUIRY_StandardData structure (defined
    // in SCSICmds_INQUIRY_Definitions.h).
    buffer = IOBufferMemoryDescriptor::withCapacity ( sizeof ( SCSICmd_INQUIRY_StandardDataAll ), kIODirectionIn, false );
	
	
    require ( ( buffer != NULL ), ErrorExit );
	
    // Get the address of the beginning of the buffer and zero-fill the buffer.
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
	
    // Create a new SCSITask object; if unsuccessful, release
	
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
	
    // Prepare the buffer for an I/O transaction. This call must be
    // balanced by a call to the complete method (shown just before
    // ReleaseTask).
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    // Use the INQUIRY method to assemble the command. Then use the
    // SendCommand method to synchronously issue the request.
	
    if ( INQUIRY (  request,
				  buffer,
				  0,
				  0,
				  0x00,
				  sizeof ( SCSICmd_INQUIRY_StandardData ),
				  0 ) == true )
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
	
    // Check the SendCommand method's return value and the status of the SCSITask object.
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( "INQUIRY succeeded %d\n", (int)buffer->getLength() );
		inquiryBuffer = (SCSICmd_INQUIRY_StandardDataAll *)(buffer->getBytesNoCopy());
		IOLog ( "INQUIRY succeeded %d '%s' '%s'\n", inquiryBuffer->ADDITIONAL_LENGTH, inquiryBuffer->VENDOR_IDENTIFICATION, inquiryBuffer->PRODUCT_IDENTIFICATION );
    }
    else
    {
        IOLog ( "INQUIRY failed\n" );
    }
	
    // Complete the processing of this buffer after the I/O transaction
    // (this call balances the earlier call to prepare).
    buffer->complete ( );
	
	// Clean up before exiting.
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}

void
org_dungeon_driver_IOSATDriver::SendCreatedINQUIRY ( void )
{
    SCSIServiceResponse                serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier                request            = NULL;
    UInt8 *                            ptr                = NULL;
	SCSICmd_INQUIRY_StandardDataAll * 	inquiryBuffer = NULL;
	//int i;
	
    IOLog("SendCInquiry\n");
	
    // Get a new IOBufferMemoryDescriptor object with a buffer large enough
    // to hold the SCSICmd_INQUIRY_StandardData structure (defined in
    // SCSICmds_INQUIRY_Definitions.h).
    buffer = IOBufferMemoryDescriptor::withCapacity ( sizeof ( SCSICmd_INQUIRY_StandardDataAll ), kIODirectionIn, false );
	
    // Return immediately if the buffer wasn't created.
    require ( ( buffer != NULL ), ErrorExit );
	
    // Get the address of the beginning of the buffer and zero-fill the buffer.
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
	
    // Create a new SCSITask object; if unsuccessful, release the buffer and return.
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
	
    // Prepare the buffer for an I/O transaction. This call must be
    // balanced by a call to the complete method (shown just before
    // ReleaseTask).
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
	
    // The BuildINQUIRY function shows how you can design and use a
    // command-building function to create a custom command to send
    // to your device. Although the BuildINQUIRY function builds a standard INQUIRY
    // command from the passed-in values, you do not create a custom function to
    // build a standard command in a real driver. Instead, you use the SCSI
    // Architecture Model family's built-in command-building functions. The
    // BuildINQUIRY function uses INQUIRY as an example merely because
    // it is a well-understood command.
    if ( BuildINQUIRY ( request,
					   buffer,
					   0x00, // CMDDT (Command support data)
					   0x00, // EVPD  (Vital product data)
					   0x00, // PAGE_OR_OPERATION_CODE
					   250, // ALLOCATION_LENGTH
					   0x00 )  // CONTROL
		== true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
		GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        IOLog ( "Vendor-created INQUIRY command succeeded\n" );
		inquiryBuffer = (SCSICmd_INQUIRY_StandardDataAll *)(buffer->getBytesNoCopy());
		IOLog ( "INQUIRY succeeded %d '%s' '%s'\n", inquiryBuffer->ADDITIONAL_LENGTH, inquiryBuffer->VENDOR_IDENTIFICATION, inquiryBuffer->PRODUCT_IDENTIFICATION );
		//if (inquiryBuffer->ADDITIONAL_LENGTH > 31) {
		//for (i = 0; i < 8; i++) {
		//	IOLog ( "INQUIRY %d %04x\n", i, (int)inquiryBuffer->VERSION_DESCRIPTOR[i] );
		//}
		//}
    }
    else
    {
        IOLog ( "Vendor-created INQUIRY command failed\n" );
    }
	
    buffer->complete ( );
	
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
	
ReleaseBuffer:
    require_quiet ( ( buffer != NULL ), ErrorExit );
    buffer->release ( );
    buffer = NULL;
	
ErrorExit:
    return;
}

bool
org_dungeon_driver_IOSATDriver::BuildINQUIRY (
											  SCSITaskIdentifier    request,
											  IOBufferMemoryDescriptor *    dataBuffer,
											  SCSICmdField1Bit     CMDDT,
											  SCSICmdField1Bit     EVPD,
											  SCSICmdField1Byte     PAGE_OR_OPERATION_CODE,
											  SCSICmdField1Byte     ALLOCATION_LENGTH,
											  SCSICmdField1Byte     CONTROL )
{
    bool result = false;
	
    // Validate the parameters here.
    require ( ( request != NULL ), ErrorExit );
    require ( ResetForNewTask ( request ), ErrorExit );
	
    // The helper functions ensure that the parameters fit within the
    // CDB fields and that the buffer passed in is large enough for
    // the transfer length.
    require ( IsParameterValid ( CMDDT, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( EVPD, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( PAGE_OR_OPERATION_CODE, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( ALLOCATION_LENGTH, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( CONTROL, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsMemoryDescriptorValid ( dataBuffer, ALLOCATION_LENGTH ), ErrorExit );
	
    // Check the validity of the PAGE_OR_OPERATION_CODE parameter, when using both the CMDDT and EVPD parameters.
	
    if ( PAGE_OR_OPERATION_CODE != 0 )
    {
        if ( ( ( CMDDT == 1 ) && ( EVPD == 1 ) ) || ( ( CMDDT == 0 ) && ( EVPD == 0 ) ) )
        {
            goto ErrorExit;
        }
    }
	
    // This is a 6-byte command: fill out the CDB appropriately
    SetCommandDescriptorBlock ( request,
							   kSCSICmd_INQUIRY,
							   ( CMDDT << 1 ) | EVPD,
							   PAGE_OR_OPERATION_CODE,
							   0x00,
							   ALLOCATION_LENGTH,
							   CONTROL );
	
    SetDataTransferDirection ( request, kSCSIDataTransfer_FromTargetToInitiator );
    SetTimeoutDuration ( request, 0 );
    SetDataBuffer ( request, dataBuffer );
    SetRequestedDataTransferCount ( request, ALLOCATION_LENGTH );
	
    result = true;
	
ErrorExit:
    return result;
}




// Padding for future binary compatibility.
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 0);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 1);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 2);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 3);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 4);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 5);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 6);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 7);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 8);
OSMetaClassDefineReservedUnused(org_dungeon_driver_IOSATDriver, 9);

