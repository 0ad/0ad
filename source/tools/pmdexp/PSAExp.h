#ifndef __PSAEXP_H
#define __PSAEXP_H

// necessary includes
#include "MaxInc.h"
#include <vector>

// necessary declarations
class ExpMesh;
class ExpSkeleton;
class CModelDef;
class CSkeleton;

/////////////////////////////////////////////////////////////////
// PSAExp:
class PSAExp : public SceneExport 
{
public:
	PSAExp();
	~PSAExp();		

	// standard stuff that Max requires
	int	ExtCount();						// Number of extensions supported
	const TCHAR* Ext(int n);			// Extension #n (i.e. "3DS")
	const TCHAR* LongDesc();			// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR* ShortDesc();			// Short ASCII description (i.e. "3D Studio")
	const TCHAR* AuthorName();			// ASCII Author name
	const TCHAR* CopyrightMessage();	// ASCII Copyright message
	const TCHAR* OtherMessage1();		// Other message #1
	const TCHAR* OtherMessage2();		// Other message #2
	unsigned int Version();				// Version number * 100 (i.e. v3.01 = 301)
	void ShowAbout(HWND hWnd);			// Show DLL's "About..." box

	BOOL SupportsOptions(int ext, DWORD options);
	int	DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

private:
	// pointer to MAXs Interface object
	Interface* m_IP;
	// handle to the export parameters window
	HWND m_Params;
	// export options
	DWORD m_Options;
	// list of all skeletons found in scene
	std::vector<ExpSkeleton*> m_Skeletons;
};


#endif
