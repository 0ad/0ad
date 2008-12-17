/**
 * =========================================================================
 * File        : acpi.h
 * Project     : 0 A.D.
 * Description : minimal subset of ACPI
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_ACPI
#define INCLUDED_ACPI

#pragma pack(push, 1)

// common header for all ACPI tables
struct AcpiTable
{
	char signature[4];
	u32 size;					// table size [bytes], including header
	u8 revision;
	u8 checksum;				// to make sum of entire table == 0
	char oemId[6];
	char oemTableId[8];
	u32 oemRevision;
	char creatorId[4];
	u32 creatorRevision;
};

enum AcpiAddressSpace
{
	// (these are not generally powers-of-two - some values have been omitted.)
	ACPI_AS_MEMORY     = 0,
	ACPI_AS_IO         = 1,
	ACPI_AS_PCI_CONFIG = 2,
	ACPI_AS_SMBUS      = 4
};

// address of a struct or register
struct AcpiGenericAddress
{
	u8 addressSpaceId;
	u8 registerBitWidth;
	u8 registerBitOffset;
	u8 accessSize;
	u64 address;
};

#pragma pack(pop)

extern bool acpi_Init();
extern void acpi_Shutdown();

extern const AcpiTable* acpi_GetTable(const char* signature);

#endif	// #ifndef INCLUDED_ACPI
