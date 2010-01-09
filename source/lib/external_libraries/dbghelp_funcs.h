/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

FUNC(BOOL, SymInitializeW, (__in HANDLE hProcess, __in_opt PCWSTR UserSearchPath, __in BOOL fInvadeProcess))
FUNC(DWORD, SymGetOptions, (VOID))
FUNC(DWORD, SymSetOptions, (__in DWORD SymOptions))
FUNC(DWORD64, SymGetModuleBase64, (__in HANDLE hProcess, __in DWORD64 qwAddr))
FUNC(BOOL, SymFromAddrW, (__in HANDLE hProcess, __in DWORD64 Address, __out_opt PDWORD64 Displacement, __inout PSYMBOL_INFOW Symbol))
FUNC(BOOL, SymGetLineFromAddrW64, (__in HANDLE hProcess, __in DWORD64 qwAddr, __out PDWORD pdwDisplacement, __out PIMAGEHLP_LINEW64 Line64))
FUNC(PVOID, SymFunctionTableAccess64, (__in HANDLE hProcess, __in DWORD64 AddrBase))
FUNC(BOOL, SymGetTypeInfo, (__in HANDLE hProcess, __in DWORD64 ModBase, __in ULONG TypeId, __in IMAGEHLP_SYMBOL_TYPE_INFO GetType, __out PVOID pInfo))
FUNC(BOOL, SymFromIndexW, (__in HANDLE hProcess, __in ULONG64 BaseOfDll, __in DWORD Index, __inout PSYMBOL_INFOW Symbol))
FUNC(BOOL, SymSetContext, (__in HANDLE hProcess, __in PIMAGEHLP_STACK_FRAME StackFrame, __in_opt PIMAGEHLP_CONTEXT Context))
FUNC(BOOL, SymEnumSymbolsW, (__in HANDLE hProcess, __in ULONG64 BaseOfDll, __in_opt PCWSTR Mask, __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback, __in_opt PVOID UserContext))
FUNC(PIMAGE_NT_HEADERS, ImageNtHeader, (__in PVOID Base))
FUNC(BOOL, MiniDumpWriteDump, (IN HANDLE hProcess, IN DWORD ProcessId, IN HANDLE hFile, IN MINIDUMP_TYPE DumpType, IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL))
FUNC(BOOL, StackWalk64, (__in DWORD MachineType, __in HANDLE hProcess, __in HANDLE hThread, __inout LPSTACKFRAME64 StackFrame, __inout PVOID ContextRecord, __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress))
