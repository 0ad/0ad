#include "precompiled.h"
#include "acpi.h"

#include "win/mahaf.h"
#include "lib/sysdep/cpu.h"

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
	// 4 KiB ought to be enough; if not, the table will be
	// re-mapped with the actual size.
	const size_t initialSize = 4*KiB;

	const AcpiTable* mappedTable = (const AcpiTable*)MapPhysicalMemory(physicalAddress, initialSize);
	if(!mappedTable)
		return 0;
	const size_t size = mappedTable->size;

	if(size > initialSize)
	{
		UnmapPhysicalMemory((void*)mappedTable);
		mappedTable = (const AcpiTable*)MapPhysicalMemory(physicalAddress, size);
		if(!mappedTable)
			return 0;
	}

	AcpiTable* table = (AcpiTable*)malloc(size);
	if(table)
		cpu_memcpy(table, mappedTable, size);

	UnmapPhysicalMemory((void*)mappedTable);
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
// get pointer to (eXtended) Root System Descriptor Table
//-----------------------------------------------------------------------------

// Root System Descriptor Pointer

static const size_t RSDP_ALIGNMENT = 16;

struct RSDPv1
{
	char signature[8];			// "RSD PTR "
	u8 checksum;				// sum of this struct = 0
	char oemId[6];
	u8 revision;				// 0 for 1.0, 2 for 2.0
	u32 rsdtPhysicalAddress;
};

struct RSDPv2Additions
{
	u32 size;					// of entire table (including V1)
	u64 xsdtPhysicalAddress64;
	u8 extendedChecksum;		// sum of entire table (including V1) = 0
	char reserved[3];			// must be 0
};

struct RSDP
{
	RSDPv1 v1;
	RSDPv2Additions v2;
};


static const RSDP* LocateRsdp(const u8* buf, size_t numBytes)
{
	const u8* const end = buf+numBytes;
	for(const u8* p = buf; p < end; p += RSDP_ALIGNMENT)
	{
		const RSDP* rsdp = (const RSDP*)p;

		if(memcmp(rsdp->v1.signature, "RSD PTR ", 8) != 0)
			continue;

		if(ComputeChecksum(p, 20) != 0)
			continue;

		return rsdp;
	}

	return 0;
}


static bool LocateAndRetrieveRsdp(uintptr_t physicalAddress, size_t numBytes, RSDP& rsdp)
{
	void* virtualAddress = MapPhysicalMemory(physicalAddress, numBytes);

	const RSDP* prsdp = LocateRsdp((const u8*)virtualAddress, numBytes);
	if(prsdp)
		rsdp = *prsdp;	// stash in output parameter before unmapping

	UnmapPhysicalMemory(virtualAddress);
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
	const BiosDataArea* bda = (const BiosDataArea*)MapPhysicalMemory(0x400, 0x100);
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


static bool VerifyRsdp(const RSDP& rsdp)
{
	if(ComputeChecksum(&rsdp.v1, sizeof(rsdp.v1)) != 0)
		return false;

	if(rsdp.v1.revision >= 2)
	{
		if(ComputeChecksum(&rsdp, rsdp.v2.size) != 0)
			return false;
	}

	return true;
}

// Root System Descriptor Table
struct RSDT
{
	AcpiTable header;
	u32 tables[1];
};

// eXtended root System Descriptor Table
struct XSDT
{
	AcpiTable header;
	u64 tables[1];
};


// caller is responsible for verifying the table is valid and must
// free() the returned pointer.
static const XSDT* AllocateCopyOfXsdt()
{
	RSDP rsdp;
	if(!RetrieveRsdp(rsdp))
		return 0;

	if(!VerifyRsdp(rsdp))
		return 0;

	// callers should only have to deal with XSDTs (same as RSDT but with
	// 64-bit pointers). if running on ACPI 2.0, just return XSDT,
	// otherwise convert RSDT to XSDT.

	if(rsdp.v1.revision >= 2)
		return (const XSDT*)AllocateCopyOfTable(rsdp.v2.xsdtPhysicalAddress64);

	const RSDT* rsdt = (const RSDT*)AllocateCopyOfTable(rsdp.v1.rsdtPhysicalAddress);
	if(!rsdt)
		return 0;
	if(!VerifyTable((const AcpiTable*)rsdt, "RSDT"))
	{
		free((void*)rsdt);
		return 0;
	}
	const size_t numTables = (rsdt->header.size - sizeof(AcpiTable)) / sizeof(u32);
	const size_t xsdtSize = sizeof(AcpiTable) + numTables * sizeof(u64);
	XSDT* xsdt = (XSDT*)malloc(xsdtSize);
	if(xsdt)
	{
		xsdt->header = rsdt->header;
		cpu_memcpy(xsdt->header.signature, "XSDT", 4);
		xsdt->header.size = (u32)xsdtSize;
		for(size_t i = 0; i < numTables; i++)
			xsdt->tables[i] = (u64)rsdt->tables[i];
		xsdt->header.checksum = -ComputeChecksum(xsdt, xsdtSize);
	}

	free((void*)rsdt);
	return xsdt;
}


//-----------------------------------------------------------------------------

typedef std::map<u32, const AcpiTable*> Tables;
static Tables tables;

static bool LatchAllTables()
{
	const XSDT* xsdt = AllocateCopyOfXsdt();
	if(!xsdt)
		return false;

	if(!VerifyTable((const AcpiTable*)xsdt, "XSDT"))
	{
		free((void*)xsdt);
		return false;
	}

	const size_t numTables = (xsdt->header.size - sizeof(AcpiTable)) / sizeof(u64);
	for(size_t i = 0; i < numTables; i++)
	{
		const AcpiTable* table = AllocateCopyOfTable(xsdt->tables[i]);
		if(!table)
			continue;
		if(!VerifyTable(table))
			debug_warn("invalid ACPI table");
		const u32 signature32 = *(u32*)table->signature;
		tables[signature32] = table;
	}

	return true;
}


static void FreeAllTables()
{
	for(Tables::iterator it(tables.begin()); it != tables.end(); ++it)
	{
		std::pair<u32, const AcpiTable*> item = *it;
		free((void*)item.second);
	}
}


const AcpiTable* acpiGetTable(const char* signature)
{
	const u32 signature32 = *(u32*)signature;
	const AcpiTable* table = tables[signature32];
	return table;
}


//-----------------------------------------------------------------------------

bool acpiInit()
{
	if(!MahafInit())
		return false;

	LatchAllTables();
	return true;
}

void acpiShutdown()
{
	FreeAllTables();

	MahafShutdown();
}
