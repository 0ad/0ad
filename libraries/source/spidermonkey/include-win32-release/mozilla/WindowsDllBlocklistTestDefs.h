/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This file was auto-generated from e:/0ad/libraries/source/spidermonkey/mozjs-78.6.0/mozglue/dllservices/WindowsDllBlocklistDefs.in by gen_dll_blocklist_data.py.  */

#ifndef mozilla_WindowsDllBlocklistTestDefs_h
#define mozilla_WindowsDllBlocklistTestDefs_h

#include "mozilla/WindowsDllBlocklistCommon.h"

DLL_BLOCKLIST_DEFINITIONS_BEGIN

#if defined(ENABLE_TESTS)
  DLL_BLOCKLIST_ENTRY("testdllblocklist_allowbyversion.dll", MAKE_VERSION(5, 5, 5, 5))
#endif  // defined(ENABLE_TESTS)
#if defined(ENABLE_TESTS)
  DLL_BLOCKLIST_ENTRY("testdllblocklist_matchbyname.dll", DllBlockInfo::ALL_VERSIONS)
#endif  // defined(ENABLE_TESTS)
#if defined(ENABLE_TESTS)
  DLL_BLOCKLIST_ENTRY("testdllblocklist_matchbyversion.dll", MAKE_VERSION(5, 5, 5, 5))
#endif  // defined(ENABLE_TESTS)
#if defined(ENABLE_TESTS)
  DLL_BLOCKLIST_ENTRY("testdllblocklist_noopentrypoint.dll", MAKE_VERSION(5, 5, 5, 5), DllBlockInfo::REDIRECT_TO_NOOP_ENTRYPOINT)
#endif  // defined(ENABLE_TESTS)

DLL_BLOCKLIST_DEFINITIONS_END

#endif  // mozilla_WindowsDllBlocklistTestDefs_h

