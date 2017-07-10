/* Copyright (C) 2010 Wildfire Games.
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

#include "precompiled.h"
#include "lib/sysdep/acpi.h"

#include "lib/byte_order.h"
#include "lib/sysdep/cpu.h"
#include "lib/module_init.h"

#define ENABLE_MAHAF 0
#if ENABLE_MAHAF
# include "lib/sysdep/os/win/mahaf.h"
#else
# include "lib/sysdep/os/win/wfirmware.h"
#endif

#pragma pack(1)

typedef const volatile u8* PCV_u8;
typedef const volatile AcpiTable* PCV_AcpiTable;


//-----------------------------------------------------------------------------
// table

static AcpiTable* AllocateTable(size_t size)
{
	ENSURE(size >= sizeof(AcpiTable));
	return (AcpiTable*)malloc(size);
}


template<typename T>
static void DeallocateTable(const T* table)
{
	free((void*)table);
}


// return 8-bit checksum of a buffer (should be 0)
static u8 ComputeChecksum(PCV_u8 buf, size_t numBytes)
{
	// (can't use std::accumulate - we need 8-bit wraparound)
	u8 sum = 0;
	for(PCV_u8 p = buf; p < buf+numBytes; p++)
		sum = u8((sum + *p) & 0xFF);
	return sum;
}


static bool ValidateTable(const AcpiTable* table, const char* signature = 0)
{
	if(!table)
		return false;

	// caller knowns the signature; make sure it matches
	if(signature)
	{
		if(memcmp(table->signature, signature, 4) != 0)
			return false;
	}
	// no specific signature is called for, just validate the characters.
	else
	{
		for(size_t i = 0; i < 4; i++)
		{
			const char c = table->signature[i];
			// "ASF!" and "____" have been encountered
			if(!isalpha(c) && c != '_' && c != '!')
				return false;
		}
	}

	// must be at least as large as the common header
	if(table->size < sizeof(AcpiTable))
		return false;

	// checksum of table must be 0
	// .. AMIBIOS OEMB table has an incorrect checksum (off-by-one),
	// so don't complain about any OEM tables (ignored anyway).
	const bool isOemTable = (memcmp(table->signature, "OEM", 3) == 0);
	if(!isOemTable)
	{
		if(ComputeChecksum((PCV_u8)table, table->size) != 0)
			return false;
	}

	return true;
}


#if ENABLE_MAHAF

//-----------------------------------------------------------------------------
// exception-safe transactional map/use/unmap

// note: if the OS happens to unmap our physical memory, the Unsafe*
// functions may crash. we catch this via SEH; on Unix, we'd need handlers
// for SIGBUS and/or SIGSEGV. the code is safe in that it releases the
// mapped memory and returns an error code.

static void* SUCCEEDED = (void*)(intptr_t)1;
static void* FAILED    = (void*)(intptr_t)-1;

typedef void* (*UnsafeFunction)(PCV_u8 mem, size_t numBytes, void* arg);

static void* CallWithSafetyBlanket(UnsafeFunction func, PCV_u8 mem, size_t numBytes, void* arg)
{
#if MSC_VERSION
	__try
	{
		return func(mem, numBytes, arg);
	}
	__except(1)
	{
		return FAILED;
	}
#else
	return func(mem, numBytes, arg);
#endif
}

static void* TransactPhysicalMemory(uintptr_t physicalAddress, size_t numBytes, UnsafeFunction func, void* arg = 0)
{
	PCV_u8 mem = (PCV_u8)mahaf_MapPhysicalMemory(physicalAddress, numBytes);
	if(!mem)
		return FAILED;
	void* ret = CallWithSafetyBlanket(func, mem, numBytes, arg);
	mahaf_UnmapPhysicalMemory((volatile void*)mem);
	return ret;
}


//-----------------------------------------------------------------------------
// Root System Descriptor Pointer

struct BiosDataArea
{
	u16 serialBase[4];
	u16 parallelBase[3];
	u16 ebdaSegment;
};

typedef const volatile BiosDataArea* PCV_BiosDataArea;

static void* UnsafeReadEbdaPhysicalAddress(PCV_u8 mem, size_t numBytes, void* UNUSED(arg))
{
	ENSURE(numBytes >= sizeof(BiosDataArea));

	PCV_BiosDataArea bda = (PCV_BiosDataArea)mem;
	const uintptr_t ebdaPhysicalAddress = ((uintptr_t)bda->ebdaSegment) * 16;
	return (void*)ebdaPhysicalAddress;
}


struct RSDP
{
	char signature[8];			// "RSD PTR "
	u8 checksum;				// sum of this struct = 0
	char oemId[6];
	u8 revision;				// 0 for 1.0, 2 for 2.0
	u32 rsdtPhysicalAddress;
};

typedef const volatile RSDP* PCV_RSDP;

static const size_t RSDP_ALIGNMENT = 16;

static void* UnsafeLocateAndRetrieveRsdp(PCV_u8 buf, size_t numBytes, void* arg)
{
	ENSURE(numBytes >= sizeof(RSDP));

	for(PCV_u8 p = buf; p < buf+numBytes; p += RSDP_ALIGNMENT)
	{
		RSDP* prsdp = (RSDP*)p;
		if(memcmp(prsdp->signature, "RSD PTR ", 8) != 0)
			continue;
		if(ComputeChecksum(p, sizeof(RSDP)) != 0)
			continue;

		memcpy(arg, prsdp, sizeof(RSDP));
		return SUCCEEDED;
	}

	return FAILED;
}

static bool RetrieveRsdp(RSDP& rsdp)
{
	// See ACPIspec30b, section 5.2.5.1:
	// RSDP is either in the first KIB of the extended BIOS data area,
	void* ret = TransactPhysicalMemory(0x400, 0x100, UnsafeReadEbdaPhysicalAddress);
	if(ret != FAILED)
	{
		const uintptr_t ebdaPhysicalAddress = (uintptr_t)ret;
		ret = TransactPhysicalMemory(ebdaPhysicalAddress, 0x400, UnsafeLocateAndRetrieveRsdp, &rsdp);
		if(ret == SUCCEEDED)
			return true;
	}

	// or in read-only BIOS memory.
	ret = TransactPhysicalMemory(0xE0000, 0x20000, UnsafeLocateAndRetrieveRsdp, &rsdp);
	if(ret == SUCCEEDED)
		return true;

	return false;	// not found
}


//-----------------------------------------------------------------------------
// copy tables from physical memory

static void* UnsafeAllocateAndCopyTable(PCV_u8 mem, size_t numBytes, void* arg)
{
	ENSURE(numBytes >= sizeof(AcpiTable));

	PCV_AcpiTable table = (PCV_AcpiTable)mem;
	const size_t tableSize = table->size;

	// physical memory window is smaller than the table
	// (caller will map a larger window and call us again)
	if(numBytes < tableSize)
	{
		memcpy(arg, &tableSize, sizeof(size_t));
		return 0;
	}

	PCV_u8 copy = (PCV_u8)AllocateTable(tableSize);
	if(!copy)
		return FAILED;

	memcpy((void*)copy, (const void*)mem, tableSize);
	return (void*)copy;
}


static const AcpiTable* AllocateAndCopyTable(uintptr_t physicalAddress)
{
	// ACPI table sizes are not known until they've been mapped. since that
	// is slow, we don't always want to do it twice. the solution is to map
	// enough for a typical table; if that is too small, realloc and map again.
	static const size_t initialSize = 4*KiB;
	size_t actualSize = 0;
	void* ret = TransactPhysicalMemory(physicalAddress, initialSize, UnsafeAllocateAndCopyTable, &actualSize);
	// initialSize was too small; actualSize has been set
	if(ret == 0)
		ret = TransactPhysicalMemory(physicalAddress, actualSize, UnsafeAllocateAndCopyTable);
	// *either* of the above calls failed to allocate memory
	if(ret == FAILED)
		return 0;
	return (const AcpiTable*)ret;
}

#endif	// ENABLE_MAHAF


static void AllocateAndCopyTables(const AcpiTable**& tables, size_t& numTables)
{
#if ENABLE_MAHAF
	if(mahaf_IsPhysicalMappingDangerous())
		return;
	if(mahaf_Init() != INFO::OK)
		return;

	RSDP rsdp;
	if(!RetrieveRsdp(rsdp))
		return;

	// Root System Descriptor Table
	struct RSDT
	{
		AcpiTable header;
		u32 tableAddresses[1];
	};
	const RSDT* rsdt = (const RSDT*)AllocateAndCopyTable(rsdp.rsdtPhysicalAddress);
	if(!ValidateTable(&rsdt->header, "RSDT"))
	{
		DeallocateTable(rsdt);
		return;
	}

	numTables = (rsdt->header.size - sizeof(AcpiTable)) / sizeof(rsdt->tableAddresses[0]);
	ENSURE(numTables != 0);

	tables = new const AcpiTable*[numTables];
	for(size_t i = 0; i < numTables; i++)
		tables[i] = AllocateAndCopyTable(rsdt->tableAddresses[i]);

	DeallocateTable(rsdt);
#else
	const wfirmware::Provider provider = FOURCC_BE('A','C','P','I');
	const wfirmware::TableIds tableIDs = wfirmware::GetTableIDs(provider);

	numTables = tableIDs.size();
	tables = new const AcpiTable*[numTables];

	for(size_t i = 0; i < numTables; i++)
	{
		wfirmware::Table table = wfirmware::GetTable(provider, tableIDs[i]);
		ENSURE(!table.empty());
		tables[i] = AllocateTable(table.size());
		memcpy((void*)tables[i], &table[0], table.size());
	}
#endif

	// to prevent callers from choking on invalid tables, we
	// zero out the corresponding tables[] entries.
	for(size_t i = 0; i < numTables; i++)
	{
		if(!ValidateTable(tables[i]))
		{
			DeallocateTable(tables[i]);
			tables[i] = 0;
		}
	}
}


//-----------------------------------------------------------------------------

// note: avoid global std::map etc. because we may be called before _cinit
static const AcpiTable** tables;	// tables == 0 <=> not initialized
static const AcpiTable* invalidTables;	// tables == &invalidTables => init failed
static size_t numTables;

void acpi_Shutdown()
{
	if(tables)
	{
		for(size_t i = 0; i < numTables; i++)
			DeallocateTable(tables[i]);
		SAFE_ARRAY_DELETE(tables);
		numTables = 0;
	}

#if ENABLE_MAHAF
	mahaf_Shutdown();
#endif
}


const AcpiTable* acpi_GetTable(const char* signature)
{
	if(cpu_CAS(&tables, (const AcpiTable**)0, &invalidTables))
		AllocateAndCopyTables(tables, numTables);

	// (typically only a few tables, linear search is OK)
	for(size_t i = 0; i < numTables; i++)
	{
		const AcpiTable* table = tables[i];
		if(!table)
			continue;	// skip invalid tables, e.g. OEM (see above)
		if(strncmp(table->signature, signature, 4) == 0)
			return table;
	}

	return 0;	// no matching AND valid table found
}
