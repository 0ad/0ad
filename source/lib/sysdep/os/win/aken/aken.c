/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// note: staticdv cannot yet check C++ code.

#include <ntddk.h>
#include "aken.h"
#include "intrinsics.h"

#define WIN32_NAME L"\\DosDevices\\Aken"
#define DEVICE_NAME L"\\Device\\Aken"

// placate PREfast
DRIVER_INITIALIZE DriverEntry;
__drv_dispatchType(IRP_MJ_CREATE) DRIVER_DISPATCH AkenCreate;
__drv_dispatchType(IRP_MJ_CLOSE) DRIVER_DISPATCH AkenClose;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH AkenDeviceControl;
DRIVER_UNLOAD AkenUnload;

// this driver isn't large, but it's still slightly nicer to make its
// functions pageable and thus not waste precious non-paged pool.
// #pragma code_seg is more convenient than specifying alloc_text for
// every other function.
#pragma alloc_text(INIT, DriverEntry)	// => discardable
#pragma code_seg(push, "PAGE")


//-----------------------------------------------------------------------------
// memory mapping
//-----------------------------------------------------------------------------

/*
there are three approaches to mapping physical memory:
(http://www.microsoft.com/whdc/driver/kernel/mem-mgmt.mspx)

- MmMapIoSpace (http://support.microsoft.com/kb/189327/en-us). despite the
  name, it maps physical pages of any kind by allocating PTEs. very easy to
  implement, but occupies precious kernel address space. possible bugs:
    http://www.osronline.com/showThread.cfm?link=96737
    http://support.microsoft.com/kb/925793/en-us

- ZwMapViewOfSection of PhysicalMemory (http://tinyurl.com/yozmgy).
  the code is a bit bulky, but the WinXP API prevents mapping pages with
  conflicting attributes (see below).

- MmMapLockedPagesSpecifyCache or MmGetSystemAddressForMdlSafe
  (http://www.osronline.com/article.cfm?id=423). note: the latter is a macro
  that calls the former. this is the 'normal' and fully documented way,
  but it doesn't appear able to map a fixed physical address.
  (MmAllocatePagesForMdl understandably doesn't work since some pages we
  want to map are marked as unavailable for allocation, and I don't see
  another documented way to fill an MDL with PFNs.)

our choice here is forced by a very insidious issue. if someone else has
already mapped a page with different attributes (e.g. cacheable), TLBs
may end up corrupted, leading to disaster. the search for a documented
means of accessing the page frame database (to check if mapped anywhere
and determine the previously set attributes) has not borne fruit, so we
must use ZwMapViewOfSection. (if taking this up again, see
http://www.woodmann.com/forum/archive/index.php/t-6516.html )

note that we guess if the page will have been mapped as cacheable and
even try the opposite if that turns out to have been incorrect.
*/

static int IsMemoryUncacheable(DWORD64 physicalAddress64)
{
	PAGED_CODE();

	// original PC memory - contains BIOS
	if(physicalAddress64 < 0x100000)
		return 1;

	return 0;
}

