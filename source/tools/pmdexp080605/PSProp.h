#ifndef _PSPROP_H
#define _PSPROP_H

#include "MaxInc.h"

TCHAR *GetString(int id);

#define PSPROP_CLASS_ID	Class_ID(0x353f201d, 0x3d01408d)

extern ClassDesc* GetPSPropDesc();

class PSPropObject : public HelperObject 
{
public:			
	static IObjParam *ip;
	static PSPropObject *editOb;
	IParamBlock2 *pblock2;

	// Class vars
	/*
	static HWND hParams;
	static IObjParam *iObjParams;
	static int dlgShowAxis;
	static float dlgAxisLength;
	*/

	// Snap suspension flag (TRUE during creation only)
	BOOL suspendSnap;
				
	// Old params... these are for loading old files only. Params are now stored in pb2.
	BOOL showAxis;
	float axisLength;

	// For use by display system
 	int extDispFlags;

	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
	   PartID& partID, RefMessage message );		

	PSPropObject();
	~PSPropObject();
	
	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt);
	void SetExtendedDisplay(int flags);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	CreateMouseCallBack* GetCreateMouseCallBack();
	void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
	void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
	TCHAR *GetObjectName() {return GetString(IDS_POINT_HELPER_NAME);}

	// From Object
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s) { s = GetString(IDS_DB_POINT); }
	ObjectHandle ApplyTransform(Matrix3& matrix) {return this;}
	int CanConvertToType(Class_ID obtype) {return FALSE;}
	Object* ConvertToType(TimeValue t, Class_ID obtype) {assert(0);return NULL;}		
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box );
	int DoOwnSelectHilite()	{ return 1; }
	Interval ObjectValidity(TimeValue t);
	int UsesWireColor() {return TRUE;}

	// Animatable methods
	void DeleteThis() { delete this; }
	Class_ID ClassID() { return PSPROP_CLASS_ID; }  
	void GetClassName(TSTR& s) { s = TSTR(GetString(IDS_DB_POINTHELPER_CLASS)); }
	int IsKeyable(){ return 0;}
	int NumSubs() { return 1; }  
	Animatable* SubAnim(int i) { return pblock2; }
	TSTR SubAnimName(int i) { return TSTR(_T("Parameters"));}
	IParamArray *GetParamBlock();
	int GetParamBlockIndex(int id);
	int	NumParamBlocks() { return 1; }
	IParamBlock2* GetParamBlock(int i) { return pblock2; }
	IParamBlock2* GetParamBlockByID(short id) { return pblock2; }

	// From ref
	RefTargetHandle Clone(RemapDir& remap = NoRemap());
	IOResult Load(ILoad *iload);
	IOResult Save(ISave *isave);
	int NumRefs() {return 1;}
	RefTargetHandle GetReference(int i) {return pblock2;}
	void SetReference(int i, RefTargetHandle rtarg) {pblock2=(IParamBlock2*)rtarg;}

	// Local methods
	void InvalidateUI();
	void UpdateParamblockFromVars();
	int DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt);
};				


#endif 
