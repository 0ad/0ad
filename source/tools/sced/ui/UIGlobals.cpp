#include "precompiled.h"
#include "stdafx.h"
#include "UIGlobals.h"

void GetVersionString(char* buf)
{
	// null version in case API calls fail for some reason
	int version[4];
	version[0]=version[1]=version[2]=version[3]=0;

	// get filename of process currently running
	char filename[256];
	::GetModuleFileName(0,filename,256);

	DWORD unused;
    DWORD len=::GetFileVersionInfoSize(filename,&unused);
    if (len>0) {
		char* versioninfo=new char[len];
        GetFileVersionInfo(filename,0,len,versioninfo);

		VS_FIXEDFILEINFO* fileinfo;
		UINT size;
        if (VerQueryValue(versioninfo,"\\",(LPVOID*) &fileinfo,&size)) {
            version[0]=HIWORD(fileinfo->dwFileVersionMS);
            version[1]=LOWORD(fileinfo->dwFileVersionMS);
            version[2]=HIWORD(fileinfo->dwFileVersionLS);
            version[3]=LOWORD(fileinfo->dwFileVersionLS);
		}

		delete[] versioninfo;
	}
	sprintf(buf,"Version: %d.%d.%d.%d",version[0],version[1],version[2],version[3]);
}

void ErrorBox(const char* errstr)
{
	::MessageBox(0,errstr,"Error",MB_OK);
}