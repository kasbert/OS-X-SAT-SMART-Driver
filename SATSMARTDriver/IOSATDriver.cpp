/*
 *  IOSATDriver.cpp
 *
 *  Created by Jarkko Sonninen on 3.2.2012.
 *  Copyright 2012 Jarkko Sonninen. All rights reserved.
 *
 */

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/scsi/SCSICmds_INQUIRY_Definitions.h>
#include <IOKit/scsi/SCSICommandOperationCodes.h>
#include <IOKit/scsi/SCSITask.h>
#include <IOKit/IOKitKeys.h>
#include <AssertMacros.h>

#include <IOKit/storage/IOBlockStorageDriver.h>
#include <IOKit/scsi/IOSCSIProtocolServices.h>
#include <IOKit/scsi/IOSCSIPeripheralDeviceNub.h>

#include <IOKit/storage/ata/IOATAStorageDefines.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>

#include "IOSATBusCommand.h"
#include "IOSATDriver.h"
#include "IOSATServices.h"

extern const double SATSMARTDriverVersionNumber;

#ifdef DEBUG
#define DEBUG_LOG IOLog
#else
#define DEBUG_LOG(...)
#endif

#define ERROR_LOG IOLog

#define getClassName() "IOSATDriver"

#define Endian16_Swap(value) \
((((UInt16)((value) & 0x00FF)) << 8) | \
(((UInt16)((value) & 0xFF00)) >> 8))

#define kSCSICmd_PASS_THROUGH_16 0x85
#define kSCSICmd_PASS_THROUGH_12 0xA1
#define kSCSICmd_PASS_THROUGH_JMicron 0xdf

// S.M.A.R.T command code
enum
{
    kATAcmdSMART    = 0xB0
};

