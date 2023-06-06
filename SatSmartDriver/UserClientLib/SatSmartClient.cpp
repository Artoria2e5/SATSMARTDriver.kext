/*
 * Modified by Jarkko Sonninen 2012
 */

/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
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

//—————————————————————————————————————————————————————————————————————————————
//	Includes
//—————————————————————————————————————————————————————————————————————————————

// Private includes
///Developer/SDKs/MacOSX10.6.sdk/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/
#include <IOKit/storage/ata/ATASMARTLib.h>
//#include "ATASMARTLib.h"
#include "SatSmartClient.h"
#include "SatSmartLibPriv.h"

#include <stdio.h>

//—————————————————————————————————————————————————————————————————————————————
//	Constants
//—————————————————————————————————————————————————————————————————————————————

enum
{
    kATASMARTLogDirectoryEntry = 0x00
};

enum
{
    kATADefaultSectorSize = 512
};

//—————————————————————————————————————————————————————————————————————————————
//	Macros
//—————————————————————————————————————————————————————————————————————————————

#ifdef DEBUG
#define SAT_SMART_DEBUGGING_LEVEL 1
#else
#define SAT_SMART_DEBUGGING_LEVEL 0
#endif

#if (SAT_SMART_DEBUGGING_LEVEL > 0)
#define PRINT(x) printf x
#else
#define PRINT(x)
#endif

//—————————————————————————————————————————————————————————————————————————————
//	Static variable initialization
//—————————————————————————————————————————————————————————————————————————————

SInt32 SatSmartClient::sFactoryRefCount = 0;

IOCFPlugInInterface
    SatSmartClient::sIOCFPlugInInterface
    = {
          0,
          &SatSmartClient::sQueryInterface,
          &SatSmartClient::sAddRef,
          &SatSmartClient::sRelease,
          1, 0, // version/revision
          &SatSmartClient::sProbe,
          &SatSmartClient::sStart,
          &SatSmartClient::sStop
      };

IOATASMARTInterface
    SatSmartClient::sATASMARTInterface
    = {
          0,
          &SatSmartClient::sQueryInterface,
          &SatSmartClient::sAddRef,
          &SatSmartClient::sRelease,
          1, 0, // version/revision
          &SatSmartClient::sSMARTEnableDisableOperations,
          &SatSmartClient::sSMARTEnableDisableAutosave,
          &SatSmartClient::sSMARTReturnStatus,
          &SatSmartClient::sSMARTExecuteOffLineImmediate,
          &SatSmartClient::sSMARTReadData,
          &SatSmartClient::sSMARTValidateReadData,
          &SatSmartClient::sSMARTReadDataThresholds,
          &SatSmartClient::sSMARTReadLogDirectory,
          &SatSmartClient::sSMARTReadLogAtAddress,
          &SatSmartClient::sSMARTWriteLogAtAddress,
          &SatSmartClient::sGetATAIdentifyData
      };

#if 0
#pragma mark -
#pragma mark Methods associated with ATASMARTLib factory
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• SatSmartLibFactory - Factory method. Exported via plist		[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

void*
SatSmartLibFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    PRINT(("SatSmartLibFactory called\n"));

    if (CFEqual(typeID, kIOATASMARTUserClientTypeID))
        return (void*) SatSmartClient::alloc();

    else
        return NULL;
}

//—————————————————————————————————————————————————————————————————————————————
//	• alloc - Used to allocate an instance of SatSmartClient		[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

IOCFPlugInInterface**
SatSmartClient::alloc(void)
{
    SatSmartClient*       userClient;
    IOCFPlugInInterface** interface = NULL;

    PRINT(("SatSmartClient::alloc called\n"));

    userClient = new SatSmartClient;
    if (userClient != NULL) {
        interface = (IOCFPlugInInterface**) &userClient->fCFPlugInInterfaceMap.pseudoVTable;
    }

    return interface;
}

//—————————————————————————————————————————————————————————————————————————————
//	• sFactoryAddRef -      Static method to increment the refcount associated with
//						the CFPlugIn factory
//																	[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

void
SatSmartClient::sFactoryAddRef(void)
{
    if (sFactoryRefCount++ == 0) {
        CFUUIDRef factoryID = kIOATASMARTLibFactoryID;

        CFRetain(factoryID);
        CFPlugInAddInstanceForFactory(factoryID);
    }
}

//—————————————————————————————————————————————————————————————————————————————
//	• sFactoryRelease - Static method to decrement the refcount associated with
//						the CFPlugIn factory and release it when the refcount
//						becomes zero.								[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

