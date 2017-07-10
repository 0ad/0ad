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
