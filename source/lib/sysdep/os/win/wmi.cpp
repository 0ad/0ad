/* Copyright (c) 2011 Wildfire Games
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
 * wrapper for Windows Management Instrumentation
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wmi.h"

#include <wbemidl.h>

#include "lib/module_init.h"

#pragma comment(lib, "wbemuuid.lib")


static IWbemServices* pSvc;

_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));

static ModuleInitState initState;

static bool didInitCOM = false;

static Status Init()
{
	HRESULT hr;

	hr = CoInitialize(0);
	ENSURE(hr == S_OK || hr == S_FALSE);	// S_FALSE => already initialized

	// balance calls to CoInitialize and CoUninitialize
	if (hr == S_FALSE)
		CoUninitialize();
	else if (hr == S_OK)
		didInitCOM = true;

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
			return ERR::_4;	// NOWARN (happens if WMI service is disabled)
	}

	hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, 0, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE);
	if(FAILED(hr))
		WARN_RETURN(ERR::_5);

	return INFO::OK;
}

static void Shutdown()
{
	pSvc->Release();

	if (didInitCOM)
	{
		/* From MSDN documentation: A thread must call CoUninitialize once for each successful call
		 * it has made to the CoInitialize or CoInitializeEx function, including any call that returns
		 * S_FALSE. Only the CoUninitialize call corresponding to the CoInitialize or CoInitializeEx
		 * call that initialized the library can close it.
		 *
		 * So it should be perfectly safe to call this, since it balances out the CoInitialize in Init
		 */
		CoUninitialize();
		didInitCOM = false;
	}
}

void wmi_Shutdown()
{
	ModuleShutdown(&initState, Shutdown);
}


Status wmi_GetClassInstances(const wchar_t* className, WmiInstances& instances)
{
	RETURN_STATUS_IF_ERR(ModuleInit(&initState, Init));

	IEnumWbemClassObjectPtr pEnum = 0;
	wchar_t query[200];
	swprintf_s(query, ARRAY_SIZE(query), L"SELECT * FROM %ls", className);
	HRESULT hr = pSvc->ExecQuery(L"WQL", _bstr_t(query), WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnum);
	if(FAILED(hr))
		WARN_RETURN(ERR::FAIL);
	ENSURE(pEnum);

	for(;;)
	{
		IWbemClassObjectPtr pObj = 0;
		ULONG numReturned = 0;
		hr = pEnum->Next((LONG)WBEM_INFINITE, 1, &pObj, &numReturned);
		if(FAILED(hr))
			WARN_RETURN(ERR::FAIL);
		if(numReturned == 0)
			break;
		ENSURE(pEnum);

		WmiInstance instance;
		pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
		for(;;)
		{
			BSTR name = NULL;
			VARIANT value;
			VariantInit(&value);
			if(pObj->Next(0, &name, &value, 0, 0) != WBEM_S_NO_ERROR)
			{
				SysFreeString(name);
				break;
			}
			instance[name] = value;
			SysFreeString(name);
		}
		instances.push_back(instance);
	}

	return INFO::OK;
}
