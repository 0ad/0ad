#ifndef __PMDEXP_H
#define __PMDEXP_H

// necessary includes
#include "MaxInc.h"
#include <vector>

// necessary declarations
class ExpMesh;
class ExpProp;
class ExpSkeleton;
class CModelDef;

/////////////////////////////////////////////////////////////////
// PMDExp:
class PMDExp : public SceneExport 
{
public:
	PMDExp();
	~PMDExp();		

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
	// BuildOutputList: traverse heirarchy collecting meshes/props
	void BuildOutputList(INode* node,ExpSkeleton* skeleton,
				   std::vector<ExpMesh*>& meshes,std::vector<ExpProp*>& props);
	// WeldMeshes: weld together given list of meshes; return result as a CModelDef
	CModelDef* BuildModel(std::vector<ExpMesh*>& meshes,std::vector<ExpProp*>& props,ExpSkeleton* skeleton);

	// pointer to MAXs Interface object
	Interface* m_IP;
	// handle to the export parameters window
	HWND m_Params;
	// export options
	DWORD m_Options;
};


#endif
