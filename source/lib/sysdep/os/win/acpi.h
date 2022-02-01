/* Copyright (C) 2022 Wildfire Games.
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

/*
 * minimal subset of ACPI
 */

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

struct FADT	// signature is FACP!
{
	AcpiTable header;
	u8 unused1[40];
	u32 pmTimerPortAddress;
	u8 unused2[16];
	u16 c2Latency;	// [us]
	u16 c3Latency;	// [us]
	u8 unused3[5];
	u8 dutyWidth;
	u8 unused4[6];
	u32 flags;
	// (ACPI4 defines additional fields after this)

	bool IsDutyCycleSupported() const
	{
		return dutyWidth != 0;
	}

	bool IsC2Supported() const
	{
		return c2Latency <= 100;	// magic value specified by ACPI
	}

	bool IsC3Supported() const
	{
		return c3Latency <= 1000;	// see above
	}
};

#pragma pack(pop)

/**
 * @param signature e.g. "RSDT"
 * @return pointer to internal storage (valid until acpi_Shutdown())
 *
 * note: the first call may be slow, e.g. if a kernel-mode driver is
 * loaded. subsequent requests will be faster since tables are cached.
 **/
const AcpiTable* acpi_GetTable(const char* signature);

/**
 * invalidates all pointers returned by acpi_GetTable.
 **/
void acpi_Shutdown();

#endif	// #ifndef INCLUDED_ACPI
