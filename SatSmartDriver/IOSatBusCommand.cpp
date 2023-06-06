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
 *	IOSatBusCommand.cpp
 *
 */

#include <IOKit/IOTypes.h>
#ifndef APPLE_KEXT_DEPRECATED
#define APPLE_KEXT_DEPRECATED __attribute__((deprecated))
#endif
#include "IOSyncer.h"
//#include "IOATATypes.h"
#include "IOSatBusCommand.h"
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

#define super IOSatCommand

OSDefineMetaClassAndStructors(IOSatBusCommand, IOSatCommand);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 0);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 1);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 2);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 3);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 4);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 5);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 6);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 7);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 8);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 9);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 10);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 11);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 12);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 13);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 14);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 15);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 16);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 17);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 18);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 19);
OSMetaClassDefineReservedUnused(IOSatBusCommand, 20);

/*-----------------------------------------------------------------------------
 *  Static allocator.
 *
 *-----------------------------------------------------------------------------*/
IOSatBusCommand*
IOSatBusCommand::allocateCmd(void)
{
    IOSatBusCommand* cmd = new IOSatBusCommand;

    if (cmd == NULL)
        return NULL;

    if (!cmd->init()) {
        cmd->free();
        return NULL;
    }

    return cmd;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSatBusCommand::init()
{
    if (!super::init())
        return false;

    zeroCommand();

    return true;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatBusCommand::zeroCommand(void)
{
    queue_init(&queueChain);
    state  = 0;
    syncer = NULL;

    super::zeroCommand();
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the command opcode
ataOpcode
IOSatBusCommand::getOpcode(void)
{
    return _opCode;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// get the command flags
ataFlags
IOSatBusCommand::getFlags(void)
{
    return (ataFlags) _flags;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
ataRegMask
IOSatBusCommand::getRegMask(void)
{
    return _regMask;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the unit id (0 master, 1 slave)
ataUnitID
IOSatBusCommand::getUnit(void)
{
    return _unit;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the timeout value for this command
UInt32
IOSatBusCommand::getTimeoutMS(void)
{
    return _timeoutMS;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// return the callback pointer
IOSatCompletionFunction*
IOSatBusCommand::getCallbackPtr(void)
{
    return _callback;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
// call the completion callback function
void
IOSatBusCommand::executeCallback(void)
{
    _inUse = false;

    if (_callback != NULL) {
        (*_callback)(this);

    } else if (syncer != NULL) {
        syncer->signal();
        syncer = NULL;
    }
}

/*-----------------------------------------------------------------------------
 * get the number of bytes between intervening interrupts for this transfer.
 *
 *-----------------------------------------------------------------------------*/
IOByteCount
IOSatBusCommand::getTransferChunkSize(void)
{
    return _logicalChunkSize;
}

ataTaskFile*
IOSatBusCommand::getTaskFilePtr(void)
{
    return &(_taskFile.taskFile);
}

UInt16
IOSatBusCommand::getPacketSize(void)
{
    return _packet.atapiPacketSize;
}

UInt16*
IOSatBusCommand::getPacketData(void)
{
    return _packet.atapiCommandByte;
}

IOByteCount
IOSatBusCommand::getByteCount(void)
{
    return _byteCount;
}

IOByteCount
IOSatBusCommand::getPosition(void)
{
    return _position;
}

IOMemoryDescriptor*
IOSatBusCommand::getBuffer(void)
{
    return _desc;
}

void
IOSatBusCommand::setActualTransfer(IOByteCount bytesTransferred)
{
    _actualByteCount = bytesTransferred;
}

void
IOSatBusCommand::setResult(IOReturn inResult)
{
    _result = inResult;
}

void
IOSatBusCommand::setCommandInUse(bool inUse /* = true */)
{
    _inUse = inUse;
}

#pragma mark-- IOSatBusCommand64 --

#undef super

#define super IOSatBusCommand

OSDefineMetaClassAndStructors(IOSatBusCommand64, super);

/*-----------------------------------------------------------------------------
 *  Static allocator.
 *
 *-----------------------------------------------------------------------------*/
IOSatBusCommand64*
IOSatBusCommand64::allocateCmd32(void)
{
    IOSatBusCommand64* cmd = new IOSatBusCommand64;

    if (cmd == NULL)
        return NULL;

    if (!cmd->init()) {
        cmd->free();
        return NULL;
    }

    return cmd;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
bool
IOSatBusCommand64::init()
{
    if (!super::init())
        return false;

    zeroCommand();

    _dmaCmd = IODMACommand::withSpecification(IODMACommand::OutputHost32,
        32,
        0x10000,
        IODMACommand::kMapped,
        512 * 2048,
        2);

    if (!_dmaCmd) {
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/
void
IOSatBusCommand64::zeroCommand(void)
{
    if (_dmaCmd != NULL) {
        if (_dmaCmd->getMemoryDescriptor() != NULL) {
            _dmaCmd->clearMemoryDescriptor();
        }
    }

    super::zeroCommand();
}

/*-----------------------------------------------------------------------------
 *
 *
 *-----------------------------------------------------------------------------*/

void
IOSatBusCommand64::free()
{
    if (_dmaCmd != NULL) {
        _dmaCmd->clearMemoryDescriptor();
        _dmaCmd->release();
        _dmaCmd = NULL;
    }

    super::free();
}

void
IOSatBusCommand64::setBuffer(IOMemoryDescriptor* inDesc)
{
    super::setBuffer(inDesc);
}

void
IOSatBusCommand64::executeCallback(void)
{
    if (_dmaCmd != NULL
        && _desc != NULL
        && (_flags & mATAFlagUseDMA)) {
        _dmaCmd->complete();
    }

    if (_dmaCmd != NULL) {
        _dmaCmd->clearMemoryDescriptor();
    }

    super::executeCallback();
}

IODMACommand*
IOSatBusCommand64::GetDMACommand(void)
{
    return _dmaCmd;
}

void
IOSatBusCommand64::setCommandInUse(bool inUse /* = true */)
{
    if (inUse) {
        if (_dmaCmd != NULL
            && _desc != NULL
            && (_flags & mATAFlagUseDMA)) {
            _dmaCmd->setMemoryDescriptor(_desc, true);
        }
    }

    super::setCommandInUse(inUse);
}
