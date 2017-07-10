/* Copyright (C) 2015 Wildfire Games.
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