static NTSTATUS AkenMapPhysicalMemory(const DWORD64 physicalAddress64, const DWORD64 numBytes64, DWORD64* virtualAddress64)
{
	NTSTATUS ntStatus;
	HANDLE hMemory;
	LARGE_INTEGER physicalAddress;	// convenience
	physicalAddress.QuadPart = physicalAddress64;

	PAGED_CODE();

	// get handle to PhysicalMemory object
	{
		OBJECT_ATTRIBUTES objectAttributes;
		UNICODE_STRING objectName = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
		const ULONG attributes = OBJ_CASE_INSENSITIVE;
		const HANDLE rootDirectory = 0;
		InitializeObjectAttributes(&objectAttributes, &objectName, attributes, rootDirectory, (PSECURITY_DESCRIPTOR)0);
		ntStatus = ZwOpenSection(&hMemory, SECTION_ALL_ACCESS, &objectAttributes);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("AkenMapPhysicalMemory: ZwOpenSection failed\n"));
			return ntStatus;
		}
	}

	// add a reference (required to prevent the handle from being deleted)
	{
		PVOID physicalMemorySection = NULL;
		const POBJECT_TYPE objectType = 0;	// allowed since specifying KernelMode
		ntStatus = ObReferenceObjectByHandle(hMemory, SECTION_ALL_ACCESS, objectType, KernelMode, &physicalMemorySection, 0);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("AkenMapPhysicalMemory: ObReferenceObjectByHandle failed\n"));
			goto close_handle;
		}
	}

	// note: mapmem.c does HalTranslateBusAddress, but we only care about
	// system memory. translating doesn't appear to be necessary, even if
	// much existing code uses it (probably due to cargo cult).

	// map desired memory into user PTEs
	{
		const HANDLE hProcess = (HANDLE)-1;
		PVOID virtualBaseAddress = 0;	// let ZwMapViewOfSection pick
		const ULONG zeroBits = 0;	// # high-order bits in address that must be 0
		SIZE_T mappedSize = (SIZE_T)numBytes64;	// will receive the actual page-aligned size
		LARGE_INTEGER physicalBaseAddress = physicalAddress;	// will be rounded down to 64KB boundary
		const SECTION_INHERIT inheritDisposition = ViewShare;
		const ULONG allocationType = 0;
		ULONG protect = PAGE_READWRITE;
		if(IsMemoryUncacheable(physicalAddress64))
			protect |= PAGE_NOCACHE;
		ntStatus = ZwMapViewOfSection(hMemory, hProcess, &virtualBaseAddress, zeroBits, mappedSize, &physicalBaseAddress, &mappedSize, inheritDisposition, allocationType, protect);
		if(!NT_SUCCESS(ntStatus))
		{
			// try again with the opposite cacheability attribute
			protect ^= PAGE_NOCACHE;
			ntStatus = ZwMapViewOfSection(hMemory, hProcess, &virtualBaseAddress, zeroBits, mappedSize, &physicalBaseAddress, &mappedSize, inheritDisposition, allocationType, protect);
			if(!NT_SUCCESS(ntStatus))
			{
				KdPrint(("AkenMapPhysicalMemory: ZwMapViewOfSection failed\n"));
				goto close_handle;
			}
		}

		// the mapping rounded our physical base address down to the nearest
		// 64KiB boundary, so adjust the virtual address accordingly.
		{
			const DWORD32 numBytesRoundedDown = physicalAddress.LowPart - physicalBaseAddress.LowPart;
			ASSERT(numBytesRoundedDown < 0x10000);
			*virtualAddress64 = (DWORD64)virtualBaseAddress + numBytesRoundedDown;
		}
	}

	ntStatus = STATUS_SUCCESS;

close_handle:
	// closing the handle even on success means that callers won't have to
	// pass it back when unmapping. why does this work? ZwMapViewOfSection
	// apparently adds a reference to hMemory.
	ZwClose(hMemory);

	return ntStatus;
}


static NTSTATUS AkenUnmapPhysicalMemory(const DWORD64 virtualAddress)
{
	PAGED_CODE();

	{
		const HANDLE hProcess = (HANDLE)-1;
		PVOID baseAddress = (PVOID)virtualAddress;
		NTSTATUS ntStatus = ZwUnmapViewOfSection(hProcess, baseAddress);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("AkenUnmapPhysicalMemory: ZwUnmapViewOfSection failed\n"));
			return ntStatus;
		}
	}

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// helper functions called from DeviceControl
//-----------------------------------------------------------------------------

