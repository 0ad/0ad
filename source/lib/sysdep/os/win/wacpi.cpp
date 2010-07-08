#include "precompiled.h"
#include "lib/sysdep/os/win/wacpi.h"

#include "lib/byte_order.h"
#include "lib/sysdep/os/win/wutil.h"


static const DWORD provider = FOURCC_BE('A','C','P','I');

std::vector<u32> wacpi_TableIDs()
{
	std::vector<u32> tableIDs;

	WUTIL_FUNC(pEnumSystemFirmwareTables, UINT, (DWORD, PVOID, DWORD));
	WUTIL_IMPORT_KERNEL32(EnumSystemFirmwareTables, pEnumSystemFirmwareTables);
	if(!pEnumSystemFirmwareTables)
		return tableIDs;	// empty list

	const UINT bufSize = pEnumSystemFirmwareTables(provider, 0, 0);
	debug_assert(bufSize != 0);
	debug_assert(bufSize % sizeof(DWORD) == 0);
	tableIDs.resize(DivideRoundUp((size_t)bufSize, sizeof(DWORD)));

	const UINT ret = pEnumSystemFirmwareTables(provider, &tableIDs[0], bufSize);
	debug_assert(ret == bufSize);

	return tableIDs;
}


std::vector<u8> wacpi_GetTable(u32 id)
{
	std::vector<u8> table;

	WUTIL_FUNC(pGetSystemFirmwareTable, UINT, (DWORD, DWORD, PVOID, DWORD));
	WUTIL_IMPORT_KERNEL32(GetSystemFirmwareTable, pGetSystemFirmwareTable);
	if(!pGetSystemFirmwareTable)
		return table;	// empty table

	const UINT bufSize = pGetSystemFirmwareTable(provider, id, 0, 0);
	debug_assert(bufSize != 0);
	table.resize(bufSize);

	const UINT ret = pGetSystemFirmwareTable(provider, id, &table[0], bufSize);
	debug_assert(ret == bufSize);

	return table;
}