void
SatSmartClient::sFactoryRelease(void)
{
    if (sFactoryRefCount-- == 1) {
        CFUUIDRef factoryID = kIOATASMARTLibFactoryID;

        CFPlugInRemoveInstanceForFactory(factoryID);
        CFRelease(factoryID);

    }

    else if (sFactoryRefCount < 0) {
        sFactoryRefCount = 0;
    }
}

#if 0
#pragma mark -
#pragma mark Public Methods
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• Constructor. Called by subclasses.							[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

SatSmartClient::SatSmartClient(void)
    : fRefCount(1)
{
    fCFPlugInInterfaceMap.pseudoVTable = (IUnknownVTbl*) &sIOCFPlugInInterface;
    fCFPlugInInterfaceMap.obj          = this;

    fATASMARTInterfaceMap.pseudoVTable = (IUnknownVTbl*) &sATASMARTInterface;
    fATASMARTInterfaceMap.obj          = this;

    sFactoryAddRef();
}

//—————————————————————————————————————————————————————————————————————————————
//	• Destructor													[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

SatSmartClient::~SatSmartClient(void)
{
    sFactoryRelease();
}

#if 0
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• QueryInterface - Called to obtain the presence of an interface
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

HRESULT
SatSmartClient::QueryInterface(REFIID iid, void** ppv)
{
    CFUUIDRef uuid   = CFUUIDCreateFromUUIDBytes(NULL, iid);
    HRESULT   result = S_OK;

    PRINT(("SatSmartClient : QueryInterface called\n"));

    if (CFEqual(uuid, IUnknownUUID)) {
        PRINT(("IUnknownUUID requested\n"));

        *ppv = &fCFPlugInInterfaceMap;
        AddRef();

    }

    else if (CFEqual(uuid, kIOCFPlugInInterfaceID)) {
        PRINT(("kIOCFPlugInInterfaceID requested\n"));

        *ppv = &fCFPlugInInterfaceMap;
        AddRef();

    }

    else if (CFEqual(uuid, kIOATASMARTInterfaceID)) {
        PRINT(("kIOATASMARTInterfaceID requested\n"));

        *ppv = &fATASMARTInterfaceMap;
        AddRef();

    }

    else {
        PRINT(("unknown interface requested\n"));
        *ppv   = 0;
        result = E_NOINTERFACE;
    }

    CFRelease(uuid);

    return result;
}

//—————————————————————————————————————————————————————————————————————————————
//	• AddRef	-	Increments refcount associated with the object.	[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

UInt32
SatSmartClient::AddRef(void)
{
    fRefCount += 1;
    return fRefCount;
}

//—————————————————————————————————————————————————————————————————————————————
//	• Release	-	Decrements refcount associated with the object, freeing it
//					when the refcount is zero.						[PUBLIC]
//—————————————————————————————————————————————————————————————————————————————

UInt32
SatSmartClient::Release(void)
{
    UInt32 returnValue = fRefCount - 1;

    if (returnValue > 0) {
        fRefCount = returnValue;
    }

    else if (returnValue == 0) {
        fRefCount = returnValue;
        delete this;

    }

    else {
        returnValue = 0;
    }

    return returnValue;
}

//—————————————————————————————————————————————————————————————————————————————
//	• Probe -       Called by IOKit to ascertain whether we can drive the provided
//				io_service_t										[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::Probe(CFDictionaryRef propertyTable,
    io_service_t                      inService,
    SInt32*                           order)
{
    CFMutableDictionaryRef dict   = NULL;
    IOReturn               status = kIOReturnBadArgument;

    PRINT(("SatSmartClient::Probe called\n"));

    // Sanity check
    if (inService == 0) {
        goto Exit;
    }

    status = IORegistryEntryCreateCFProperties(inService, &dict, NULL, 0);
    if (status != kIOReturnSuccess) {
        goto Exit;
    }

    if (!CFDictionaryContainsKey(dict, CFSTR("IOCFPlugInTypes"))) {
        goto Exit;
    }

    status = kIOReturnSuccess;

Exit:

    if (dict != NULL) {
        CFRelease(dict);
        dict = NULL;
    }

    PRINT(("SatSmartClient::Probe called %x\n", status));
    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• Start - Called to start providing our services.				[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::Start(CFDictionaryRef propertyTable, io_service_t service)
{
    IOReturn status = kIOReturnSuccess;

    PRINT(("SatSmartClient : Start\n"));

    fService = service;
    status   = IOServiceOpen(fService,
        mach_task_self(),
        kIOATASMARTLibConnection,
        &fConnection);

    if (!fConnection)
        status = kIOReturnNoDevice;

    PRINT(("SatSmartClient : IOServiceOpen status = 0x%08lx, connection = %d\n",
        (long) status, fConnection));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• Stop - Called to stop providing our services.					[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::Stop(void)
{
    PRINT(("SatSmartClient : Stop\n"));

    if (fConnection) {
        PRINT(("SatSmartClient : IOServiceClose connection = %d\n", fConnection));
        IOServiceClose(fConnection);
        fConnection = MACH_PORT_NULL;
    }

    return kIOReturnSuccess;
}

#if 0
#pragma mark -
#pragma mark SMART Methods
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTEnableDisableOperations - Enables/Disables SMART operations
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTEnableDisableOperations(Boolean enable)
{
    IOReturn status    = kIOReturnSuccess;
    uint64_t selection = (enable) ? 1 : 0;

    PRINT(("SatSmartClient::SMARTEnableDisableOperations called\n"));

    status = IOConnectCallScalarMethod(fConnection,
        kIOATASMARTEnableDisableOperations,
        &selection, 1,
        0, 0);

    PRINT(("SatSmartClient::SMARTEnableDisableOperations status = %d\n", status));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTEnableDisableAutosave - Enables/Disables SMART AutoSave
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTEnableDisableAutosave(Boolean enable)
{
    IOReturn status    = kIOReturnSuccess;
    uint64_t selection = (enable) ? 1 : 0;

    PRINT(("SatSmartClient::SMARTEnableDisableAutosave called\n"));

    status = IOConnectCallScalarMethod(fConnection,
        kIOATASMARTEnableDisableAutoSave,
        &selection, 1,
        0, 0);

    PRINT(("SatSmartClient::SMARTEnableDisableAutosave status = %d\n", status));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTReturnStatus - Returns SMART status
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTReturnStatus(Boolean* exceededCondition)
{
    IOReturn status    = kIOReturnSuccess;
    uint64_t condition = 0;
    uint32_t outputCnt = 1;

    PRINT(("SatSmartClient::SMARTReturnStatus called\n"));

    status = IOConnectCallScalarMethod(fConnection,
        kIOATASMARTReturnStatus,
        0, 0,
        &condition, &outputCnt);

    if (status == kIOReturnSuccess) {
        *exceededCondition = (condition != 0);
        PRINT(("exceededCondition = %ld\n", (long) condition));
    }

    PRINT(("SatSmartClient::SMARTReturnStatus status = %d outputCnt = %d\n", status, (int) outputCnt));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTExecuteOffLineImmediate - Executes an off-line immediate SMART test
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTExecuteOffLineImmediate(Boolean extendedTest)
{
    IOReturn status    = kIOReturnSuccess;
    uint64_t selection = (extendedTest) ? 1 : 0;

    PRINT(("SatSmartClient::SMARTExecuteOffLineImmediate called\n"));

    status = IOConnectCallScalarMethod(fConnection,
        kIOATASMARTExecuteOffLineImmediate,
        &selection, 1,
        0, 0);

    PRINT(("SatSmartClient::SMARTExecuteOffLineImmediate status = %d\n", status));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTReadData - Reads the SMART data
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTReadData(ATASMARTData* data)
{
    IOReturn status;
    size_t   bytesTransferred = sizeof(ATASMARTData);

    PRINT(("SatSmartClient::SMARTReadData called\n"));

    status = IOConnectCallStructMethod(fConnection,
        kIOATASMARTReadData,
        (void*) 0, 0,
        data, &bytesTransferred);

    PRINT(("SatSmartClient::SMARTReadData status = %d\n", status));

#ifdef DEBUG
    if (status == kIOReturnSuccess) {
        UInt8* ptr = (UInt8*) data;

        printf("ATA SMART DATA\n");

        for (int index = 0; (index < sizeof(ATASMARTData)); index += 8) {
            printf("0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7]);
        }
    }
#endif

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTReadDataThresholds - Reads the SMART data thresholds
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTReadDataThresholds(ATASMARTDataThresholds* data)
{
    IOReturn status;
    size_t   bytesTransferred = sizeof(ATASMARTDataThresholds);

    PRINT(("SatSmartClient::SMARTReadDataThresholds called\n"));

    status = IOConnectCallStructMethod(fConnection,
        kIOATASMARTReadDataThresholds,
        (void*) 0, 0,
        data, &bytesTransferred);

    PRINT(("SatSmartClient::SMARTReadDataThresholds status = %d\n", status));

#ifdef DEBUG
    if (status == kIOReturnSuccess) {
        UInt8* ptr = (UInt8*) data;

        printf("ATA SMART DATA THRESHOLDS\n");

        for (int index = 0; (index < sizeof(ATASMARTDataThresholds)); index += 8) {
            printf("0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7]);
        }
    }
#endif

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTReadLogDirectory - Reads the SMART Log Directory
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTReadLogDirectory(ATASMARTLogDirectory* log)
{
    IOReturn status;

    status = SMARTReadLogAtAddress(kATASMARTLogDirectoryEntry,
        (void*) log,
        sizeof(ATASMARTLogDirectory));

    PRINT(("SatSmartClient::SMARTReadLogDirectory status = %d\n", status));

#ifdef DEBUG
    if (status == kIOReturnSuccess) {
        UInt8* ptr = (UInt8*) log;

        printf("ATA SMART Log Directory\n");

        for (int index = 0; (index < sizeof(ATASMARTLogDirectory)); index += 8) {
            printf("0x%02x 0x%02x 0x%02x 0x%02x | 0x%02x 0x%02x 0x%02x 0x%02x\n",
                ptr[index + 0], ptr[index + 1], ptr[index + 2], ptr[index + 3],
                ptr[index + 4], ptr[index + 5], ptr[index + 6], ptr[index + 7]);
        }
    }
#endif

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTReadLogAtAddress -       Reads from the SMART Log at specified address
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTReadLogAtAddress(UInt32 address,
    void*                                    buffer,
    UInt32                                   bufferSize)
{
    IOReturn              status;
    size_t                bytesTransferred = 0;
    ATASMARTReadLogStruct params;

    PRINT(("SatSmartClient::SMARTReadLogAtAddress called\n"));

    if ((address > 0xFF) || (buffer == NULL)) {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    params.numSectors = bufferSize / kATADefaultSectorSize;
    params.logAddress = address & 0xFF;
    bytesTransferred  = bufferSize;

    // Can't read or write more than 16 sectors
    if (params.numSectors > 16) {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    PRINT(("SatSmartClient::SMARTReadLogAtAddress address = %ld\n", (long) address));

    status = IOConnectCallStructMethod(fConnection,
        kIOATASMARTReadLogAtAddress,
        (void*) &params, sizeof(params),
        buffer, &bytesTransferred);

Exit:

    PRINT(("SatSmartClient::SMARTReadLogAtAddress status = %x\n", status));

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• SMARTWriteLogAtAddress - Writes to the SMART Log at specified address
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::SMARTWriteLogAtAddress(UInt32 address,
    const void*                               buffer,
    UInt32                                    bufferSize)
{
    IOReturn               status;
    ATASMARTWriteLogStruct params;

    PRINT(("SatSmartClient::SMARTWriteLogAtAddress called %d %p %d\n", (int) address, buffer, (int) bufferSize));

    if ((address > 0xFF) || (buffer == NULL) || (bufferSize > kSATMaxDataSize)) {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    params.numSectors  = bufferSize / kATADefaultSectorSize;
    params.logAddress  = address & 0xFF;
    params.data_length = bufferSize;

    // Can't read or write more than 16 sectors
    if (params.numSectors > 16) {
        status = kIOReturnBadArgument;
        goto Exit;
    }
    //memcpy (params.buffer, buffer, bufferSize);
    params.data_pointer = (uintptr_t) buffer;

    PRINT(("SatSmartClient::SMARTWriteLogAtAddress address = %ld\n", (long) address));

    status = IOConnectCallStructMethod(fConnection,
        kIOATASMARTWriteLogAtAddress,
        (void*) &params, sizeof(params),
        0, 0);

Exit:

    PRINT(("SatSmartClient::SMARTWriteLogAtAddress status = %d\n", status));

    return status;
}

#if 0
#pragma mark -
#pragma mark Additional Methods
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• GetATAIdentifyData - Gets ATA Identify Data.					[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::GetATAIdentifyData(void* buffer, UInt32 inSize, UInt32* outSize)
{
    IOReturn status           = kIOReturnBadArgument;
    size_t   bytesTransferred = 0;

    if ((buffer == NULL) || (inSize > kATADefaultSectorSize) || (inSize == 0)) {
        status = kIOReturnBadArgument;
        goto Exit;
    }

    bytesTransferred = kATADefaultSectorSize;

    PRINT(("SatSmartClient::GetATAIdentifyData %x\n", inSize));

    status = IOConnectCallStructMethod(fConnection,
        kIOATASMARTGetIdentifyData,
        (void*) 0, 0,
        buffer, &bytesTransferred);

    if (outSize != NULL) {
        *outSize = (UInt32) bytesTransferred;
    }

Exit:
    PRINT(("SatSmartClient::GetATAIdentifyData status = %d\n", status));

    return status;
}

#if 0
#pragma mark -
#pragma mark Static C->C++ Glue Functions
#pragma mark -
#endif

//—————————————————————————————————————————————————————————————————————————————
//	• sQueryInterface - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

HRESULT
SatSmartClient::sQueryInterface(void* self, REFIID iid, void** ppv)
{
    SatSmartClient* obj = ((InterfaceMap*) self)->obj;
    return obj->QueryInterface(iid, ppv);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sAddRef - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

UInt32
SatSmartClient::sAddRef(void* self)
{
    SatSmartClient* obj = ((InterfaceMap*) self)->obj;
    return obj->AddRef();
}

//—————————————————————————————————————————————————————————————————————————————
//	• sRelease - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

UInt32
SatSmartClient::sRelease(void* self)
{
    SatSmartClient* obj = ((InterfaceMap*) self)->obj;
    return obj->Release();
}

//—————————————————————————————————————————————————————————————————————————————
//	• sProbe - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sProbe(void* self,
    CFDictionaryRef          propertyTable,
    io_service_t             service,
    SInt32*                  order)
{
    return getThis(self)->Probe(propertyTable, service, order);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sStart - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sStart(void* self,
    CFDictionaryRef          propertyTable,
    io_service_t             service)
{
    return getThis(self)->Start(propertyTable, service);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sStop - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sStop(void* self)
{
    return getThis(self)->Stop();
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTEnableDisableOperations - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTEnableDisableOperations(void* self, Boolean enable)
{
    return getThis(self)->SMARTEnableDisableOperations(enable);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTEnableDisableAutosave - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTEnableDisableAutosave(void* self, Boolean enable)
{
    return getThis(self)->SMARTEnableDisableAutosave(enable);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTReturnStatus - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTReturnStatus(void* self, Boolean* exceededCondition)
{
    return getThis(self)->SMARTReturnStatus(exceededCondition);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTExecuteOffLineImmediate - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTExecuteOffLineImmediate(void* self, Boolean extendedTest)
{
    return getThis(self)->SMARTExecuteOffLineImmediate(extendedTest);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTReadData - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTReadData(void* self, ATASMARTData* data)
{
    return getThis(self)->SMARTReadData(data);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTValidateReadData - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTValidateReadData(void* self, const ATASMARTData* data)
{
    SInt8    checksum = 0;
    UInt32   index    = 0;
    SInt8*   ptr      = (SInt8*) data;
    IOReturn status   = kIOReturnError;

    PRINT(("sSMARTValidateReadData called\n"));

    // Checksum the 511 bytes of the structure;
    for (index = 0; index < (sizeof(ATASMARTData) - 1); index++) {
        checksum += ptr[index];
    }

    PRINT(("Checksum = %d\n", checksum));
    PRINT(("ptr[511] = %d\n", ptr[511]));

    if ((checksum + ptr[511]) == 0) {
        PRINT(("Checksum is valid\n"));
        status = kIOReturnSuccess;
    }

    return status;
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTReadDataThresholds - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTReadDataThresholds(
    void*                   self,
    ATASMARTDataThresholds* data)
{
    return getThis(self)->SMARTReadDataThresholds(data);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTReadLogDirectory - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTReadLogDirectory(void* self, ATASMARTLogDirectory* log)
{
    return getThis(self)->SMARTReadLogDirectory(log);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTReadLogAtAddress - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTReadLogAtAddress(void* self,
    UInt32                                   address,
    void*                                    buffer,
    UInt32                                   size)
{
    return getThis(self)->SMARTReadLogAtAddress(address, buffer, size);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sSMARTWriteLogAtAddress - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sSMARTWriteLogAtAddress(void* self,
    UInt32                                    address,
    const void*                               buffer,
    UInt32                                    size)
{
    return getThis(self)->SMARTWriteLogAtAddress(address, buffer, size);
}

//—————————————————————————————————————————————————————————————————————————————
//	• sGetATAIdentifyData - Static function for C->C++ glue
//																	[PROTECTED]
//—————————————————————————————————————————————————————————————————————————————

IOReturn
SatSmartClient::sGetATAIdentifyData(void* self, void* buffer, UInt32 inSize, UInt32* outSize)
{
    return getThis(self)->GetATAIdentifyData(buffer, inSize, outSize);
}
