#include "PSAExp.h"
#include "ExpMesh.h"
#include "ExpSkeleton.h"

#undef PI
#include "ModelDef.h"
//#include "SkeletonAnim.h"

//////////////////////////////////////////////////////////////////////
// PSAExp constructor
PSAExp::PSAExp()
{
}

//////////////////////////////////////////////////////////////////////
// PSAExp destructor
PSAExp::~PSAExp() 
{
}

//////////////////////////////////////////////////////////////////////
// ExtCount: return the number of file name extensions supported 
// by the plug-in. 
int PSAExp::ExtCount()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////
// Ext: return the ith file name extension 
const TCHAR* PSAExp::Ext(int n)
{		
	return _T("psa");
}

//////////////////////////////////////////////////////////////////////
// LongDesc: return long ASCII description 
const TCHAR* PSAExp::LongDesc()
{
	return _T("Prometheus Skeleton Anim");
}
	
//////////////////////////////////////////////////////////////////////
// ShortDesc: return short ASCII description 
const TCHAR* PSAExp::ShortDesc() 
{			
	return _T("Prometheus Anim");
}

//////////////////////////////////////////////////////////////////////
// AuthorName: return author name
const TCHAR* PSAExp::AuthorName()
{			
	return _T("Rich Cross");
}

//////////////////////////////////////////////////////////////////////
// CopyrightMessage: return copyright message
const TCHAR* PSAExp::CopyrightMessage() 
{	
	return _T("(c) Wildfire Games 2004");
}

//////////////////////////////////////////////////////////////////////
// OtherMessage1: return some other message (or don't, in this case)
const TCHAR* PSAExp::OtherMessage1() 
{		
	return _T("");
}

//////////////////////////////////////////////////////////////////////
// OtherMessage2: return some other message (or don't, in this case)
const TCHAR* PSAExp::OtherMessage2() 
{		
	return _T("");
}

//////////////////////////////////////////////////////////////////////
// Version: return version number * 100 (i.e. v3.01 = 301)
unsigned int PSAExp::Version()
{				
	return 1;
}

//////////////////////////////////////////////////////////////////////
// ShowAbout: show an about box (or don't, in this case)
void PSAExp::ShowAbout(HWND hWnd)
{			
}

//////////////////////////////////////////////////////////////////////
// SupportsOptions: return true for each option supported by each 
// extension the exporter supports
BOOL PSAExp::SupportsOptions(int ext, DWORD options)
{
	// return TRUE to indicate export selected supported
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// DoExport: actually perform the export to the given filename
int	PSAExp::DoExport(const TCHAR *name,ExpInterface *ei,Interface *ip, BOOL suppressPrompts, DWORD options)
{
	// result of the export: assume we'll fail somewhere along the way
	BOOL res=FALSE;

	// save off the interface ptr
	m_IP=ip;

	// save off the options
	m_Options=options;
	
	// build any skeletons in MAXs heirarchy before going any further
	std::vector<ExpSkeleton*> skeletons;
	ExpSkeleton::BuildSkeletons(m_IP->GetRootNode(),skeletons);
	if (skeletons.size()>1) {
		MessageBox(GetActiveWindow(),"Found more than one skeleton in scene","Error",MB_OK);
	} else if (skeletons.size()==0) {
		MessageBox(GetActiveWindow(),"No skeletons found in scene","Error",MB_OK);
	} else {
		// build an animation from first skeleton
		ExpSkeleton* skeleton=skeletons[0];
		Interval interval=m_IP->GetAnimRange();
		CSkeletonAnimDef* anim=skeleton->BuildAnimation(interval.Start(),interval.End(),1000/30);
		try {
			CSkeletonAnimDef::Save(name,anim);
			res=TRUE;
		} catch (...) {
			res=FALSE;
		}

		MessageBox(GetActiveWindow(),res ? "Export Complete" : "Error saving model",res ? "Info" : "Error",MB_OK);
	}
			
	// clean up
	for (int i=0;i<skeletons.size();i++) {
		delete skeletons[i];
	}

	// return result
	return res;
}

