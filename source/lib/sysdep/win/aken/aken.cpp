extern "C" {	// must come before ntddk.h

#include <ntddk.h>
#include "aken.h"

#define WIN32_NAME L"\\DosDevices\\Aken"
#define DEVICE_NAME L"\\Device\\Aken"

// note: this driver isn't much larger than a page anyway, so
// there's little point in using #pragma alloc_text.

//-----------------------------------------------------------------------------
// memory mapping
//-----------------------------------------------------------------------------

// references: DDK mapmem.c sample,
// http://support.microsoft.com/kb/189327/en-us
static NTSTATUS AkenMapPhysicalMemory(const DWORD64 physicalAddress64, const DWORD64 numBytes64, DWORD64& virtualAddress64)
{
	NTSTATUS ntStatus;

	// (convenience)
	LARGE_INTEGER physicalAddress;
	physicalAddress.QuadPart = physicalAddress64;

	// get handle to PhysicalMemory object
	HANDLE hMemory;
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

	// map desired memory into user PTEs (note: don't use MmMapIoSpace
	// because that occupies precious non-paged pool)
	{
		const HANDLE hProcess = (HANDLE)-1;
		PVOID virtualBaseAddress = 0;	// let ZwMapViewOfSection pick
		const ULONG zeroBits = 0;	// # high-order bits in address that must be 0
		SIZE_T mappedSize = (SIZE_T)numBytes64;	// will receive the actual page-aligned size
		LARGE_INTEGER physicalBaseAddress = physicalAddress;	// will be rounded down to 64KB boundary
		const SECTION_INHERIT inheritDisposition = ViewShare;
		const ULONG allocationType = 0;
		const ULONG protect = PAGE_READWRITE|PAGE_NOCACHE;
		ntStatus = ZwMapViewOfSection(hMemory, hProcess, &virtualBaseAddress, zeroBits, mappedSize, &physicalBaseAddress, &mappedSize, inheritDisposition, allocationType, protect);
		if(!NT_SUCCESS(ntStatus))
		{
			KdPrint(("AkenMapPhysicalMemory: ZwMapViewOfSection failed\n"));
			goto close_handle;
		}

		// the mapping rounded our physical base address down to the nearest
		// 64KiB boundary, so adjust the virtual address accordingly.
		const DWORD32 numBytesRoundedDown = physicalAddress.LowPart - physicalBaseAddress.LowPart;
		ASSERT(numBytesRoundedDown < 0x10000);
		virtualAddress64 = (DWORD64)virtualBaseAddress + numBytesRoundedDown;
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
	const HANDLE hProcess = (HANDLE)-1;
	PVOID baseAddress = (PVOID)virtualAddress;
	NTSTATUS ntStatus = ZwUnmapViewOfSection(hProcess, baseAddress);
	if(!NT_SUCCESS(ntStatus))
	{
		KdPrint(("AkenUnmapPhysicalMemory: ZwUnmapViewOfSection failed\n"));
		return ntStatus;
	}

	return STATUS_SUCCESS;
}


//-----------------------------------------------------------------------------
// helper functions called from DeviceControl
//-----------------------------------------------------------------------------

static NTSTATUS AkenIoctlReadPort(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	if(inSize != sizeof(AkenReadPortIn) || outSize != sizeof(AkenReadPortOut))
		return STATUS_BUFFER_TOO_SMALL;

	const AkenReadPortIn* in = (const AkenReadPortIn*)buf;
	const USHORT port = in->port;
	const UCHAR numBytes = in->numBytes;
	DWORD32 value;
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
	KdPrint(("AkenIoctlReadPort: port %x = %x\n", port, value));

	AkenReadPortOut* out = (AkenReadPortOut*)buf;
	out->value = value;
	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlWritePort(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	if(inSize != sizeof(AkenWritePortIn) || outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

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
	KdPrint(("AkenIoctlWritePort: port %x := %x\n", port, value));

	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlMap(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	if(inSize != sizeof(AkenMapIn) || outSize != sizeof(AkenMapOut))
		return STATUS_BUFFER_TOO_SMALL;

	const AkenMapIn* in = (const AkenMapIn*)buf;
	const DWORD64 physicalAddress = in->physicalAddress;
	const DWORD64 numBytes        = in->numBytes;
	DWORD64 virtualAddress;
	NTSTATUS ntStatus = AkenMapPhysicalMemory(physicalAddress, numBytes, virtualAddress);

	AkenMapOut* out = (AkenMapOut*)buf;
	out->virtualAddress = virtualAddress;
	return ntStatus;
}

static NTSTATUS AkenIoctlUnmap(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	if(inSize != sizeof(AkenUnmapIn) || outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

	const AkenUnmapIn* in = (const AkenUnmapIn*)buf;
	const DWORD64 virtualAddress = in->virtualAddress;
	NTSTATUS ntStatus = AkenUnmapPhysicalMemory(virtualAddress);

	return ntStatus;
}

static NTSTATUS AkenIoctlCopyPhysical(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	if(inSize != sizeof(AkenCopyPhysicalIn) || outSize != 0)
		return STATUS_BUFFER_TOO_SMALL;

	const AkenCopyPhysicalIn* in = (const AkenCopyPhysicalIn*)buf;
	PHYSICAL_ADDRESS physicalAddress;
	physicalAddress.QuadPart = in->physicalAddress;
	const ULONG numBytes     = (ULONG)in->numBytes;
	void* userBuffer         = (void*)(UINT_PTR)in->userAddress;

	PVOID kernelBuffer = MmMapIoSpace(physicalAddress, numBytes, MmNonCached);
	if(!kernelBuffer)
		return STATUS_NO_MEMORY;

	// (this works because we're called in the user's context)
	RtlCopyMemory(userBuffer, kernelBuffer, numBytes);

	MmUnmapIoSpace(kernelBuffer, numBytes);

	return STATUS_SUCCESS;
}

static NTSTATUS AkenIoctlUnknown(PVOID buf, const ULONG inSize, ULONG& outSize)
{
	KdPrint(("AkenIoctlUnknown\n"));
	outSize = 0;
	return STATUS_INVALID_DEVICE_REQUEST;
}


typedef NTSTATUS (*AkenIoctl)(PVOID buf, ULONG inSize, ULONG& outSize);

static inline AkenIoctl AkenIoctlFromCode(ULONG ioctlCode)
{
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

	case IOCTL_AKEN_COPY_PHYSICAL:
		return AkenIoctlCopyPhysical;

	default:
		return AkenIoctlUnknown;
	}
}


//-----------------------------------------------------------------------------
// entry points
//-----------------------------------------------------------------------------

static NTSTATUS AkenCreate(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


static NTSTATUS AkenClose(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	// same as AkenCreate ATM
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


static NTSTATUS AkenDeviceControl(IN PDEVICE_OBJECT deviceObject, IN PIRP irp)
{
	// get buffer from IRP. all our IOCTLs are METHOD_BUFFERED, so buf is
	// allocated by the I/O manager and used for both input and output.
	PVOID buf = irp->AssociatedIrp.SystemBuffer;
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
	ULONG ioctlCode    = irpStack->Parameters.DeviceIoControl.IoControlCode;
	const ULONG inSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outSize      = irpStack->Parameters.DeviceIoControl.OutputBufferLength;	// modified by AkenIoctl*

	const AkenIoctl akenIoctl = AkenIoctlFromCode(ioctlCode);
	const NTSTATUS ntStatus = akenIoctl(buf, inSize, outSize);

	irp->IoStatus.Information = outSize;	// number of bytes to copy from buf to user's buffer
	irp->IoStatus.Status = ntStatus;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return ntStatus;
}


static VOID AkenUnload(IN PDRIVER_OBJECT driverObject)
{
	KdPrint(("AkenUnload\n"));

	UNICODE_STRING win32Name = RTL_CONSTANT_STRING(WIN32_NAME);
	IoDeleteSymbolicLink(&win32Name);

	if(driverObject->DeviceObject)
		IoDeleteDevice(driverObject->DeviceObject);
}


NTSTATUS DriverEntry(IN OUT PDRIVER_OBJECT driverObject, IN PUNICODE_STRING registryPath)
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

}	// extern "C" {
