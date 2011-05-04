#include "precompiled.h"
#include "lib/sysdep/os/win/wfirmware.h"

#include "lib/sysdep/os/win/wutil.h"

namespace wfirmware {

TableIds GetTableIDs(Provider provider)
{
	WUTIL_FUNC(pEnumSystemFirmwareTables, UINT, (DWORD, PVOID, DWORD));
	WUTIL_IMPORT_KERNEL32(EnumSystemFirmwareTables, pEnumSystemFirmwareTables);
	if(!pEnumSystemFirmwareTables)
		return TableIds();

	const size_t tableIdsSize = pEnumSystemFirmwareTables(provider, 0, 0);
	ENSURE(tableIdsSize != 0);
	ENSURE(tableIdsSize % sizeof(TableId) == 0);
	TableIds tableIDs(DivideRoundUp(tableIdsSize, sizeof(TableId)), 0);

	const size_t bytesWritten = pEnumSystemFirmwareTables(provider, &tableIDs[0], (DWORD)tableIdsSize);
	ENSURE(bytesWritten == tableIdsSize);

	return tableIDs;
}


Table GetTable(Provider provider, TableId tableId)
{
	WUTIL_FUNC(pGetSystemFirmwareTable, UINT, (DWORD, DWORD, PVOID, DWORD));
	WUTIL_IMPORT_KERNEL32(GetSystemFirmwareTable, pGetSystemFirmwareTable);
	if(!pGetSystemFirmwareTable)
		return Table();

	const size_t tableSize = pGetSystemFirmwareTable(provider, tableId, 0, 0);
	if(tableSize == 0)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);
		return Table();
	}

	Table table(tableSize, 0);
	const size_t bytesWritten = pGetSystemFirmwareTable(provider, tableId, &table[0], (DWORD)tableSize);
	ENSURE(bytesWritten == tableSize);

	return table;
}

}	// namespace wfirmware
