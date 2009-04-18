/* Copyright (C) 2009 Wildfire Games.
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

/*
 * wrapper for Windows Management Instrumentation
 */

#include "precompiled.h"
#include "wmi.h"

#include <wbemidl.h>

#include "lib/module_init.h"

#pragma comment(lib, "wbemuuid.lib")


static IWbemServices* pSvc;

_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));

static ModuleInitState initState;

static LibError Init()
{
	if(!ModuleShouldInitialize(&initState))
		return INFO::SKIPPED;

	HRESULT hr;

	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if(FAILED(hr))
		WARN_RETURN(ERR::_1);

	hr = CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE, 0);
	if(FAILED(hr))
		WARN_RETURN(ERR::_2);

	{
		IWbemLocatorPtr pLoc = 0;
		hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
		if(FAILED(hr))
			WARN_RETURN(ERR::_3);

		hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), 0, 0, 0, 0, 0, 0, &pSvc);
		if(FAILED(hr))
			WARN_RETURN(ERR::_4);
	}

	hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE);
	if(FAILED(hr))
		WARN_RETURN(ERR::_5);

	return INFO::OK;
}


void wmi_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	pSvc->Release();

	// note: don't shut down COM because other modules may still be using it.
	//CoUninitialize();
}


LibError wmi_GetClass(const char* className, WmiMap& wmiMap)
{
	Init();

	IEnumWbemClassObjectPtr pEnum = 0;
	char query[200];
	sprintf_s(query, ARRAY_SIZE(query), "SELECT * FROM %s", className);
	HRESULT hr = pSvc->ExecQuery(L"WQL", _bstr_t(query), WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnum);
	if(FAILED(hr))
		WARN_RETURN(ERR::FAIL);
	debug_assert(pEnum);

	for(;;)
	{
		IWbemClassObjectPtr pObj = 0;
		ULONG numReturned = 0;
		hr = pEnum->Next((LONG)WBEM_INFINITE, 1, &pObj, &numReturned);
		if(FAILED(hr))
			WARN_RETURN(ERR::FAIL);
		if(numReturned == 0)
			break;
		debug_assert(pEnum);

		pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
		for(;;)
		{
			BSTR name;
			VARIANT value;
			VariantInit(&value);
			if(pObj->Next(0, &name, &value, 0, 0) != WBEM_S_NO_ERROR)
			{
				SysFreeString(name);
				break;
			}
			wmiMap[name] = value;
			SysFreeString(name);
		}
	}

	return INFO::OK;
}
