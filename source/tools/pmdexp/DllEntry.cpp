/**********************************************************************
 *<
	FILE: DllEntry.cpp

	DESCRIPTION: Contains the Dll Entry stuff

	CREATED BY: 

	HISTORY: 

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/
#include "PMDExp.h"
#include "PSAExp.h"
#include "PSProp.h"
#include "MaxInc.h"


#define PMDEXP_CLASS_ID	Class_ID(0x71d92656, 0x136330c5)
#define PSAEXP_CLASS_ID	Class_ID(0x6cf86c73, 0x54e0844)

HINSTANCE hInstance;
static int controlsInit = FALSE;

TCHAR* GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance) {
		if (!LoadString(hInstance, id, buf, sizeof(buf))) {
			return NULL;
		} 
		return buf;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
// PMDExpClassDesc: required class to expose PMDExp to MAX
class PMDExpClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new PMDExp(); }
	const TCHAR *	ClassName() { return GetString(IDS_PSA_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return PMDEXP_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("PMDExp"); }	
	HINSTANCE		HInstance() { return hInstance; }		

};

static PMDExpClassDesc PMDExpDesc;
ClassDesc2* GetPMDExpDesc() { return &PMDExpDesc; }

//////////////////////////////////////////////////////////////////////////////////////
// PSAExpClassDesc: required class to expose PSAExp to MAX
class PSAExpClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new PSAExp(); }
	const TCHAR *	ClassName() { return GetString(IDS_PSA_CLASS_NAME); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return PSAEXP_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("PSAExp"); }	
	HINSTANCE		HInstance() { return hInstance; }		

};

static PSAExpClassDesc PSAExpDesc;
ClassDesc2* GetPSAExpDesc() { return &PSAExpDesc; }




BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	hInstance = hinstDLL;				// Hang on to this DLL's instance handle.

	if (!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);	// Initialize MAX's custom controls
		InitCommonControls();			// Initialize Win95 controls
	}
			
	return (TRUE);
}


__declspec(dllexport) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

__declspec(dllexport) int LibNumberClasses()
{
	return 3;
}

__declspec(dllexport) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
		case 0: return GetPMDExpDesc();
		case 1: return GetPSAExpDesc();
		case 2: return GetPSPropDesc();
		default: return 0;
	}
}

__declspec(dllexport) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}