static NTSTATUS AkenIoctlReadPort(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	DWORD32 value;

	PAGED_CODE();

	if(inSize != sizeof(AkenReadPortIn) || *outSize != sizeof(AkenReadPortOut))
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenReadPortIn* in = (const AkenReadPortIn*)buf;
		const USHORT port = in->port;
		const UCHAR numBytes = in->numBytes;
		switch(numBytes)
		{
		case 1:
			value = (DWORD32)READ_PORT_UCHAR((PUCHAR)port);
			break;
		case 2:
			value = (DWORD32)READ_PORT_USHORT((PUSHORT)port);
			break;
		case 4:
			value = (DWORD32)READ_PORT_ULONG((PULONG)port);
			break;
		default:
			return STATUS_INVALID_PARAMETER;
		}
	}

	{
		AkenReadPortOut* out = (AkenReadPortOut*)buf;
		out->value = value;
	}
	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlWritePort(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	PAGED_CODE();

	if(inSize != sizeof(AkenWritePortIn) || *outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenWritePortIn* in = (const AkenWritePortIn*)buf;
		const DWORD32 value  = in->value;
		const USHORT port    = in->port;
		const UCHAR numBytes = in->numBytes;
		switch(numBytes)
		{
		case 1:
			WRITE_PORT_UCHAR((PUCHAR)port, (UCHAR)(value & 0xFF));
			break;
		case 2:
			WRITE_PORT_USHORT((PUSHORT)port, (USHORT)(value & 0xFFFF));
			break;
		case 4:
			WRITE_PORT_ULONG((PULONG)port, value);
			break;
		default:
			return STATUS_INVALID_PARAMETER;
		}
	}

	return STATUS_SUCCESS;
}


static NTSTATUS AkenIoctlMap(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	DWORD64 virtualAddress;
	NTSTATUS ntStatus;

	PAGED_CODE();

	if(inSize != sizeof(AkenMapIn) || *outSize != sizeof(AkenMapOut))
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenMapIn* in = (const AkenMapIn*)buf;
		const DWORD64 physicalAddress = in->physicalAddress;
		const DWORD64 numBytes        = in->numBytes;
		ntStatus = AkenMapPhysicalMemory(physicalAddress, numBytes, &virtualAddress);
	}

	{
		AkenMapOut* out = (AkenMapOut*)buf;
		out->virtualAddress = virtualAddress;
	}
	return ntStatus;
}

static NTSTATUS AkenIoctlUnmap(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	NTSTATUS ntStatus;

	PAGED_CODE();

	if(inSize != sizeof(AkenUnmapIn) || *outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenUnmapIn* in = (const AkenUnmapIn*)buf;
		const DWORD64 virtualAddress = in->virtualAddress;
		ntStatus = AkenUnmapPhysicalMemory(virtualAddress);
	}

	return ntStatus;
}