// S.M.A.R.T command features register codes
enum
{
    kFeaturesRegisterReadData               = 0xD0,
    kFeaturesRegisterReadDataThresholds     = 0xD1,
    kFeaturesRegisterEnableDisableAutoSave  = 0xD2,
    // Reserved
    kFeaturesRegisterExecuteOfflineImmed    = 0xD4,
    kFeaturesRegisterReadLogAtAddress       = 0xD5,
    kFeaturesRegisterWriteLogAtAddress      = 0xD6,
    // Reserved
    kFeaturesRegisterEnableOperations       = 0xD8,
    kFeaturesRegisterDisableOperations      = 0xD9,
    kFeaturesRegisterReturnStatus           = 0xDA
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


#define super IOSCSIPeripheralDeviceType00
OSDefineMetaClassAndStructors(fi_dungeon_driver_IOSATDriver, IOSCSIPeripheralDeviceType00)

/////////////
static void hexdump16(UInt16 *data, int start, int len) {
    int i, j;
    for (i = start; i < start + len; i+=8) {
        DEBUG_LOG ( "%d:", i);
        for (j = 0; j < 8 && i + j < start + len; j++) {
            DEBUG_LOG ( " %04x", (int)data[i+j]);
        }
        DEBUG_LOG ( "\n");
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
    i--; q--;
    for(; *q == ' ' && i >= 0; i--, q--) *q = 0;
}

void swapbytes(IOBufferMemoryDescriptor *        buffer) {
    int i;
    UInt16 *ptr = ( UInt16 * ) buffer->getBytesNoCopy ( );
    for (i = 0; i < buffer->getLength()/sizeof(UInt16); i++) {
        ptr[i]=Endian16_Swap(ptr[i]);
    }
}

int countzeros(IOBufferMemoryDescriptor *        buffer) {
    int i, count = 0;
    UInt8 *ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    for (i = 0; i < buffer->getLength(); i++) {
        if (ptr[i] == 0) {
            count ++;
        }
    }
    return count;
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


bool fi_dungeon_driver_IOSATDriver::init(OSDictionary *dict)
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    bool result = super::init(dict);
    memset(serial, 0, sizeof(serial));
    memset(model, 0, sizeof(model));
    memset(revision, 0, sizeof(revision));
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

void fi_dungeon_driver_IOSATDriver::free(void)
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    super::free();
}

IOService *fi_dungeon_driver_IOSATDriver::probe(IOService *provider,
                                                 SInt32 *score)
{
    DEBUG_LOG("%s[%p]::%s score %d\n", getClassName(), this, __FUNCTION__, score ? (int)*score : -1);
    IOService *result = super::probe(provider, score);
    //if (result != NULL) {
    // the following doesn not work
    //if (start(provider)) {
    //		Send_ATA_IDENTIFY();
    //		stop(provider);
    //	}
    //if (!fSATSMARTCapable) result = 0;
    //}
    
    OSNumber *idVendor = OSDynamicCast(OSNumber, getParentProperty("idVendor"));
    OSNumber *idProduct = OSDynamicCast(OSNumber, getParentProperty("idProduct"));
    OSString *vendorID = OSDynamicCast ( OSString, getParentProperty("Vendor Identification"));
    OSString *productID = OSDynamicCast ( OSString, getParentProperty("Product Identification"));
    OSNumber *fwvendorID = OSDynamicCast ( OSNumber, getParentProperty("Vendor_ID"));
    OSNumber *fwmodelID = OSDynamicCast ( OSNumber, getParentProperty("Model_ID"));
    
    OSDictionary*   dict= OSDynamicCast ( OSDictionary, getProperty("Identifiers"));
    
    if (dict) {
        OSDictionary* details = 0;
        OSDictionary* unknown = 0;
        OSCollectionIterator *iterator = OSCollectionIterator::withCollection(dict);
        OSObject *object; 
        const char * name = 0;
        char buffer[30];
        
        while ((object = iterator->getNextObject())) {
            OSString *key = OSDynamicCast (OSString, object);
            if (!key) {
                continue;
            }
            details= OSDynamicCast ( OSDictionary, dict->getObject(key));
            if (!details) {
                continue;
            }
            // Special entry
            if (key->isEqualTo("UNKNOWN")) {
                unknown = details;
                continue;
            }
            name = key->getCStringNoCopy();
            OSNumber *idVendor2 = OSDynamicCast ( OSNumber, details->getObject("idVendor"));
            OSNumber *idProduct2 = OSDynamicCast ( OSNumber, details->getObject("idProduct"));
            OSString *vendorID2 = OSDynamicCast ( OSString, details->getObject("Vendor Identification"));
            OSString *productID2 = OSDynamicCast ( OSString, details->getObject("Product Identification"));
            OSNumber *fwvendorID2 = OSDynamicCast ( OSNumber, details->getObject("Vendor_ID"));
            OSNumber *fwmodelID2 = OSDynamicCast ( OSNumber, details->getObject("Model_ID"));
            if (idVendor && idVendor2 && idProduct && idProduct2 
                && idVendor->unsigned32BitValue() == idVendor2->unsigned32BitValue()
                && idProduct->unsigned32BitValue() == idProduct2->unsigned32BitValue()) {
                DEBUG_LOG("%s[%p]::%s '%s' MATCH %04x:%04x\n", getClassName(), this, __FUNCTION__, name, idVendor->unsigned32BitValue(), idProduct->unsigned32BitValue());
                *score += 5000;
                break;
            }
            if (vendorID && vendorID2 && productID && productID2 
                && vendorID->isEqualTo(vendorID2) && productID->isEqualTo(productID2)) {
                DEBUG_LOG("%s[%p]::%s '%s' MATCH '%s':'%s'\n", getClassName(), this, __FUNCTION__, name, vendorID->getCStringNoCopy(), productID->getCStringNoCopy());
                *score += 5000;
                break;
            }
            if (fwvendorID && fwvendorID2 && fwmodelID && fwmodelID2 
                && fwvendorID->unsigned32BitValue() == fwvendorID2->unsigned32BitValue() 
                && fwmodelID->unsigned32BitValue() == fwmodelID2->unsigned32BitValue()) {
                DEBUG_LOG("%s[%p]::%s '%s' MATCH %04x:%04x\n", getClassName(), this, __FUNCTION__, name, fwvendorID->unsigned32BitValue(), fwmodelID->unsigned32BitValue());
                *score += 5000;
                break;
            }
            details = 0;
        } 
        iterator->release(); 
        if (!details && unknown) {
            details = unknown;
            name = 0;
        }
        if (!name) {
            if (!idVendor && !idProduct && fwvendorID && fwmodelID) {
                snprintf(buffer, 30, "Unknown fw %04x:%04x", fwvendorID->unsigned32BitValue(), fwmodelID->unsigned32BitValue());
            } else {
                snprintf(buffer, 30, "Unknown %04x:%04x", idVendor ? idVendor->unsigned32BitValue() : 0, idProduct ? idProduct->unsigned32BitValue() : 0);
            }
            name = buffer;
        }
        setProperty(kEnclosureName, name);
        
        if (details) {
            OSString *options = OSDynamicCast ( OSString, details->getObject(kPassThroughMode));
            if (options) {
                DEBUG_LOG("%s[%p]::%s '%s' PassThroughMode %s\n", getClassName(), this, __FUNCTION__, name, options->getCStringNoCopy());
                setProperty(kPassThroughMode, options->getCStringNoCopy());
                if (options->isEqualTo("DISABLE")) {
                    DEBUG_LOG("%s[%p]::%s DISABLED\n", getClassName(), this, __FUNCTION__);
                    *score = -5000;
                    // TODO Consider removing
                    IOLog("SATSMARTDriver v%d.%d: enclosure '%s' is not SAT capable, skipping\n",
                          (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
                          name);
                }
            }
            OSBoolean *permissive = OSDynamicCast ( OSBoolean, details->getObject(kPermissiveKey));
            if (permissive) {
                setProperty(kPermissiveKey, permissive);
            }
            OSBoolean *delayIdentify = OSDynamicCast ( OSBoolean, details->getObject(kDelayIdentifyKey));
            if (delayIdentify) {
                setProperty(kDelayIdentifyKey, delayIdentify);
            }
            
        }
    }
    
    DEBUG_LOG("%s[%p]::%s result %p score %d\n", getClassName(), this,  __FUNCTION__, result, score ? (int)*score : -1);
    return result;
}

OSObject *fi_dungeon_driver_IOSATDriver::getParentProperty(const char *key) {
    IOService *parent = this;
    OSObject *value = NULL;
    while ((parent = parent->getProvider())) {
        value = parent->getProperty(key);
        if (value) {
            return value;
        }
    }
    return 0;
}

bool fi_dungeon_driver_IOSATDriver::start(IOService *provider)
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    OSBoolean *permissive = OSDynamicCast ( OSBoolean, getProperty(kPermissiveKey));
    if  (permissive != NULL && permissive->isTrue()) {
        fPermissive = true;
    } else {
        fPermissive = false;
    }
    OSBoolean *delayIdentify = OSDynamicCast ( OSBoolean, getProperty(kDelayIdentifyKey));
    if  (delayIdentify != NULL && delayIdentify->isTrue()) {
        fDelayIdentify = true;
    } else {
        fDelayIdentify = false;
    }
    OSString* value = OSDynamicCast ( OSString, getProperty(kPassThroughMode));
    if (value) {
        if (!strcmp(value->getCStringNoCopy(), "sat12")) {
            fPassThroughMode = kPassThroughModeSAT12;
        } else if (value->isEqualTo("sat16")) {
            fPassThroughMode = kPassThroughModeSAT16;
        } else if (value->isEqualTo("jmicron")) {
            fPassThroughMode = kPassThroughModeJMicron;
        } else if (value->isEqualTo("auto")) {
            fPassThroughMode = kPassThroughModeAuto;
        } else if (value->isEqualTo("none")) {
            fPassThroughMode = kPassThroughModeNone;
        } else {
            fPassThroughMode = kPassThroughModeAuto;
        }
    } else {
        fPassThroughMode = kPassThroughModeAuto;
    }
    if (true || fSATSMARTCapable) {
	unsigned long features = kIOATAFeatureSMART;
	OSNumber *number = OSNumber::withNumber(features, 32);
	if (number) {
            setProperty ( kIOATASupportedFeaturesKey, number );
            number->release();
        }
    }
    
    bool result = super::start(provider); // will call CreateStorageServiceNub
    OSString *name = OSDynamicCast(OSString, getProperty(kEnclosureName));
    __Require (result, ErrorExit);
    
    if (fSATSMARTCapable) {
        if (fDelayIdentify) {
            IOLog("SATSMARTDriver v%d.%d: enclosure '%s', identify drive later'\n",
                  (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
                  name ? name->getCStringNoCopy() : "unknown");
        } else {
            /*
            IOLog("SATSMARTDriver v%d.%d: enclosure '%s', disk serial '%s', revision '%s', model '%s'\n",
                  (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
                  name ? name->getCStringNoCopy() : "unknown",
                  serial, revision, model);
             */
        }
    } else {
        IOLog("SATSMARTDriver v%d.%d: enclosure '%s', disk is not SAT capable\n",
              (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
              name ? name->getCStringNoCopy() : "unknown");
        //result = false;
    }
    if (!result) {
        // Stop is not called on failure
        //TerminateDeviceSupport();
        //stop(provider);
    }
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

void fi_dungeon_driver_IOSATDriver::stop(IOService *provider)
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    super::stop(provider);
}

// This function will be called when the user process calls IORegistryEntrySetCFProperties on
// this driver. You can add your custom functionality to this function.
IOReturn fi_dungeon_driver_IOSATDriver::setProperties(OSObject* properties)
{
    OSDictionary*   dict;
    OSNumber*       number;
    IOReturn result = kIOReturnSuccess;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    dict = OSDynamicCast(OSDictionary, properties);
    require_action(dict, ErrorExit, result = kIOReturnBadArgument);
    
    number = OSDynamicCast(OSNumber, dict->getObject(kMyPropertyKey));
    if (!number) {
        result = super::setProperties(properties);
    } else {
        UInt32 value= number->unsigned32BitValue();
        
        DEBUG_LOG("%s[%p]::%s(%p) got value %u\n", getName(), this, __FUNCTION__, properties, (unsigned int)value);
        
        // Some code to experiment the SAT commands
#if 1
        switch (value) {
            case 1:
                SendBuiltInINQUIRY ( );
                break;
            case 2: {
                int ret = Send_ATA_IDENTIFY();
                IOLog("identify %d\n", ret);
                if (ret) {
                    IOLog("SATSMARTDriver v%d.%d: disk serial '%s', revision '%s', model '%s'\n",
                          (int)SATSMARTDriverVersionNumber, ((int)(10*SATSMARTDriverVersionNumber))%10, 
                          serial, revision, model);
                }
                break;
            }
            case 3:
                IOLog("smart read %d\n", Send_ATA_SMART_READ_DATA());
                break;
            case 4:
            {
                UInt8 status = -1;
                IOService *fProvider = getProvider();
                if (fProvider) {
                    OSNumber *lun = OSDynamicCast ( OSNumber, fProvider->getProperty("IOUnitLUN"));
                    if (lun) {
                        DEBUG_LOG("%s[%p]::%s LUN %d\n", getClassName(), this, __FUNCTION__, lun->unsigned32BitValue());
                        
                        //value->release();
                    }
                }
                if (JMicron_get_registers(0x720f, & status, sizeof status)) {
                    DEBUG_LOG("%s[%p]::%s register value %02x\n", getClassName(), this, __FUNCTION__, (int) status);
                }
            }
                
            case 5:
                IOLog("IdentifyDevice() %d\n", IdentifyDevice());
                break;
                
            case 12: {
                //fPort = 0;
                //fDevice = 0;
                int oldPassThroughMode = fPassThroughMode;
                fPassThroughMode = kPassThroughModeJMicron; // FIXME race condition
                int ret = Send_ATA_IDENTIFY();
                fPassThroughMode = oldPassThroughMode;
                IOLog("JMicron identify %d port %d device %d\n", ret, fPort, fDevice);
                if (ret) {
                    IOLog("JMicron v%d.%d: disk serial '%s', revision '%s', model '%s'\n",
                          (int)SATSMARTDriverVersionNumber, ((int)(10*SATSMARTDriverVersionNumber))%10, 
                          serial, revision, model);
                }
                break;
            }
            case 13: {
                //fPort = 0;
                //fDevice = 0;
                int oldPassThroughMode = fPassThroughMode;
                fPassThroughMode = kPassThroughModeJMicron; // FIXME race condition
                IOLog("JMicron smart read %d\n", Send_ATA_SMART_READ_DATA());
                fPassThroughMode = oldPassThroughMode;
                break;
            }
            case 14: {
                fPort = fPort ^ 1;
                IOLog("JMicron port %d device %d\n", fPort, fDevice);
                break;
            }
            case 15: {
                fDevice = fDevice ^ 1;
                IOLog("JMicron port %d device %d\n", fPort, fDevice);
                break;
            }
                
            case 20:
		IOLog("SRST %d\n", Send_ATA_SEND_SOFT_RESET());
                break;
            case 21:
                IOLog("sat %d %p\n", fDeviceHasSATTranslation, GetProtocolDriver());
                break;
            case 22:
                IOLog("targetreset %d\n", GetProtocolDriver()->TargetReset());
                break;
            case 23:
		IOLog("standby %d\n", Send_ATA_STANDBY(0));
		break;
            case 24:
		IOLog("idle %d\n", Send_ATA_IDLE(0));
		break;
            case 25:
		IOLog("standby %d\n", Send_ATA_STANDBY(12)); // 12 * 5 = 60s
 		break;
            case 26: {
                int mode = -2;
                int ret = Send_ATA_CHECK_POWER_MODE(&mode);
		IOLog("power mode %d %d\n", ret, mode);
		break;
            }
            case 27:
		IOLog("idle %d\n", Send_ATA_IDLE_IMMEDIATE());
		break;
            case 28:
		IOLog("idle %d\n", Send_ATA_STANDBY_IMMEDIATE());
		break;
            case 33:
		IOLog("standby %d\n", Send_ATA_STANDBY(253));
		break;
            case 34:
		IOLog("idle %d\n", Send_ATA_IDLE(253));
		break;
            case 40: {
                IOSATBusCommand* command= IOSATBusCommand64::allocateCmd32();
                if ( command == NULL )
                {
                    break;
                }
                
                command->setFeatures    ( kFeaturesRegisterReturnStatus );
                command->setOpcode              ( kATAFnExecIO );
                command->setTimeoutMS   ( kATAThirtySecondTimeoutInMS );
                command->setCylLo               ( kSMARTMagicCylinderLoValue );
                command->setCylHi               ( kSMARTMagicCylinderHiValue );
                command->setCommand             ( kATAcmdSMART );
                command->setRegMask             ( ( ataRegMask ) ( mATACylinderHiValid | mATACylinderLoValid ) );
                command->setFlags               ( mATAFlagTFAccessResult );
                
                IOReturn status = sendSMARTCommand ( command );
                
                UInt8 lbaMid  = command->getCylLo ( );
                UInt8 lbaHigh = command->getCylHi ( );
		IOLog("SMART status %d %02x, %02x\n", status, lbaMid,lbaHigh);
                
                if ( status == kIOReturnSuccess )
                {
                }
                command->release();
                break;
            }
                
        }
#endif
        
        result = kIOReturnSuccess;
    }
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d %s\n", getClassName(), this,  __FUNCTION__, result, stringFromReturn(result));
    return result;
}


bool
fi_dungeon_driver_IOSATDriver::attach ( IOService * provider )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    bool result = false;
    
    require_string ( super::attach ( provider ), ErrorExit,
                    "Superclass didn't attach" );
    
    result = true;
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

void
fi_dungeon_driver_IOSATDriver::detach ( IOService * provider )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    super::detach ( provider );
}

bool
fi_dungeon_driver_IOSATDriver::open ( IOService *	   forClient,
                                     IOOptionBits	   options = 0,
                                     void *		   arg = 0 ) {
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    return super::open(forClient, options, arg);
}

void
fi_dungeon_driver_IOSATDriver::CreateStorageServiceNub ( void )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    IOService *         nub = NULL;
    
    if (fDelayIdentify) {
        fSATSMARTCapable = true; // for now
        fIdentified = false;
    } else {
        // Query device identification and check SAT capability
        IdentifyDevice();
        fIdentified = true;
    }
    
    if (!fSATSMARTCapable) {
        super::CreateStorageServiceNub();
        return;
    }
    //nub = OSTypeAlloc ( IOBlockStorageServices );
    nub = OSTypeAlloc ( IOSATServices );
    require_quiet ( nub, ErrorExit );
    
    nub->init ( );
    require ( nub->attach ( this ), ReleaseNub );
    nub->registerService ( );
    nub->release ( );
    return;
    
ReleaseNub:
    nub->release ( );
    
ErrorExit:
    super::CreateStorageServiceNub();
    ERROR_LOG("%s::%s result failed\n", getClassName(), __FUNCTION__);
    return;
}

IOReturn fi_dungeon_driver_IOSATDriver::sendSMARTCommand ( IOSATCommand * command )
{
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    int ataResult = kATAErrUnknownType;
    IOReturn err = kIOReturnInvalid;
    int direction, count, protocol;
    
    if (!fIdentified) {
        // Query device identification and check SAT capability
        IdentifyDevice();
        fIdentified = true;
    }
    
    IOSATBusCommand* cmd = OSDynamicCast( IOSATBusCommand, command);
    require_action_string(cmd, ErrorExit, err = kIOReturnBadArgument, "Command is not a IOSATBusCommand");
    
    require (fSATSMARTCapable, ErrorExit );
    
    request = GetSCSITask ( );
    require (request, ErrorExit );
    
    DEBUG_LOG("buffer %p, bytecount %d, features %x, opcode %x, timeout %d, seccnt %x, secnum %x, cyllo %x, cylhi %x, dev %x, cmd %x, flags %x\n"
              , cmd->getBuffer() , (int)cmd->getByteCount(), cmd->getErrorReg()
              , cmd->getOpcode(), (int)cmd->getTimeoutMS(), cmd->getSectorCount()
              , cmd->getSectorNumber(), cmd->getCylLo(), cmd->getCylHi()
              , cmd->getDevice_Head(), cmd->getStatus(), cmd->getFlags());
    
    direction = (cmd->getFlags() & mATAFlagIORead) ? 1 : 0;
    count = 0;
    if (cmd->getBuffer() && cmd->getByteCount() == 512) {
        count = 1;
    }
    protocol = kIOSATProtocolNonData;
    if (count) {
        protocol = direction ? kIOSATProtocolPIODataIn : kIOSATProtocolPIODataOut;
    }
    DEBUG_LOG("%s[%p]::%s direction %d, count %d, protocol %d\n", getClassName(), this, __FUNCTION__, direction, count, protocol);
    if ( PASS_THROUGH_12or16 ( request,
                              cmd->getBuffer(),
                              0,               //     MULTIPLE_COUNT,
                              protocol,               //     PROTOCOL,
                              0,               //     EXTEND,
                              0,               //     OFF_LINE,
                              0,               //     CK_COND,
                              direction,               //     T_DIR,
                              1,               //     BYT_BLOK,
                              count ? 0x02 : 0,               //     T_LENGTH,
                              cmd->getErrorReg(),               //	FEATURES
                              count,               //	SECTOR_COUNT,
                              cmd->getSectorNumber(),               //	LBA_LOW,
                              cmd->getCylLo(),               //	LBA_MID,
                              cmd->getCylHi(),               //	LBA_HIGH,
                              0,               //	DEVICE,
                              cmd->getStatus(),               //	COMMAND, smart
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, cmd->getTimeoutMS() );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        (GetTaskStatus ( request ) == kSCSITaskStatus_GOOD  ||
         (fPermissive && (GetTaskStatus(request) == kSCSITaskStatus_CHECK_CONDITION))))
        // TODO senseData.VALID_RESPONSE_CODE == 0x72 && senseData.SENSE_KEY == 5) ?
    {
        DEBUG_LOG("%s[%p]::%s success, service response %d, task status %d\n",
                  getClassName(), this, __FUNCTION__, serviceResponse, GetTaskStatus ( request ));
        ataResult = kATANoErr;
        err=kIOReturnSuccess;
#if 0
        ERROR_LOG("REQUEST OK: [ %02x %02x %02x %02x %02x %02x %02x ]"
                  ", flags 0x%x"
                  ", opcode 0x%x"
                  ", timeout %dms, direction %d, count %d, protocol %d bytecount %d\n"
                  , cmd->getErrorReg()
                  , cmd->getSectorCount()
		  , cmd->getSectorNumber()
		  , cmd->getCylLo()
                  , cmd->getCylHi()
		  , cmd->getDevice_Head()
		  , cmd->getStatus()
                  , cmd->getFlags()
		  , cmd->getOpcode()
		  , (int)cmd->getTimeoutMS()
		  , direction, count, protocol, (int)cmd->getByteCount());
#endif
    }
    else
    {
        ERROR_LOG("%s::%s failed, service response %d, task status %d\n",
                  getClassName(), __FUNCTION__, serviceResponse, GetTaskStatus ( request ));
        if (GetTaskStatus ( request ) == kSCSITaskStatus_TaskTimeoutOccurred) {
            ataResult = kATATimeoutErr;
            err=kIOReturnSuccess;
        } else {
            ataResult = kATAErrUnknownType;
            err=kIOReturnError;
        }
        ERROR_LOG("REQUEST: [ %02x %02x %02x %02x %02x %02x %02x ]"
                  ", flags 0x%x"
                  ", opcode 0x%x"
                  ", timeout %d direction %d, count %d, protocol %d bytecount %d\n"
                  , cmd->getErrorReg()
                  , cmd->getSectorCount()
		  , cmd->getSectorNumber()
		  , cmd->getCylLo()
                  , cmd->getCylHi()
		  , cmd->getDevice_Head()
		  , cmd->getStatus()
                  , cmd->getFlags()
		  , cmd->getOpcode()
		  , (int)cmd->getTimeoutMS(), direction, count, protocol, (int)cmd->getByteCount());
        LogAutoSenseData (request) ;
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ErrorExit );
    ReleaseSCSITask ( request );
    request = NULL;
    
ErrorExit:
    
    if (err == kIOReturnSuccess) {
        cmd->setResult(ataResult);
        cmd->executeCallback();
    }
    
    DEBUG_LOG("%s[%p]::%s result %d %d %s\n", getClassName(), this,  __FUNCTION__, err, ataResult, stringFromReturn(err));
    return err;
}


bool
fi_dungeon_driver_IOSATDriver::JMicron_get_registers ( UInt16 address, UInt8 *ptr, UInt16 length )
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier request            = NULL;
    SCSI_Sense_Data senseData;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    buffer = IOMemoryDescriptor::withAddress(ptr, length, kIODirectionIn);
    require ( ( buffer != NULL ), ErrorExit );
    bzero ( ptr, buffer->getLength ( ) );
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
    
    if ( PASS_THROUGH_JMicron( request,
                              buffer,
                              kIOSATProtocolPIODataIn,               //     PROTOCOL,
                              kIOSATTDirectionFromDevice,               //     T_DIR,
                              0,               //	FEATURES,
                              address >> 8,               //	SECTOR_COUNT,
                              address & 0xff,               //	LBA_LOW,
                              0,               //	LBA_MID,
                              0,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xFD,               //	COMMAND
                              0x00,                // CONTROL
                              kSCSIDataTransfer_FromTargetToInitiator, length)
        == true)
    {
        SetTimeoutDuration ( request, 0 );
        SetDataTransferDirection ( request, kSCSIDataTransfer_FromTargetToInitiator);
        SetRequestedDataTransferCount ( request, length );
        if (length > 0 && buffer) {
            SetDataBuffer ( request, buffer );
        }
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        (GetTaskStatus ( request ) == kSCSITaskStatus_GOOD) )
    {
        DEBUG_LOG("%s[%p]::%s register %04x value %02x\n", getClassName(), this,  __FUNCTION__, (int)address, (int)*ptr );
        result = true;
    }
    else
    {
        GetAutoSenseData( request, &senseData, sizeof(senseData) );
        if (((senseData.VALID_RESPONSE_CODE == 0x70) || (senseData.VALID_RESPONSE_CODE == 0x72)) && (senseData.SENSE_KEY == 5)) {
            // Illegal Request - The disk (or for example a flash memory) does not support the command
            DEBUG_LOG("%s::%s failed %d %d illegal request\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        } else if (senseData.VALID_RESPONSE_CODE == 0x70 && senseData.ADDITIONAL_SENSE_CODE == 0x3A) {
            DEBUG_LOG("%s::%s failed %d %d no media\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
            *ptr = 0;
            result = true; // use this command to probe JMicron
        } else {
            ERROR_LOG("%s::%s failed %d %d\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
            LogAutoSenseData(request);
        }
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
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}


bool
fi_dungeon_driver_IOSATDriver::IdentifyDevice ( void )
{
    bool result = false;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    OSString *name = OSDynamicCast(OSString, getProperty(kEnclosureName));
    
    fSATSMARTCapable = true;
    //SendBuiltInINQUIRY ( );
    boolean_t autodetect = (fPassThroughMode  == kPassThroughModeAuto);
    if (autodetect) {
        UInt8 status;
        // JMicron needs the port value. Unfortunately this is not reliable
        IOService *fProvider = getProvider();
        if (fProvider) {
            OSNumber *lun = OSDynamicCast ( OSNumber, fProvider->getProperty("IOUnitLUN"));
            if (lun) {
                DEBUG_LOG("%s[%p]::%s LUN %d\n", getClassName(), this, __FUNCTION__, lun->unsigned32BitValue());
                fPort = lun->unsigned32BitValue() & 1;
            }
        }

        ERROR_LOG("%s[%p]::%s Trying with PassThrough12\n", getClassName(), this,  __FUNCTION__);
        fPassThroughMode = kPassThroughModeSAT12;
        setProperty(kPassThroughMode, "sat12");
        fSATSMARTCapable = Send_ATA_IDENTIFY();
        if (!fSATSMARTCapable) {
            ERROR_LOG("%s[%p]::%s SAT PassThrough12 failed, retrying with PassThrough16\n", getClassName(), this,  __FUNCTION__);
            fSATSMARTCapable = true;
            fPassThroughMode = kPassThroughModeSAT16;
            setProperty(kPassThroughMode, "sat16");
            fSATSMARTCapable = Send_ATA_IDENTIFY();
        }
        if (!fSATSMARTCapable) {
            ERROR_LOG("%s[%p]::%s SAT PassThrough12 probe failed, trying with JMicron\n", getClassName(), this,  __FUNCTION__);
            fSATSMARTCapable = true;
            fPassThroughMode = kPassThroughModeJMicron;
            setProperty(kPassThroughMode, "jmicron");
            
            if (JMicron_get_registers(0x720f, & status, sizeof status)) {
                DEBUG_LOG("%s[%p]::%s register value %02x USE JMICRON!\n", getClassName(), this, __FUNCTION__, (int) status);
                if (status & 0x40) {
                    // This does not work for me. Status is always 4 in my enclosure
                    fPort = 1;
                }
                fSATSMARTCapable = Send_ATA_IDENTIFY();
            }
        }
        if (!fSATSMARTCapable) {
            setProperty(kPassThroughMode, "none");
        }
    }
    if (!autodetect) {
        fSATSMARTCapable = Send_ATA_IDENTIFY();
    }
    
    if (fSATSMARTCapable) {
        //setProperty (kIOPropertyVendorNameKey, GetVendorString() );
        setProperty (kProductModelKey, model);
        setProperty (kIOPropertyProductNameKey, model);
        setProperty (kIOPropertyProductRevisionLevelKey, revision);
        setProperty (kIOPropertyProductSerialNumberKey, serial);
        
	fSATSMARTCapable = Send_ATA_SMART_READ_DATA();
    }
    
    setProperty(kSATSMARTCapableKey, fSATSMARTCapable);
    if (fSATSMARTCapable) {
        IOLog("SATSMARTDriver v%d.%d: enclosure '%s', disk serial '%s', revision '%s', model '%s'\n",
              (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
              name ? name->getCStringNoCopy() : "unknown",
              serial, revision, model);
              result = true;
    } else {
        IOLog("SATSMARTDriver v%d.%d: enclosure '%s', disk is not SAT capable\n",
              (int)SATSMARTDriverVersionNumber, ((int)(SATSMARTDriverVersionNumber * 100))%100,
              name ? name->getCStringNoCopy() : "unknown");
        result = false;
    }
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

void fi_dungeon_driver_IOSATDriver::LogAutoSenseData (SCSITaskIdentifier request) {
    SCSI_Sense_Data senseData;
    GetAutoSenseData( request, &senseData, sizeof(senseData) );
    ERROR_LOG( "senseData: VALID_RESPONSE_CODE=%d (7=valid),\n"
              ":          SEGMENT_NUMBER=%d,\n"
              ":          SENSE_KEY=%d (7 = FILEMARK, 6 = EOM, 5 = ILI, 3-0 = SENSE KEY)\n"
              ":          INFORMATION_1,_2,_3,_4=%d,%d,%d,%d,\n"
              ":          ADDITIONAL_SENSE_LENGTH=%d,\n"
              ":          COMMAND_SPECIFIC_INFORMATION_1,_2,_3,_4=%d,%d,%d,%d,\n"
              ":          ADDITIONAL_SENSE_CODE=%d,\n"
              ":          ADDITIONAL_SENSE_CODE_QUALIFIER=%d,\n"
              ":          FIELD_REPLACEABLE_UNIT_CODE=%d,\n"
              ":          SKSV_SENSE_KEY_SPECIFIC_MSB=%d (7 = Sense Key Specific Valid bit, 6-0 Sense Key Specific MSB),\n"
              ":          SENSE_KEY_SPECIFIC_MID=%d,\n"
              ":          SENSE_KEY_SPECIFIC_LSB=%d\n",
              senseData.VALID_RESPONSE_CODE, senseData.SEGMENT_NUMBER, senseData.SENSE_KEY,
              senseData.INFORMATION_1, senseData.INFORMATION_2,
              senseData.INFORMATION_3, senseData.INFORMATION_4, senseData.ADDITIONAL_SENSE_LENGTH,
              senseData.COMMAND_SPECIFIC_INFORMATION_1, senseData.COMMAND_SPECIFIC_INFORMATION_2,
              senseData.COMMAND_SPECIFIC_INFORMATION_3, senseData.COMMAND_SPECIFIC_INFORMATION_4,
              senseData.ADDITIONAL_SENSE_CODE, senseData.ADDITIONAL_SENSE_CODE_QUALIFIER,
              senseData.FIELD_REPLACEABLE_UNIT_CODE, senseData.SKSV_SENSE_KEY_SPECIFIC_MSB,
              senseData.SENSE_KEY_SPECIFIC_MID, senseData.SENSE_KEY_SPECIFIC_LSB
              );
}

bool
fi_dungeon_driver_IOSATDriver::InitializeDeviceSupport ( void )
{
    bool result;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    result = super::InitializeDeviceSupport ( );
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

void fi_dungeon_driver_IOSATDriver::TerminateDeviceSupport ( void ) {
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    super::TerminateDeviceSupport();
}


char *
fi_dungeon_driver_IOSATDriver::GetVendorString ( void ) {
    //serial[sizeof(serial)-1]=0;
    //if (*serial) return serial;
    if (!fIdentified && fSATSMARTCapable) {
        // Query device identification and check SAT capability
        IdentifyDevice();
        fIdentified = true;
        // TODO check. This may identify the drive too early
    }
    return super::GetVendorString();
}

char *
fi_dungeon_driver_IOSATDriver::GetProductString ( void ) {
    if (!fIdentified && fSATSMARTCapable) {
        // Query device identification and check SAT capability
        IdentifyDevice();
        fIdentified = true;
        // TODO check. This may identify the drive too early
    }
    model[sizeof(model)-1]=0;
    if (*model) return model;
    return super::GetProductString();
}

char *
fi_dungeon_driver_IOSATDriver::GetRevisionString ( void ) {
    if (!fIdentified && fSATSMARTCapable) {
        // Query device identification and check SAT capability
        IdentifyDevice();
        fIdentified = true;
        // TODO check. This may identify the drive too early
    }
    revision[sizeof(revision)-1] =0;
    if (*revision) return revision;
    return super::GetRevisionString();
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_IDENTIFY ( void )
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier request            = NULL;
    SCSI_Sense_Data senseData;
    UInt8 *                            ptr                = NULL;
    UInt16 * ataIdentify = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
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
    if ( PASS_THROUGH_12or16 ( request,
                              buffer,
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolPIODataIn,               //     PROTOCOL,
                              0,               //     EXTEND,
                              kIOSATOffline0s,               //     OFF_LINE,
                              0,               //     CK_COND,
                              kIOSATTDirectionFromDevice,               //     T_DIR,
                              kIOSATTLengthBlocks,               //     BYT_BLOK,
                              kIOSATTLengthInSectorCount,               //     T_LENGTH,
                              0,               //	FEATURES,
                              1,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0,               //	LBA_MID,
                              0,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xEC,               //	COMMAND, identify
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        (GetTaskStatus ( request ) == kSCSITaskStatus_GOOD) )
    {
        DEBUG_LOG("%s[%p]::%s checksum %d\n", getClassName(), this,  __FUNCTION__, checksum(buffer) );
        swapbytes(buffer);
        ataIdentify = ( UInt16 * ) buffer->getBytesNoCopy ( );
        
        trimcpy(serial, (char*)(ataIdentify+kATAIdentifySerialNumber), sizeof(serial)-1);
        trimcpy(revision, (char*)(ataIdentify+kATAIdentifyFirmwareRevision), sizeof(revision)-1);
        trimcpy(model, (char*)(ataIdentify+kATAIdentifyModelNumber), sizeof(model)-1);
        
        if ((*revision == 0 && *model == 0) || (countzeros(buffer) == buffer->getLength ( ))) {
	    // Some disk enclosures return success, but data is no good
            ERROR_LOG("%s::%s zero response\n", getClassName(), __FUNCTION__);
            //hexdump16(ataIdentify, 0, buffer->getLength ( ));
        } else {
	    fCapabilities = ataIdentify[kATAIdentifyDriveCapabilities]&0xffff;
	    result = true;
	}
    }
    else
    {
        GetAutoSenseData( request, &senseData, sizeof(senseData) );
        if (((senseData.VALID_RESPONSE_CODE == 0x70) || (senseData.VALID_RESPONSE_CODE == 0x72)) && (senseData.SENSE_KEY == 5)) {
            // Illegal Request - The disk (or for example a flash memory) does not support the command
            DEBUG_LOG("%s::%s failed %d %d illegal request\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        } else if (senseData.VALID_RESPONSE_CODE == 0x70 && senseData.ADDITIONAL_SENSE_CODE == 0x3A) {
            DEBUG_LOG("%s::%s failed %d %d no media\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        } else {
            ERROR_LOG("%s::%s failed %d %d\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
            LogAutoSenseData(request);
        }
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
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}


bool
fi_dungeon_driver_IOSATDriver::Send_ATA_SMART_READ_DATA ( void )
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *        buffer            = NULL;
    SCSITaskIdentifier request            = NULL;
    SCSI_Sense_Data senseData;
    UInt8 *                            ptr                = NULL;
    UInt16 * ptr16;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    buffer = IOBufferMemoryDescriptor::withCapacity ( 512, kIODirectionIn, false );
    
    require ( ( buffer != NULL ), ErrorExit );
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
    ptr16 = ( UInt16 * ) buffer->getBytesNoCopy ( );
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
    
    if ( PASS_THROUGH_12or16 ( request,
                              buffer,
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolPIODataIn,               //     PROTOCOL,
                              0,               //     EXTEND,
                              kIOSATOffline0s,               //     OFF_LINE,
                              0,               //     CK_COND,
                              kIOSATTDirectionFromDevice,               //     T_DIR,
                              kIOSATTLengthBlocks,               //     BYT_BLOK,
                              kIOSATTLengthInSectorCount,               //     T_LENGTH,
                              0xd0,               //	FEATURES, read data
                              1,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x4f,               //	LBA_MID,
                              0xc2,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xB0,               //	COMMAND, smart
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    // 20120521 RJVB: it appears we should accept a kSCSITaskStatus_CHECK_CONDITION status too:
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        (GetTaskStatus ( request ) == kSCSITaskStatus_GOOD ||
         (fPermissive && (GetTaskStatus(request) == kSCSITaskStatus_CHECK_CONDITION))) )
    {
        DEBUG_LOG("%s[%p]::%s success checksum %d\n",
                  getClassName(), this,  __FUNCTION__, checksum(buffer));
        swapbytes(buffer);
        hexdump16(ptr16, 368, 10);
	result = true;
    }
    else
    {
        GetAutoSenseData( request, &senseData, sizeof(senseData) );
        if (((senseData.VALID_RESPONSE_CODE == 0x70) || (senseData.VALID_RESPONSE_CODE == 0x72)) && (senseData.SENSE_KEY == 5)) {
            // Illegal Request - The disk (or for example a flash memory) does not support the command
            DEBUG_LOG("%s::%s failed %d %d illegal request\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        } else if (senseData.VALID_RESPONSE_CODE == 0x70 && senseData.ADDITIONAL_SENSE_CODE == 0x3A) {
            DEBUG_LOG("%s::%s failed %d %d no media\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        } else {
            ERROR_LOG("%s::%s failed %d %d\n",
                      getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
            LogAutoSenseData  (request);
        }
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
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}


bool
fi_dungeon_driver_IOSATDriver::Send_ATA_IDLE(UInt8 value)
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s value %d\n", getClassName(), this, __FUNCTION__, (int) value);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolNonData,     //     PROTOCOL, soft reset
                              0,               //     EXTEND,
                              kIOSATOffline0s, //     OFF_LINE,
                              0,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,   //     T_LENGTH,
                              0x00,               //	FEATURES
                              value,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xe3,               //	COMMAND, idle
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d\n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
	result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_IDLE_IMMEDIATE()
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolNonData,     //     PROTOCOL
                              0,               //     EXTEND,
                              kIOSATOffline2s, //     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
                              0,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,   //     T_LENGTH,
                              0x00,               //	FEATURES
                              0x00,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xe1,               //	COMMAND, idle immediate
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d\n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
	result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_STANDBY(UInt8 value)
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s value %d\n", getClassName(), this, __FUNCTION__, (int) value);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolNonData,     //     PROTOCOL
                              0,               //     EXTEND,
                              kIOSATOffline2s, //     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
                              0,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,   //     T_LENGTH,
                              0x00,               //	FEATURES
                              value,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xe2,               //	COMMAND, standby
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d\n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
	result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_STANDBY_IMMEDIATE()
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolNonData,     //     PROTOCOL
                              0,               //     EXTEND,
                              kIOSATOffline0s, //     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
                              0,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,   //     T_LENGTH,
                              0x00,               //	FEATURES
                              0x00,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xe0,               //	COMMAND, standby immediate
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d\n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
	result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_CHECK_POWER_MODE(int *mode)
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolNonData,     //     PROTOCOL
                              0,               //     EXTEND,
                              kIOSATOffline0s, //     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
                              1,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,   //     T_LENGTH,
                              0x00,               //	FEATURES
                              0x00,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0xe5,               //	COMMAND, check power mode
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d \n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
        result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    LogAutoSenseData(request);
    if (mode) {
        *mode = 42;
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::Send_ATA_SEND_SOFT_RESET ( void )
{
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    SCSITaskIdentifier request            = NULL;
    bool result = false;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    
    if ( PASS_THROUGH_12or16 ( request,
                              0,               // buffer
                              0,               //     MULTIPLE_COUNT,
                              kIOSATProtocolSRST,               //     PROTOCOL, soft reset
                              0,               //     EXTEND,
                              kIOSATOffline6s,               //     OFF_LINE, 1 == 2s,  2 == 6s, 3 == 14s
                              0,               //     CK_COND,
                              0,               //     T_DIR,
                              0,               //     BYT_BLOK,
                              kIOSATTLengthNoData,               //     T_LENGTH,
                              0x00,               //	FEATURES
                              0,               //	SECTOR_COUNT,
                              0,               //	LBA_LOW,
                              0x00,               //	LBA_MID,
                              0x00,               //	LBA_HIGH,
                              0,               //	DEVICE,
                              0x00,               //	COMMAND
                              0x00)               // CONTROL
        == true)
    {
        serviceResponse = SendCommand ( request, kTenSecondTimeoutInMS );
    }
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        DEBUG_LOG("%s[%p]::%s success %d %d\n",
                  getClassName(), this,  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
	result = true;
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n",
                  getClassName(),  __FUNCTION__, serviceResponse,GetTaskStatus ( request ));
    }
    
ReleaseTask:
    require_quiet ( ( request != NULL ), ReleaseBuffer );
    ReleaseSCSITask ( request );
    request = NULL;
    
ReleaseBuffer:
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, (int)result);
    return result;
}

//////////////

bool
fi_dungeon_driver_IOSATDriver::PASS_THROUGH_JMicron (
                                                      SCSITaskIdentifier request,
                                                      IOMemoryDescriptor *    dataBuffer,
                                                      SCSICmdField4Bit PROTOCOL,
                                                      SCSICmdField1Bit T_DIR,
                                                      SCSICmdField1Byte FEATURES,
                                                      SCSICmdField1Byte SECTOR_COUNT,
                                                      SCSICmdField1Byte LBA_LOW,
                                                      SCSICmdField1Byte LBA_MID,
                                                      SCSICmdField1Byte LBA_HIGH,
                                                      SCSICmdField1Byte DEVICE,
                                                      SCSICmdField1Byte COMMAND,
                                                      SCSICmdField1Byte CONTROL,
                                                      int direction, int transferCount)
{
    bool result = false;
    int dxfer_len;
    dxfer_len = 1; // FIXME in SMART_READ
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
    // Validate the parameters here.
    require ( ( request != NULL ), ErrorExit );
    require ( ResetForNewTask ( request ), ErrorExit );
    
    // The helper functions ensure that the parameters fit within the
    // CDB fields and that the buffer passed in is large enough for
    // the transfer length.
    require ( IsParameterValid ( PROTOCOL, kSCSICmdFieldMask4Bit ), ErrorExit );
    require ( IsParameterValid ( T_DIR, kSCSICmdFieldMask1Bit ), ErrorExit );
    require ( IsParameterValid ( FEATURES, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( SECTOR_COUNT, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( LBA_LOW, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( LBA_MID, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( LBA_HIGH, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( DEVICE, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( COMMAND, kSCSICmdFieldMask1Byte ), ErrorExit );
    require ( IsParameterValid ( CONTROL, kSCSICmdFieldMask1Byte ), ErrorExit );
    
    dxfer_len = transferCount;
    
    SetCommandDescriptorBlock ( request,
                               kSCSICmd_PASS_THROUGH_JMicron,
                               (direction == kSCSIDataTransfer_FromInitiatorToTarget) ? 0 : 0x10,
                               0,
                               dxfer_len >> 8,
                               dxfer_len & 0xff,
                               (FEATURES & 0xff),
                               (SECTOR_COUNT & 0xff),
                               (LBA_LOW & 0xff),
                               (LBA_MID & 0xff),
                               (LBA_HIGH & 0xff),
                               fDevice | (fPort ? 0xa0 : 0xb0),
                               COMMAND);
    
    result = true;
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::PASS_THROUGH_12 (
                                                 SCSITaskIdentifier request,
                                                 IOMemoryDescriptor *    dataBuffer,
                                                 SCSICmdField3Bit MULTIPLE_COUNT,
                                                 SCSICmdField4Bit PROTOCOL,
                                                 SCSICmdField1Bit EXTEND,
                                                 SCSICmdField2Bit OFF_LINE,
                                                 SCSICmdField1Bit CK_COND,
                                                 SCSICmdField1Bit T_DIR,
                                                 SCSICmdField1Bit BYT_BLOK,
                                                 SCSICmdField2Bit T_LENGTH,
                                                 SCSICmdField1Byte FEATURES,
                                                 SCSICmdField1Byte SECTOR_COUNT,
                                                 SCSICmdField1Byte LBA_LOW,
                                                 SCSICmdField1Byte LBA_MID,
                                                 SCSICmdField1Byte LBA_HIGH,
                                                 SCSICmdField1Byte DEVICE,
                                                 SCSICmdField1Byte COMMAND,
                                                 SCSICmdField1Byte CONTROL)
{
    bool result = false;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
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
    
    // This is a 12-byte command: fill out the CDB appropriately
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
    
    result = true;
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::PASS_THROUGH_16 (
                                                 SCSITaskIdentifier request,
                                                 IOMemoryDescriptor *    dataBuffer,
                                                 SCSICmdField3Bit MULTIPLE_COUNT,
                                                 SCSICmdField4Bit PROTOCOL,
                                                 SCSICmdField1Bit EXTEND,
                                                 SCSICmdField2Bit OFF_LINE,
                                                 SCSICmdField1Bit CK_COND,
                                                 SCSICmdField1Bit T_DIR,
                                                 SCSICmdField1Bit BYT_BLOK,
                                                 SCSICmdField2Bit T_LENGTH,
                                                 SCSICmdField2Byte FEATURES,
                                                 SCSICmdField2Byte SECTOR_COUNT,
                                                 SCSICmdField2Byte LBA_LOW,
                                                 SCSICmdField2Byte LBA_MID,
                                                 SCSICmdField2Byte LBA_HIGH,
                                                 SCSICmdField1Byte DEVICE,
                                                 SCSICmdField1Byte COMMAND,
                                                 SCSICmdField1Byte CONTROL)
{
    // TODO the EXTEND is always set to zero, should it be 1?
    EXTEND = 1;
    
    bool result = false;
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    
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
    
    
    result = true;
    
ErrorExit:
    DEBUG_LOG("%s[%p]::%s result %d\n", getClassName(), this,  __FUNCTION__, result);
    return result;
}

bool
fi_dungeon_driver_IOSATDriver::PASS_THROUGH_12or16 (
                                                     SCSITaskIdentifier request,
                                                     IOMemoryDescriptor *    dataBuffer,
                                                     SCSICmdField3Bit MULTIPLE_COUNT,
                                                     SCSICmdField4Bit PROTOCOL,
                                                     SCSICmdField1Bit EXTEND,
                                                     SCSICmdField2Bit OFF_LINE,
                                                     SCSICmdField1Bit CK_COND,
                                                     SCSICmdField1Bit T_DIR,
                                                     SCSICmdField1Bit BYT_BLOK,
                                                     SCSICmdField2Bit T_LENGTH,
                                                     SCSICmdField2Byte FEATURES,
                                                     SCSICmdField2Byte SECTOR_COUNT,
                                                     SCSICmdField2Byte LBA_LOW,
                                                     SCSICmdField2Byte LBA_MID,
                                                     SCSICmdField2Byte LBA_HIGH,
                                                     SCSICmdField1Byte DEVICE,
                                                     SCSICmdField1Byte COMMAND,
                                                     SCSICmdField1Byte CONTROL)
{
    bool result = false;
    int direction = kSCSIDataTransfer_NoDataTransfer;
    int transferCount = 0;
    
    require ( ( request != NULL ), ErrorExit );
    
    switch (PROTOCOL) {
        case kIOSATProtocolHardReset:     // Hard Reset
        case kIOSATProtocolSRST:     // SRST
        case kIOSATProtocolDEVICERESET:     // DEVICE RESET
        case kIOSATProtocolNonData:     // Non-data
            require (!dataBuffer, ErrorExit);
            break;
        case kIOSATProtocolPIODataIn:     // PIO Data-In
        case kIOSATProtocolUDMADataIn:     // UDMA Data In
            require (T_DIR == kIOSATTDirectionFromDevice, ErrorExit);
            require (dataBuffer, ErrorExit);
            break;
        case kIOSATProtocolPIODataOut:     // PIO Data-Out
        case kIOSATProtocolUDMADataOut:     // UDMA Data Out
            require (T_DIR == kIOSATTDirectionToDevice, ErrorExit);
            require (dataBuffer, ErrorExit);
            break;
        case kIOSATProtocolDMA:     // DMA
        case kIOSATProtocolDMAQueued:     // DMA Queued
        case kIOSATProtocolDeviceDiagnostic:     // Device Diagnostic
        case kIOSATProtocolFPDMA:     //FPDMA
        default:
            break;
    }
    if (dataBuffer) {
        require ( IsMemoryDescriptorValid ( dataBuffer, dataBuffer->getLength() ), ErrorExit );
        transferCount = (int) dataBuffer->getLength();
        if (T_DIR == kIOSATTDirectionFromDevice) {
            direction = kSCSIDataTransfer_FromTargetToInitiator;
        } else {
            direction = kSCSIDataTransfer_FromInitiatorToTarget;
        }
    } else {
        direction = kSCSIDataTransfer_NoDataTransfer;
        transferCount = 0;
    }
    
    if (fPassThroughMode == kPassThroughModeJMicron) {
        result = PASS_THROUGH_JMicron(request, dataBuffer,
                                      PROTOCOL, T_DIR, 
                                      FEATURES, SECTOR_COUNT,
                                      LBA_LOW, LBA_MID, LBA_HIGH,
                                      DEVICE, COMMAND, CONTROL,
                                      direction, transferCount );
        
    } else if (fPassThroughMode == kPassThroughModeSAT16) {
        result = PASS_THROUGH_16( request, dataBuffer,
                                 MULTIPLE_COUNT, PROTOCOL, EXTEND, OFF_LINE, CK_COND, T_DIR, BYT_BLOK, T_LENGTH,
                                 FEATURES, SECTOR_COUNT,
                                 LBA_LOW, LBA_MID, LBA_HIGH,
                                 DEVICE, COMMAND, CONTROL);
    } else {
        result = PASS_THROUGH_12( request, dataBuffer,
                                 MULTIPLE_COUNT, PROTOCOL, EXTEND, OFF_LINE, CK_COND, T_DIR, BYT_BLOK, T_LENGTH,
                                 (SCSICmdField1Byte) FEATURES, (SCSICmdField1Byte) SECTOR_COUNT,
                                 (SCSICmdField1Byte) LBA_LOW, (SCSICmdField1Byte) LBA_MID, (SCSICmdField1Byte) LBA_HIGH,
                                 DEVICE, COMMAND, CONTROL);
    }
    
    require (result, ErrorExit );
    
    SetTimeoutDuration ( request, 0 );
    SetDataTransferDirection ( request, direction);
    SetRequestedDataTransferCount ( request, transferCount );
    if (transferCount > 0) {
        SetDataBuffer ( request, dataBuffer );
    }
    
ErrorExit:
    return result;
}

void
fi_dungeon_driver_IOSATDriver::SendBuiltInINQUIRY ( void )
{
    // The Service Response represents the execution status of a service request.
    SCSIServiceResponse serviceResponse = kSCSIServiceResponse_SERVICE_DELIVERY_OR_TARGET_FAILURE;
    IOBufferMemoryDescriptor *         buffer  = NULL;
    SCSITaskIdentifier request = NULL;
    UInt8 *                            ptr     = NULL;
    SCSICmd_INQUIRY_StandardDataAll *   inquiryBuffer = NULL;
    
    DEBUG_LOG("%s[%p]::%s\n", getClassName(), this, __FUNCTION__);
    buffer = IOBufferMemoryDescriptor::withCapacity ( sizeof ( SCSICmd_INQUIRY_StandardDataAll ), kIODirectionIn, false );
    require ( ( buffer != NULL ), ErrorExit );
    
    ptr = ( UInt8 * ) buffer->getBytesNoCopy ( );
    bzero ( ptr, buffer->getLength ( ) );
    
    request = GetSCSITask ( );
    require ( ( request != NULL ), ReleaseBuffer );
    require ( ( buffer->prepare ( ) == kIOReturnSuccess ), ReleaseTask );
    
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
    
    if ( ( serviceResponse == kSCSIServiceResponse_TASK_COMPLETE ) &&
        GetTaskStatus ( request ) == kSCSITaskStatus_GOOD )
    {
        inquiryBuffer = (SCSICmd_INQUIRY_StandardDataAll *)(buffer->getBytesNoCopy());
        DEBUG_LOG("%s[%p]::%s success %d '%s' '%s'\n", getClassName(), this,  __FUNCTION__, inquiryBuffer->ADDITIONAL_LENGTH, inquiryBuffer->VENDOR_IDENTIFICATION, inquiryBuffer->PRODUCT_IDENTIFICATION );
    }
    else
    {
        ERROR_LOG("%s::%s failed %d %d\n", getClassName(), __FUNCTION__, serviceResponse,GetTaskStatus ( request ) );
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


// Padding for future binary compatibility.
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 0);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 1);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 2);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 3);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 4);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 5);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 6);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 7);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 8);
OSMetaClassDefineReservedUnused(fi_dungeon_driver_IOSATDriver, 9);

