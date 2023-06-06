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
 *	IOSatCommand.cpp
 *
 */

#include <IOKit/IOLib.h>
#include <IOKit/IOTypes.h>

//#include"IOATATypes.h"
#include "IOSatCommand.h"
#include <IOKit/ata/IOATATypes.h>

#ifdef DLOG
#undef DLOG
#endif

#ifdef __GNUC__
#ifdef DEBUG
#define DLOG(fmt, args...) IOLog(fmt, ##args)
#else
#define DLOG(fmt, args...)
#endif
#else
#define DLOG(fmt, ...)
#endif
//---------------------------------------------------------------------------

#define super IOCommand

OSDefineMetaClass(IOSatCommand, IOCommand);
OSDefineAbstractStructors(IOSatCommand, IOCommand);

OSMetaClassDefineReservedUsed(IOSatCommand, 0); //setendResult()
OSMetaClassDefineReservedUsed(IOSatCommand, 1); // getExtendedLBAPtr()
OSMetaClassDefineReservedUnused(IOSatCommand, 2);
OSMetaClassDefineReservedUnused(IOSatCommand, 3);
OSMetaClassDefineReservedUnused(IOSatCommand, 4);
OSMetaClassDefineReservedUnused(IOSatCommand, 5);
OSMetaClassDefineReservedUnused(IOSatCommand, 6);
OSMetaClassDefineReservedUnused(IOSatCommand, 7);
OSMetaClassDefineReservedUnused(IOSatCommand, 8);
OSMetaClassDefineReservedUnused(IOSatCommand, 9);
OSMetaClassDefineReservedUnused(IOSatCommand, 10);
OSMetaClassDefineReservedUnused(IOSatCommand, 11);
OSMetaClassDefineReservedUnused(IOSatCommand, 12);
OSMetaClassDefineReservedUnused(IOSatCommand, 13);
OSMetaClassDefineReservedUnused(IOSatCommand, 14);
OSMetaClassDefineReservedUnused(IOSatCommand, 15);
OSMetaClassDefineReservedUnused(IOSatCommand, 16);
OSMetaClassDefineReservedUnused(IOSatCommand, 17);
OSMetaClassDefineReservedUnused(IOSatCommand, 18);
OSMetaClassDefineReservedUnused(IOSatCommand, 19);
OSMetaClassDefineReservedUnused(IOSatCommand, 20);

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSatCommand::init()
{
    fExpansionData         = (ExpansionData*) IOMalloc(sizeof(ExpansionData));
    fExpansionData->extLBA = IOSatExtendedLBA::createIOSatExtendedLBA(this);

    if (!super::init() || fExpansionData == NULL || fExpansionData->extLBA == NULL)
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
IOSatCommand::free()
{
    getExtendedLBA()->release();
    IOFree(fExpansionData, sizeof(ExpansionData));
    super::free();
}
/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatCommand::zeroCommand(void)
{
    _opCode           = kATANoOp;
    _flags            = 0;
    _unit             = kATAInvalidDeviceID;
    _timeoutMS        = 0;
    _desc             = NULL;
    _position         = (IOByteCount) 0;
    _byteCount        = (IOByteCount) 0;
    _regMask          = (ataRegMask) 0;
    _callback         = NULL;
    _result           = (IOReturn) 0;
    _actualByteCount  = (IOByteCount) 0;
    _status           = 0;
    _errReg           = 0;
    _logicalChunkSize = kATADefaultSectorSize;
    _inUse            = false;

    _taskFile.ataDataRegister        = 0x0000;
    _taskFile.ataAltSDevCReg         = 0x00;
    _taskFile.taskFile.ataTFFeatures = 0;
    _taskFile.taskFile.ataTFCount    = 0;
    _taskFile.taskFile.ataTFSector   = 0;
    _taskFile.taskFile.ataTFCylLo    = 0;
    _taskFile.taskFile.ataTFCylHigh  = 0;
    _taskFile.taskFile.ataTFSDH      = 0;
    _taskFile.taskFile.ataTFCommand  = 0;

    for (int i = 0; i < 16; i += 2) {
        _packet.atapiCommandByte[i] = 0x000;
    }

    _packet.atapiPacketSize = 0;

    getExtendedLBA()->zeroData();
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatCommand::setOpcode(ataOpcode inCode)
{
    _opCode = inCode;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setFlags(UInt32 inFlags)
{
    _flags = inFlags;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setUnit(ataUnitID inUnit)
{
    _unit = inUnit;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setTimeoutMS(UInt32 inMS)
{
    _timeoutMS = inMS;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setCallbackPtr(IOSatCompletionFunction* inCompletion)
{
    _callback = inCompletion;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatCommand::setRegMask(ataRegMask mask)
{
    _regMask = mask;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setBuffer(IOMemoryDescriptor* inDesc)
{
    _desc = inDesc;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setPosition(IOByteCount fromPosition)
{
    _position = fromPosition;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setByteCount(IOByteCount numBytes)
{
    _byteCount = numBytes;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setTransferChunkSize(IOByteCount numBytes)
{
    _logicalChunkSize = numBytes;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setFeatures(UInt8 in)
{
    _taskFile.taskFile.ataTFFeatures = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getErrorReg(void)
{
    return _taskFile.taskFile.ataTFFeatures;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setSectorCount(UInt8 in)
{
    _taskFile.taskFile.ataTFCount = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getSectorCount(void)
{
    return _taskFile.taskFile.ataTFCount;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setSectorNumber(UInt8 in)
{
    _taskFile.taskFile.ataTFSector = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getSectorNumber(void)
{
    return _taskFile.taskFile.ataTFSector;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setCylLo(UInt8 in)
{
    _taskFile.taskFile.ataTFCylLo = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getCylLo(void)
{
    return _taskFile.taskFile.ataTFCylLo;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatCommand::setCylHi(UInt8 in)
{
    _taskFile.taskFile.ataTFCylHigh = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getCylHi(void)
{
    return _taskFile.taskFile.ataTFCylHigh;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setDevice_Head(UInt8 in)
{
    _taskFile.taskFile.ataTFSDH = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getDevice_Head(void)
{
    return _taskFile.taskFile.ataTFSDH;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setCommand(UInt8 in)
{
    _taskFile.taskFile.ataTFCommand = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getStatus(void)
{
    return _taskFile.taskFile.ataTFCommand;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOReturn
IOSatCommand::setPacketCommand(UInt16 packetSizeBytes, UInt8* packetBytes)
{
    //	IOLog("ATACommand::setPacket size %d  bytePtr = %lx\n", packetSizeBytes, packetBytes);

    if ((packetSizeBytes > 16) || (packetBytes == 0L))
        return -1;

    UInt8* cmdBytes = (UInt8*) _packet.atapiCommandByte;

    for (int i = 0; i < packetSizeBytes; i++) {
        cmdBytes[i] = packetBytes[i];
    }

    _packet.atapiPacketSize = packetSizeBytes;

    return kATANoErr;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setDataReg(UInt16 in)
{
    _taskFile.ataDataRegister = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt16
IOSatCommand::getDataReg(void)
{
    return _taskFile.ataDataRegister;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatCommand::setControl(UInt8 in)
{
    _taskFile.ataAltSDevCReg = in;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getAltStatus(void)
{
    return _taskFile.ataAltSDevCReg;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOReturn
IOSatCommand::getResult(void)
{
    return _result;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOMemoryDescriptor*
IOSatCommand::getBuffer(void)
{
    return _desc;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOByteCount
IOSatCommand::getActualTransfer(void)
{
    return _actualByteCount;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getEndStatusReg(void)
{
    return _status;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

UInt8
IOSatCommand::getEndErrorReg(void)
{
    return _errReg;
}

/*-----------------------------------------------------------------------------
 * returns true if IOATAController is using the command.
 *
 *-----------------------------------------------------------------------------*/

bool
IOSatCommand::getCommandInUse(void)
{
    return _inUse;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

IOReturn
IOSatCommand::setLBA28(UInt32 lba, ataUnitID inUnit)
{
    // param check the inputs

    if ((lba & 0xF0000000) != 0x00000000
        || !(inUnit == kATADevice0DeviceID || inUnit == kATADevice1DeviceID)) {
        //param out of range
        return -1;
    }

    setSectorNumber((lba & 0xFF));                                                        //LBA 7:0
    setCylLo(((lba & 0xFF00) >> 8));                                                      // LBA 15:8
    setCylHi(((lba & 0x00FF0000) >> 16));                                                 // LBA 23:16
    setDevice_Head(((lba & 0x0F000000) >> 24) | mATALBASelect | (((UInt8) inUnit) << 4)); //LBA 27:24

    return kATANoErr;
}

void
IOSatCommand::setEndResult(UInt8 inStatus, UInt8 endError)
{
    _status = inStatus;
    _errReg = endError;
}

IOSatExtendedLBA*
IOSatCommand::getExtendedLBA(void)
{
    return fExpansionData->extLBA;
}

////////////////////////////////////////////////////////////////////////
#pragma mark IOSatExtendedLBA
#undef super

#define super OSObject
OSDefineMetaClassAndStructors(IOSatExtendedLBA, OSObject);

OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 0);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 1);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 2);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 3);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 4);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 5);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 6);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 7);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 8);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 9);
OSMetaClassDefineReservedUnused(IOSatExtendedLBA, 10);

IOSatExtendedLBA*
IOSatExtendedLBA::createIOSatExtendedLBA(IOSatCommand* inOwner)
{
    IOSatExtendedLBA* me = new IOSatExtendedLBA;
    if (me == NULL) {
        return NULL;
    }

    me->owner = inOwner;
    me->zeroData();

    return me;
}

void
IOSatExtendedLBA::setLBALow16(UInt16 inLBALow)
{
    lbaLow = inLBALow;
}

UInt16
IOSatExtendedLBA::getLBALow16(void)
{
    return lbaLow;
}

void
IOSatExtendedLBA::setLBAMid16(UInt16 inLBAMid)
{
    lbaMid = inLBAMid;
}

UInt16
IOSatExtendedLBA::getLBAMid16(void)
{
    return lbaMid;
}

void
IOSatExtendedLBA::setLBAHigh16(UInt16 inLBAHigh)
{
    lbaHigh = inLBAHigh;
}

UInt16
IOSatExtendedLBA::getLBAHigh16(void)
{
    return lbaHigh;
}

void
IOSatExtendedLBA::setSectorCount16(UInt16 inSectorCount)
{
    sectorCount = inSectorCount;
}

UInt16
IOSatExtendedLBA::getSectorCount16(void)
{
    return sectorCount;
}

void
IOSatExtendedLBA::setFeatures16(UInt16 inFeatures)
{
    features = inFeatures;
}

UInt16
IOSatExtendedLBA::getFeatures16(void)
{
    return features;
}

void
IOSatExtendedLBA::setDevice(UInt8 inDevice)
{
    device = inDevice;
}

UInt8
IOSatExtendedLBA::getDevice(void)
{
    return device;
}

void
IOSatExtendedLBA::setCommand(UInt8 inCommand)
{
    command = inCommand;
}

UInt8
IOSatExtendedLBA::getCommand(void)
{
    return command;
}

void
IOSatExtendedLBA::setExtendedLBA(UInt32 inLBAHi, UInt32 inLBALo, ataUnitID inUnit, UInt16 extendedCount, UInt8 extendedCommand)
{
    UInt8 lba7, lba15, lba23, lba31, lba39, lba47;
    lba7  = (inLBALo & 0xff);
    lba15 = (inLBALo & 0xff00) >> 8;
    lba23 = (inLBALo & 0xff0000) >> 16;
    lba31 = (inLBALo & 0xff000000) >> 24;
    lba39 = (inLBAHi & 0xff);
    lba47 = (inLBAHi & 0xff00) >> 8;

    setLBALow16(lba7 | (lba31 << 8));
    setLBAMid16(lba15 | (lba39 << 8));
    setLBAHigh16(lba23 | (lba47 << 8));

    setSectorCount16(extendedCount);
    setCommand(extendedCommand);
    setDevice(mATALBASelect | (((UInt8) inUnit) << 4)); // set the LBA bit and device select bits. The rest are reserved in extended addressing.
}

void
IOSatExtendedLBA::getExtendedLBA(UInt32* outLBAHi, UInt32* outLBALo)
{
    *outLBALo = (getLBALow16() & 0xFF) | ((getLBAMid16() & 0xff) << 8) | ((getLBAHigh16() & 0xff) << 16) | ((getLBALow16() & 0xff00) << 16);

    *outLBAHi = (getLBAHigh16() & 0xff00) | ((getLBAMid16() & 0xff00) >> 8);
}

void
IOSatExtendedLBA::zeroData(void)
{
    lbaLow = lbaMid = lbaHigh = sectorCount = features = device = command = 0;
}