static NTSTATUS AkenIoctlReadModelSpecificRegister(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	DWORD64 value;

	PAGED_CODE();

	if(inSize != sizeof(AkenReadRegisterIn) || *outSize != sizeof(AkenReadRegisterOut))
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenReadRegisterIn* in = (const AkenReadRegisterIn*)buf;
		const DWORD64 reg = in->reg;
		value = __readmsr((int)reg);
	}

	{
		AkenReadRegisterOut* out = (AkenReadRegisterOut*)buf;
		out->value = value;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlWriteModelSpecificRegister(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	PAGED_CODE();

	if(inSize != sizeof(AkenWriteRegisterIn) || *outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenWriteRegisterIn* in = (const AkenWriteRegisterIn*)buf;
		const DWORD64 reg   = in->reg;
		const DWORD64 value = in->value;
		__writemsr((unsigned long)reg, value);
	}

	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlReadPerformanceMonitoringCounter(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	DWORD64 value;

	PAGED_CODE();

	if(inSize != sizeof(AkenReadRegisterIn) || *outSize != sizeof(AkenReadRegisterOut))
		return STATUS_BUFFER_TOO_SMALL;

	{
		const AkenReadRegisterIn* in = (const AkenReadRegisterIn*)buf;
		const DWORD64 reg = in->reg;
		value = __readpmc((unsigned long)reg);
	}

	{
		AkenReadRegisterOut* out = (AkenReadRegisterOut*)buf;
		out->value = value;
	}

	return STATUS_SUCCESS;
}


static NTSTATUS AkenIoctlUnknown(PVOID buf, const ULONG inSize, ULONG* outSize)
{
	PAGED_CODE();

	KdPrint(("AkenIoctlUnknown\n"));

	*outSize = 0;
	return STATUS_INVALID_DEVICE_REQUEST;
}


typedef NTSTATUS (*AkenIoctl)(PVOID buf, ULONG inSize, ULONG* outSize);

static AkenIoctl AkenIoctlFromCode(ULONG ioctlCode)
{
	PAGED_CODE();

	switch(ioctlCode)
	{
	case IOCTL_AKEN_READ_PORT:
		return AkenIoctlReadPort;
	case IOCTL_AKEN_WRITE_PORT:
		return AkenIoctlWritePort;

	case IOCTL_AKEN_MAP:
		return AkenIoctlMap;
	case IOCTL_AKEN_UNMAP:
		return AkenIoctlUnmap;

	case IOCTL_AKEN_READ_MSR:
		return AkenIoctlReadModelSpecificRegister;
	case IOCTL_AKEN_WRITE_MSR:
		return AkenIoctlWriteModelSpecificRegister;

	default:
		return AkenIoctlUnknown;
	}
}


//-----------------------------------------------------------------------------
// entry points
//-----------------------------------------------------------------------------

static NTSTATUS AkenCreate(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	PAGED_CODE();

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


static NTSTATUS AkenClose(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	PAGED_CODE();

	// same as AkenCreate ATM
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


static NTSTATUS AkenDeviceControl(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	PAGED_CODE();

	{
		// get buffer from IRP. all our IOCTLs are METHOD_BUFFERED, so buf is
		// allocated by the I/O manager and used for both input and output.
		PVOID buf = irp->AssociatedIrp.SystemBuffer;
		PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
		ULONG ioctlCode    = irpStack->Parameters.DeviceIoControl.IoControlCode;
		const ULONG inSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
		ULONG outSize      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;	// modified by AkenIoctl*

		const AkenIoctl akenIoctl = AkenIoctlFromCode(ioctlCode);
		const NTSTATUS ntStatus = akenIoctl(buf, inSize, &outSize);

		irp->IoStatus.Information = outSize;	// number of bytes to copy from buf to user's buffer
		irp->IoStatus.Status = ntStatus;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return ntStatus;
	}
}


static VOID AkenUnload(IN PDRIVER_OBJECT driverObject)
{
	PAGED_CODE();

	KdPrint(("AkenUnload\n"));

	{
		UNICODE_STRING win32Name = RTL_CONSTANT_STRING(WIN32_NAME);
		IoDeleteSymbolicLink(&win32Name);
	}

	if(driverObject->DeviceObject)
		IoDeleteDevice(driverObject->DeviceObject);
}


#pragma code_seg(pop)	// make sure we don't countermand the alloc_text

NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject, IN PUNICODE_STRING registryPath)
{
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(DEVICE_NAME);

	// create device object
	PDEVICE_OBJECT deviceObject;
	{
		const ULONG deviceExtensionSize = 0;
		const ULONG deviceCharacteristics = FILE_DEVICE_SECURE_OPEN;
		const BOOLEAN exlusive = TRUE;
		NTSTATUS ntStatus = IoCreateDevice(driverObject, deviceExtensionSize, &deviceName, FILE_DEVICE_AKEN, deviceCharacteristics, exlusive, &deviceObject);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("DriverEntry: IoCreateDevice failed\n"));
			return ntStatus;
		}
	}

	// set entry points
	driverObject->MajorFunction[IRP_MJ_CREATE] = AkenCreate;
	driverObject->MajorFunction[IRP_MJ_CLOSE]  = AkenClose;
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AkenDeviceControl;
	driverObject->DriverUnload = AkenUnload;

	// symlink NT device name to Win32 namespace
	{
		UNICODE_STRING win32Name = RTL_CONSTANT_STRING(WIN32_NAME);
		NTSTATUS ntStatus = IoCreateSymbolicLink(&win32Name, &deviceName);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("DriverEntry: IoCreateSymbolicLink failed\n"));
			IoDeleteDevice(deviceObject);
			return ntStatus;
		}
	}

	return STATUS_SUCCESS;
}
