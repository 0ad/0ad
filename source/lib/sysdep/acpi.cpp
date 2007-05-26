#include "precompiled.h"
#include "acpi.h"

#include "win/mahaf.h"
#include "lib/sysdep/cpu.h"
#include "lib/module_init.h"

#pragma pack(1)

//-----------------------------------------------------------------------------
// table utility functions
//-----------------------------------------------------------------------------

// return 8-bit checksum of a buffer (should be 0)
static u8 ComputeChecksum(const void* buf, size_t numBytes)
{
	u8 sum = 0;
	const u8* end = (const u8*)buf+numBytes;
	for(const u8* p = (const u8*)buf; p < end; p++)
		sum += *p;

	return sum;
}


// caller is responsible for verifying the table is valid and must
// free() the returned pointer.
static const AcpiTable* AllocateCopyOfTable(u64 physicalAddress)
{
	// 4 KiB ought to be enough; if not, the table will be re-mapped.
	const size_t initialSize = 4*KiB;
	const AcpiTable* mappedTable = (const AcpiTable*)mahaf_MapPhysicalMemory(physicalAddress, initialSize);
	if(!mappedTable)
		return 0;
	const size_t size = mappedTable->size;

	if(size > initialSize)
	{
		// re-map with correct size
		mahaf_UnmapPhysicalMemory((void*)mappedTable);
		mappedTable = (const AcpiTable*)mahaf_MapPhysicalMemory(physicalAddress, size);
		if(!mappedTable)
			return 0;
	}

	AcpiTable* table = (AcpiTable*)malloc(size);
	if(table)
		cpu_memcpy(table, mappedTable, size);

	mahaf_UnmapPhysicalMemory((void*)mappedTable);
	return table;
}


static bool VerifyTable(const AcpiTable* table, const char* signature = 0)
{
	// caller knowns the signature; make sure it matches
	if(signature)
	{
		if(memcmp(table->signature, signature, 4) != 0)
			return false;
	}
	// no specific signature is called for; just make sure it's 4 letters
	else
	{
		for(int i = 0; i < 4; i++)
		{
			if(!isalpha(table->signature[i]))
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
	if(ComputeChecksum(table, table->size) != 0 && !isOemTable)
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Root System Descriptor Pointer
//-----------------------------------------------------------------------------

struct RSDP
{
	char signature[8];			// "RSD PTR "
	u8 checksum;				// sum of this struct = 0
	char oemId[6];
	u8 revision;				// 0 for 1.0, 2 for 2.0
	u32 rsdtPhysicalAddress;
};

static const size_t RSDP_ALIGNMENT = 16;

static const RSDP* LocateRsdp(const u8* buf, size_t numBytes)
{
	const u8* const end = buf+numBytes;
	for(const u8* p = buf; p < end; p += RSDP_ALIGNMENT)
	{
		const RSDP* rsdp = (const RSDP*)p;

		if(memcmp(rsdp->signature, "RSD PTR ", 8) != 0)
			continue;

		if(ComputeChecksum(p, sizeof(RSDP)) != 0)
			continue;

		return rsdp;
	}

	return 0;
}


static bool LocateAndRetrieveRsdp(uintptr_t physicalAddress, size_t numBytes, RSDP& rsdp)
{
	void* virtualAddress = mahaf_MapPhysicalMemory(physicalAddress, numBytes);

	const RSDP* prsdp = LocateRsdp((const u8*)virtualAddress, numBytes);
	if(prsdp)
		rsdp = *prsdp;	// stash in output parameter before unmapping

	mahaf_UnmapPhysicalMemory(virtualAddress);
	return (prsdp != 0);
}


static uintptr_t LocateEbdaPhysicalAddress()
{
	struct BiosDataArea
	{ 
		u16 serialBase[4];
		u16 parallelBase[3];
		u16 ebdaSegment;
		// ...
	};
	const BiosDataArea* bda = (const BiosDataArea*)mahaf_MapPhysicalMemory(0x400, 0x100);
	if(!bda)
		return 0;
	const uintptr_t ebdaPhysicalAddress = ((uintptr_t)bda->ebdaSegment) * 16;

	return ebdaPhysicalAddress;
}


static bool RetrieveRsdp(RSDP& rsdp)
{
	// See ACPIspec30b, section 5.2.5.1:
	// RSDP is either in the first KIB of the extended BIOS data area,
	if(LocateAndRetrieveRsdp(LocateEbdaPhysicalAddress(), 1*KiB, rsdp))
		return true;

	// or in read-only BIOS memory.
	if(LocateAndRetrieveRsdp(0xE0000, 0x20000, rsdp))
		return true;

	return false;	// not found
}


//-----------------------------------------------------------------------------

// Root System Descriptor Table
struct RSDT
{
	AcpiTable header;
	u32 tables[1];
};

// avoid std::map et al. because we are called before _cinit
static const AcpiTable** tables;
static size_t numTables;

static bool LatchAllTables()
{
	RSDP rsdp;
	if(!RetrieveRsdp(rsdp))
		return false;
	const RSDT* rsdt = (const RSDT*)AllocateCopyOfTable(rsdp.rsdtPhysicalAddress);
	if(!rsdt)
		return false;
	if(!VerifyTable((const AcpiTable*)rsdt, "RSDT"))
	{
		free((void*)rsdt);
		return false;
	}

	numTables = (rsdt->header.size - sizeof(AcpiTable)) / sizeof(rsdt->tables[0]);
	tables = new const AcpiTable*[numTables];

	for(size_t i = 0; i < numTables; i++)
	{
		const AcpiTable* table = AllocateCopyOfTable(rsdt->tables[i]);
		if(!table)
			continue;
		if(!VerifyTable(table))
			debug_warn("invalid ACPI table");
		tables[i] = table;	// transfers ownership
	}

	free((void*)rsdt);
	return true;
}


static void FreeAllTables()
{
	for(size_t i = 0; i < numTables; i++)
	{
		SAFE_FREE(tables[i]);
	}
	delete[] tables;
}


const AcpiTable* acpi_GetTable(const char* signature)
{
	// (typically only a few tables, linear search is OK)
	for(size_t i = 0; i < numTables; i++)
	{
		const AcpiTable* table = tables[i];
		if(strncmp(table->signature, signature, 4) == 0)
			return table;
	}

	return 0;
}


//-----------------------------------------------------------------------------

static ModuleInitState initState;

bool acpi_Init()
{
	if(!ModuleShouldInitialize(&initState))
		return true;

	if(!mahaf_Init())
	{
		ModuleSetError(&initState);
		return false;
	}

	LatchAllTables();
	return true;
}

void acpi_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	FreeAllTables();

	mahaf_Shutdown();
}
