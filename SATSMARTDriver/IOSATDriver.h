/*
 *  IOSATDriver.h
 *
 *  Created by Jarkko Sonninen on 3.2.2012.
 *  Copyright 2012 Jarkko Sonninen. All rights reserved.
 *
 */

#ifndef _EXTERNAL_SAT_DRIVER_H_
#define _EXTERNAL_SAT_DRIVER_H_

#if defined(KERNEL) && defined(__cplusplus)

#include <IOKit/scsi/IOSCSIPeripheralDeviceType00.h>

class IOSATCommand;
class IOBufferMemoryDescriptor;

#define kSATSMARTCapableKey  "SATSMARTCapable"
#define kPermissiveKey  "Permissive"
#define kPassThroughMode  "PassThroughMode"
#define kMyPropertyKey  "MyProperty"
#define kProductModelKey "Model"
#define kEnclosureName "Enclosure Name"

enum {
    kPassThroughModeNone = 0,
    kPassThroughModeAuto = 1,
    kPassThroughModeSAT16 = 2,
    kPassThroughModeSAT12 = 3,
    kPassThroughModeJMicron = 4,
};

enum {
    kIOSATTDirectionToDevice = 0,
    kIOSATTDirectionFromDevice = 1,
};

enum {
    kIOSATTLengthBytes = 0,
    kIOSATTLengthBlocks = 1
};

enum {
    kIOSATOffline0s = 0,
    kIOSATOffline2s = 1,
    kIOSATOffline6s = 2,
    kIOSATOffline14s = 3
};

enum {
    kIOSATProtocolHardReset = 0,
    kIOSATProtocolSRST = 1,
    kIOSATProtocolNonData = 3,
    kIOSATProtocolPIODataIn = 4,
    kIOSATProtocolPIODataOut = 5,
    kIOSATProtocolDMA = 6,
    kIOSATProtocolDMAQueued = 7,
    kIOSATProtocolDeviceDiagnostic = 8,
    kIOSATProtocolDEVICERESET = 9,
    kIOSATProtocolUDMADataIn = 10,
    kIOSATProtocolUDMADataOut = 11,
    kIOSATProtocolFPDMA = 12,
    kIOSATProtocolReturnResponseInformation = 15
};

enum  {
    kIOSATTLengthNoData = 0,
    kIOSATTLengthInFeatures = 1,
    kIOSATTLengthInSectorCount = 2,
    kIOSATTLengthInSTPSIU = 3
};

class org_dungeon_driver_IOSATDriver : public IOSCSIPeripheralDeviceType00
{
OSDeclareDefaultStructors(org_dungeon_driver_IOSATDriver)

char serial[21];
char revision[9];
char model[41];

bool fSATSMARTCapable;
int fPassThroughMode;
    int fPort;
    int fDevice;
bool fPermissive;
int capabilities;
protected:
bool    InitializeDeviceSupport ( void );
void	TerminateDeviceSupport ( void );
bool    Send_ATA_IDENTIFY ( void );
bool    Send_ATA_SMART_READ_DATA(void);
bool    Send_ATA_IDLE(UInt8 value);
bool    Send_ATA_IDLE_IMMEDIATE(void);
bool    Send_ATA_STANDBY(UInt8 value);
bool    Send_ATA_STANDBY_IMMEDIATE(void);
bool    Send_ATA_CHECK_POWER_MODE(int *);
bool    Send_ATA_SEND_SOFT_RESET(void);
    OSObject *getParentProperty(const char *key);

    bool JMicron_get_registers(UInt16 address, UInt8 *ptr, UInt16 length );
    bool
    PASS_THROUGH_JMicron (
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
                                                          SCSICmdField1Byte CONTROL, int direction, int transferCount);
    bool    PASS_THROUGH_12 (
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
    SCSICmdField1Byte CONTROL);
bool    PASS_THROUGH_16 (
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
    SCSICmdField1Byte CONTROL);
bool    PASS_THROUGH_12or16 (
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
    SCSICmdField1Byte CONTROL);
void    SendBuiltInINQUIRY ( void );

public:
virtual bool init(OSDictionary *dictionary = NULL);
virtual void free(void);
virtual IOService *probe(IOService *provider, SInt32 *score);
virtual bool start(IOService *provider);
virtual void stop(IOService *provider);

virtual char *          GetVendorString ( void );
virtual char *          GetProductString ( void );
virtual char *          GetRevisionString ( void );


virtual IOReturn setProperties(OSObject* properties);

virtual bool            attach ( IOService * provider );
virtual void            detach ( IOService * provider );

virtual void            CreateStorageServiceNub ( void );
virtual IOReturn    sendSMARTCommand ( IOSATCommand * command );
virtual bool IdentifyDevice ( void );
virtual void LogAutoSenseData (SCSITaskIdentifier request);
    
protected:

// Reserve space for future expansion.
struct IOSATDriverExpansionData { };
IOSATDriverExpansionData * fIIOSATDriverReserved;

private:

// Padding for future binary compatibility.
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 0);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 1);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 2);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 3);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 4);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 5);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 6);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 7);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 8);
OSMetaClassDeclareReservedUnused(org_dungeon_driver_IOSATDriver, 9);
};

#endif

#endif
