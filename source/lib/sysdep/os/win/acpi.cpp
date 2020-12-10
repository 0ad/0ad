/* Copyright (C) 2020 Wildfire Games.
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
#include "lib/sysdep/os/win/acpi.h"

#include "lib/byte_order.h"
#include "lib/sysdep/cpu.h"
#include "lib/module_init.h"

#include "lib/sysdep/os/win/wfirmware.h"

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

static void AllocateAndCopyTables(const AcpiTable**& tables, size_t& numTables)
{
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
