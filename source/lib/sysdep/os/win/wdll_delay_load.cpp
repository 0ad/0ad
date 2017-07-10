/* Copyright (C) 2013 Wildfire Games.
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
 * DLL delay loading and notification
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wdll_delay_load.h"

#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os/win/win.h"
#include "lib/sysdep/os/win/winit.h"

WINIT_REGISTER_LATE_SHUTDOWN2(wdll_Shutdown);	// last - DLLs are unloaded here

//-----------------------------------------------------------------------------
// delay loading (modified from VC7 DelayHlp.cpp and DelayImp.h)

#if MSC_VERSION && MSC_VERSION >= 1700
// FACILITY_VISUALCPP is already defined in winerror.h in VC2012
# undef FACILITY_VISUALCPP
#endif
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#define VcppException(sev,status)  ((sev) | (FACILITY_VISUALCPP<<16) | status)

typedef IMAGE_THUNK_DATA *          PImgThunkData;
typedef const IMAGE_THUNK_DATA *    PCImgThunkData;
typedef DWORD                       RVA;

typedef struct ImgDelayDescr {
	DWORD           grAttrs;        // attributes
	RVA             rvaDLLName;     // RVA to dll name
	RVA             rvaHmod;        // RVA of module handle
	RVA             rvaIAT;         // RVA of the IAT
	RVA             rvaINT;         // RVA of the INT
	RVA             rvaBoundIAT;    // RVA of the optional bound IAT
	RVA             rvaUnloadIAT;   // RVA of optional copy of original IAT
	DWORD           dwTimeStamp;    // 0 if not bound,
	// O.W. date/time stamp of DLL bound to (Old BIND)
} ImgDelayDescr, * PImgDelayDescr;

typedef const ImgDelayDescr *   PCImgDelayDescr;

enum DLAttr {                   // Delay Load Attributes
	dlattrRva = 0x1                // RVAs are used instead of pointers
	// Having this set indicates a VC7.0
	// and above delay load descriptor.
};

enum {
	dliStartProcessing,             // used to bypass or note helper only
	dliNoteStartProcessing = dliStartProcessing,

	dliNotePreLoadLibrary,          // called just before LoadLibrary, can
	//  override w/ new HMODULE return val
	dliNotePreGetProcAddress,       // called just before GetProcAddress, can
	//  override w/ new FARPROC return value
	dliFailLoadLib,                 // failed to load library, fix it by
	//  returning a valid HMODULE
	dliFailGetProc,                 // failed to get proc address, fix it by
	//  returning a valid FARPROC
	dliNoteEndProcessing           // called after all processing is done, no
	//  no bypass possible at this point except
	//  by longjmp()/throw()/RaiseException.
};


typedef struct DelayLoadProc {
	BOOL                fImportByName;
	union {
		LPCSTR          szProcName;
		DWORD           dwOrdinal;
	};
} DelayLoadProc;


typedef struct DelayLoadInfo {
	DWORD               cb;         // size of structure
	PCImgDelayDescr     pidd;       // raw form of data (everything is there)
	FARPROC *           ppfn;       // points to address of function to load
	LPCSTR              szDll;      // name of dll
	DelayLoadProc       dlp;        // name or ordinal of procedure
	HMODULE             hmodCur;    // the hInstance of the library we have loaded
	FARPROC             pfnCur;     // the actual function that will be called
	DWORD               dwLastError;// error received (if an error notification)
} DelayLoadInfo, * PDelayLoadInfo;


typedef FARPROC (WINAPI *PfnDliHook)(unsigned dliNotify, PDelayLoadInfo  pdli);


//-----------------------------------------------------------------------------
// load notification

static WdllLoadNotify* notify_list;

void wdll_add_notify(WdllLoadNotify* notify)
{
	notify->next = notify_list;
	notify_list = notify;
}

static FARPROC WINAPI notify_hook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	if(dliNotify != dliNoteEndProcessing)
		return 0;

	for(WdllLoadNotify* n = notify_list; n; n = n->next)
		if(strncasecmp(pdli->szDll, n->dll_name, strlen(n->dll_name)) == 0)
			n->func();

	return 0;
}


//-----------------------------------------------------------------------------
// hook

// The "notify hook" gets called for every call to the
// delay load helper.  This allows a user to hook every call and
// skip the delay load helper entirely.
//
// dliNotify == {
//  dliStartProcessing |
//  dliNotePreLoadLibrary  |
//  dliNotePreGetProc |
//  dliNoteEndProcessing}
//  on this call.
//
EXTERN_C PfnDliHook __pfnDliNotifyHook2 = notify_hook;

// This is the failure hook, dliNotify = {dliFailLoadLib|dliFailGetProc}
EXTERN_C PfnDliHook __pfnDliFailureHook2 = 0;








#if !ICC_VERSION
#pragma intrinsic(strlen,memcmp,memcpy)
#endif

// utility function for calculating the index of the current import
// for all the tables (INT, BIAT, UIAT, and IAT).
inline unsigned
IndexFromPImgThunkData(PCImgThunkData pitdCur, PCImgThunkData pitdBase) {
    return (unsigned) (pitdCur - pitdBase);
    }

// C++ template utility function for converting RVAs to pointers
//
extern "C" const IMAGE_DOS_HEADER __ImageBase;

template <class X>
X PFromRva(RVA rva) {
    return X(PBYTE(&__ImageBase) + rva);
    }

// structure definitions for the list of unload records
typedef struct UnloadInfo * PUnloadInfo;
typedef struct UnloadInfo {
    PUnloadInfo     puiNext;
    PCImgDelayDescr pidd;
    } UnloadInfo;

// utility function for calculating the count of imports given the base
// of the IAT.  NB: this only works on a valid IAT!
inline unsigned
CountOfImports(PCImgThunkData pitdBase) {
    unsigned        cRet = 0;
    PCImgThunkData  pitd = pitdBase;
    while (pitd->u1.Function) {
        pitd++;
        cRet++;
        }
    return cRet;
    }

extern "C" PUnloadInfo __puiHead = 0;

struct ULI : public UnloadInfo
{
	ULI(PCImgDelayDescr pidd_)
	{
		pidd = pidd_;
		Link();
	}

	~ULI() { Unlink();	}

	void* operator new(size_t cb) {	return ::LocalAlloc(LPTR, cb); }
	void operator delete(void* pv) { ::LocalFree(pv); }

	void Unlink()
	{
		PUnloadInfo* ppui = &__puiHead;

		while (*ppui && *ppui != this)
			ppui = &((*ppui)->puiNext);
		if (*ppui == this)
			*ppui = puiNext;
	}

	void Link()
	{
		puiNext = __puiHead;
		__puiHead = this;
	}
};


// For our own internal use, we convert to the old
// format for convenience.
//
struct InternalImgDelayDescr {
    DWORD           grAttrs;        // attributes
    LPCSTR          szName;         // pointer to dll name
    HMODULE *       phmod;          // address of module handle
    PImgThunkData   pIAT;           // address of the IAT
    PCImgThunkData  pINT;           // address of the INT
    PCImgThunkData  pBoundIAT;      // address of the optional bound IAT
    PCImgThunkData  pUnloadIAT;     // address of optional copy of original IAT
    DWORD           dwTimeStamp;    // 0 if not bound,
                                    // O.W. date/time stamp of DLL bound to (Old BIND)
    };

typedef InternalImgDelayDescr *         PIIDD;
typedef const InternalImgDelayDescr *   PCIIDD;

static inline PIMAGE_NT_HEADERS WINAPI
PinhFromImageBase(HMODULE hmod) {
    return PIMAGE_NT_HEADERS(PBYTE(hmod) + PIMAGE_DOS_HEADER(hmod)->e_lfanew);
    }

static inline void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc) {
    memcpy(pitdDst, pitdSrc, CountOfImports(pitdDst) * sizeof IMAGE_THUNK_DATA);
    }

static inline DWORD WINAPI
TimeStampOfImage(PIMAGE_NT_HEADERS pinh) {
    return pinh->FileHeader.TimeDateStamp;
    }

static inline bool WINAPI
FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS pinh, HMODULE hmod) {
    return UINT_PTR(hmod) == pinh->OptionalHeader.ImageBase;
    }


extern "C" FARPROC WINAPI __delayLoadHelper2(PCImgDelayDescr pidd, FARPROC* ppfnIATEntry)
{
    // Set up some data we use for the hook procs but also useful for
    // our own use
    //
    InternalImgDelayDescr   idd = {
        pidd->grAttrs,
        PFromRva<LPCSTR>(pidd->rvaDLLName),
        PFromRva<HMODULE*>(pidd->rvaHmod),
        PFromRva<PImgThunkData>(pidd->rvaIAT),
        PFromRva<PCImgThunkData>(pidd->rvaINT),
        PFromRva<PCImgThunkData>(pidd->rvaBoundIAT),
        PFromRva<PCImgThunkData>(pidd->rvaUnloadIAT),
        pidd->dwTimeStamp
        };

    DelayLoadInfo   dli = {
        sizeof(DelayLoadInfo), pidd, ppfnIATEntry, idd.szName,
        { 0 }, 0, 0, 0
	};

    if (!(idd.grAttrs & dlattrRva))
	{
        PDelayLoadInfo  rgpdli[1] = { &dli };
        RaiseException(VcppException(ERROR_SEVERITY_ERROR, ERROR_INVALID_PARAMETER), 0, 1, PULONG_PTR(rgpdli));
        return 0;
    }

    HMODULE hmod = *idd.phmod;

    // Calculate the index for the IAT entry in the import address table
    // N.B. The INT entries are ordered the same as the IAT entries so
    // the calculation can be done on the IAT side.
    //
    const unsigned  iIAT = IndexFromPImgThunkData(PCImgThunkData(ppfnIATEntry), idd.pIAT);
    const unsigned  iINT = iIAT;

    PCImgThunkData  pitd = &(idd.pINT[iINT]);

    dli.dlp.fImportByName = !IMAGE_SNAP_BY_ORDINAL(pitd->u1.Ordinal);

    if (dli.dlp.fImportByName)
        dli.dlp.szProcName = LPCSTR(PFromRva<PIMAGE_IMPORT_BY_NAME>(RVA(UINT_PTR(pitd->u1.AddressOfData)))->Name);
    else
        dli.dlp.dwOrdinal = DWORD(IMAGE_ORDINAL(pitd->u1.Ordinal));

    // Call the initial hook.  If it exists and returns a function pointer,
    // abort the rest of the processing and just return it for the call.
    //
    FARPROC pfnRet = NULL;

    if (__pfnDliNotifyHook2) {
        pfnRet = ((*__pfnDliNotifyHook2)(dliStartProcessing, &dli));

        if (pfnRet != NULL)
            goto HookBypass;
    }

    // Check to see if we need to try to load the library.
    //
    if (hmod == 0) {
        if (__pfnDliNotifyHook2) {
            hmod = HMODULE(((*__pfnDliNotifyHook2)(dliNotePreLoadLibrary, &dli)));
            }
        if (hmod == 0) {
            hmod = ::LoadLibraryA(dli.szDll);
            }
        if (hmod == 0) {
            dli.dwLastError = ::GetLastError();
            if (__pfnDliFailureHook2) {
                // when the hook is called on LoadLibrary failure, it will
                // return 0 for failure and an hmod for the lib if it fixed
                // the problem.
                //
                hmod = HMODULE((*__pfnDliFailureHook2)(dliFailLoadLib, &dli));
                }

            if (hmod == 0) {
                PDelayLoadInfo  rgpdli[1] = { &dli };

                RaiseException(VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND),
                    0, 1, PULONG_PTR(rgpdli));

                // If we get to here, we blindly assume that the handler of the exception
                // has magically fixed everything up and left the function pointer in
                // dli.pfnCur.
                //
                return dli.pfnCur;
                }
            }

        // Store the library handle.  If it is already there, we infer
        // that another thread got there first, and we need to do a
        // FreeLibrary() to reduce the refcount
        //
        HMODULE hmodT = HMODULE(InterlockedExchangePointer((PVOID *) idd.phmod, PVOID(hmod)));
        if (hmodT != hmod) {
            // add lib to unload list if we have unload data
            if (pidd->rvaUnloadIAT) {
                new ULI(pidd);
                }
            }
        else {
            ::FreeLibrary(hmod);
            }

        }

    // Go for the procedure now.
    //
    dli.hmodCur = hmod;
    if (__pfnDliNotifyHook2) {
        pfnRet = (*__pfnDliNotifyHook2)(dliNotePreGetProcAddress, &dli);
        }
    if (pfnRet == 0) {
        if (pidd->rvaBoundIAT && pidd->dwTimeStamp) {
            // bound imports exist...check the timestamp from the target image
            //
            PIMAGE_NT_HEADERS   pinh(PinhFromImageBase(hmod));

            if (pinh->Signature == IMAGE_NT_SIGNATURE &&
                TimeStampOfImage(pinh) == idd.dwTimeStamp &&
                FLoadedAtPreferredAddress(pinh, hmod)) {

                // Everything is good to go, if we have a decent address
                // in the bound IAT!
                //
                pfnRet = FARPROC(UINT_PTR(idd.pBoundIAT[iIAT].u1.Function));
                if (pfnRet != 0) {
                    goto SetEntryHookBypass;
                    }
                }
            }

        pfnRet = ::GetProcAddress(hmod, dli.dlp.szProcName);
        }

    if (pfnRet == 0) {
        dli.dwLastError = ::GetLastError();
        if (__pfnDliFailureHook2) {
            // when the hook is called on GetProcAddress failure, it will
            // return 0 on failure and a valid proc address on success
            //
            pfnRet = (*__pfnDliFailureHook2)(dliFailGetProc, &dli);
            }
        if (pfnRet == 0) {
            PDelayLoadInfo  rgpdli[1] = { &dli };

            RaiseException(VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND),
                0, 1, PULONG_PTR(rgpdli));

            // If we get to here, we blindly assume that the handler of the exception
            // has magically fixed everything up and left the function pointer in
            // dli.pfnCur.
            //
            pfnRet = dli.pfnCur;
            }
        }

SetEntryHookBypass:
    *ppfnIATEntry = pfnRet;

HookBypass:
    if (__pfnDliNotifyHook2) {
        dli.dwLastError = 0;
        dli.hmodCur = hmod;
        dli.pfnCur = pfnRet;
        (*__pfnDliNotifyHook2)(dliNoteEndProcessing, &dli);
        }
    return pfnRet;
    }


static void UnloadAllDlls()
{
	PUnloadInfo pui;

	// free all DLLs (avoid BoundsChecker warning)
	while((pui = __puiHead) != 0)
		if(pui->pidd->rvaUnloadIAT)
		{
			PCImgDelayDescr pidd = pui->pidd;
			HMODULE* phmod = PFromRva<HMODULE*>(pidd->rvaHmod);
			HMODULE hmod = *phmod;

			OverlayIAT(
			PFromRva<PImgThunkData>(pidd->rvaIAT),
			PFromRva<PCImgThunkData>(pidd->rvaUnloadIAT)
			);
			::FreeLibrary(hmod);
			*phmod = NULL;

			delete (ULI*)pui; // changes __puiHead!
		}
}

//-----------------------------------------------------------------------------

static Status wdll_Shutdown()
{
	UnloadAllDlls();
	return INFO::OK;
}
