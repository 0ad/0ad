/**********************************************************************
 *<
	FILE: pthelp.cpp

	DESCRIPTION:  A point helper implementation

	CREATED BY: 

	HISTORY: created 14 July 1995

 *>	Copyright (c) 1995, All Rights Reserved.
 **********************************************************************/

#include "PSProp.h"


//------------------------------------------------------

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

#define AXIS_LENGTH 20.0f
#define ZFACT (float).005;

void AxisViewportRect(ViewExp *vpt, const Matrix3 &tm, float length, Rect *rect);
void DrawAxis(ViewExp *vpt, const Matrix3 &tm, float length, BOOL screenSize);
Box3 GetAxisBox(ViewExp *vpt, const Matrix3 &tm,float length,int resetTM);


//////////////////////////////////////////////////////////////////////////////////////
// PSPropClassDesc : required class to expose PSProp to MAX
class PSPropClassDesc : public ClassDesc2 
{
public:
	int 			IsPublic() { return 1; }
	void *			Create(BOOL loading = FALSE) { return new PSPropObject; }
	const TCHAR *	ClassName() { return GetString(IDS_DB_POINT_CLASS); }
	SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
	Class_ID		ClassID() { return PSPROP_CLASS_ID; }
	const TCHAR* 	Category() { return _T("PS Helpers");  }
	
	const TCHAR*	InternalName() {return _T("PSProp");}
	HINSTANCE		HInstance() {return hInstance;}			
};

PSPropClassDesc pointHelpObjDesc;
ClassDesc* GetPSPropDesc() { return &pointHelpObjDesc; }


// class variable for point class.
IObjParam *PSPropObject::ip = NULL;
PSPropObject *PSPropObject::editOb = NULL;


//HWND PSPropObject::hParams = NULL;
//IObjParam *PSPropObject::iObjParams;

//int PSPropObject::dlgShowAxis = TRUE;
//float PSPropObject::dlgAxisLength = AXIS_LENGTH;

void resetPointParams() 
{
	//PSPropObject::dlgShowAxis = TRUE;
	//PSPropObject::dlgAxisLength = AXIS_LENGTH;
}





#define PBLOCK_REF_NO	 0

// The following two enums are transfered to the istdplug.h by AG: 01/20/2002 
// in order to access the parameters for use in Spline IK Control modifier
// and the Spline IK Solver

// block IDs
//enum { pointobj_params, };

// pointobj_params IDs

// enum { 
//	pointobj_size, pointobj_centermarker, pointobj_axistripod, 
//	pointobj_cross, pointobj_box, pointobj_screensize, pointobj_drawontop };

// per instance block
static ParamBlockDesc2 pointobj_param_blk( 
	
	pointobj_params, _T("PointObjectParameters"),  0, &pointHelpObjDesc, P_AUTO_CONSTRUCT+P_AUTO_UI, PBLOCK_REF_NO,

	//rollout
	IDD_NEW_POINTPARAM, IDS_POINT_PARAMS, 0, 0, NULL,

	// params
	pointobj_size, _T("size"), TYPE_FLOAT, P_ANIMATABLE, IDS_POINT_SIZE,
		p_default, 		20.0,	
		p_ms_default,	20.0,
		p_range, 		0.0f, float(1.0E30), 
		p_ui, 			TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_POINT_SIZE, IDC_POINT_SIZESPIN, SPIN_AUTOSCALE, 
		end, 


	pointobj_centermarker, 	_T("centermarker"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_CENTERMARKER,
		p_default, 			FALSE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_MARKER, 
		end, 

	pointobj_axistripod, 	_T("axistripod"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_AXISTRIPOD,
		p_default, 			FALSE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_AXIS, 
		end, 

	pointobj_cross, 		_T("cross"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_CROSS,
		p_default, 			TRUE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_CROSS, 
		end, 

	pointobj_box, 			_T("box"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_BOX,
		p_default, 			FALSE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_BOX, 
		end, 

	pointobj_screensize,	_T("constantscreensize"), TYPE_BOOL, P_ANIMATABLE, IDS_POINT_SCREENSIZE,
		p_default, 			FALSE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_SCREENSIZE,
		end, 

	pointobj_drawontop,	    _T("drawontop"),       TYPE_BOOL, P_ANIMATABLE, IDS_POINT_DRAWONTOP,
		p_default, 			FALSE,
		p_ui, 				TYPE_SINGLECHEKBOX, 	IDC_POINT_DRAWONTOP,
		end, 
		
	end
	);





/*
INT_PTR CALLBACK PointParamProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
	PSPropObject *po = (PSPropObject*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (!po && msg!=WM_INITDIALOG) return FALSE;

	switch (msg) {
		case WM_INITDIALOG: {
			po = (PSPropObject*)lParam;
			SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
			CheckDlgButton(hWnd,IDC_SHOWAXIS,po->showAxis);
			
			ISpinnerControl *spin = 
				GetISpinner(GetDlgItem(hWnd,IDC_AXISLENGHSPIN));
			spin->SetLimits(10,1000,FALSE);
			spin->SetScale(0.1f);
			spin->SetValue(po->axisLength,FALSE);
			spin->LinkToEdit(GetDlgItem(hWnd,IDC_AXISLENGTH),EDITTYPE_FLOAT);
			ReleaseISpinner(spin);
			return FALSE;
			}

		case CC_SPINNER_CHANGE: {
			ISpinnerControl *spin = (ISpinnerControl*)lParam;
			po->axisLength = spin->GetFVal();
			po->NotifyDependents(FOREVER,PART_OBJ,REFMSG_CHANGE);
			po->iObjParams->RedrawViews(po->iObjParams->GetTime());
			break;
			}

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_SHOWAXIS:
					po->showAxis = IsDlgButtonChecked(hWnd,IDC_SHOWAXIS);
					po->NotifyDependents(FOREVER,PART_OBJ,REFMSG_CHANGE);
					po->iObjParams->RedrawViews(po->iObjParams->GetTime());
					break;
				}
			break;
		
		default:
			return FALSE;
		}
	
	return TRUE;
	} 
*/

void PSPropObject::BeginEditParams(
		IObjParam *ip, ULONG flags,Animatable *prev)
	{	
	this->ip = ip;
	editOb   = this;
	pointHelpObjDesc.BeginEditParams(ip, this, flags, prev);	

	/*
	iObjParams = ip;
	if (!hParams) {
		hParams = ip->AddRollupPage( 
				hInstance, 
				MAKEINTRESOURCE(IDD_POINTPARAM),
				PointParamProc, 
				GetString(IDS_DB_PARAMETERS), 
				(LPARAM)this );
		ip->RegisterDlgWnd(hParams);
	} else {
		SetWindowLongPtr(hParams,GWLP_USERDATA,(LONG_PTR)this);
		CheckDlgButton(hParams,IDC_SHOWAXIS,showAxis);
		ISpinnerControl *spin = 
			GetISpinner(GetDlgItem(hParams,IDC_AXISLENGHSPIN));
		spin->SetValue(axisLength,FALSE);
		ReleaseISpinner(spin);
		}
		*/
	}
		
void PSPropObject::EndEditParams(
		IObjParam *ip, ULONG flags,Animatable *next)
	{	
	editOb   = NULL;
	this->ip = NULL;
	pointHelpObjDesc.EndEditParams(ip, this, flags, next);
	ClearAFlag(A_OBJ_CREATING);

	/*
	dlgShowAxis = IsDlgButtonChecked(hParams, IDC_SHOWAXIS );
	ISpinnerControl *spin = GetISpinner(GetDlgItem(hParams,IDC_AXISLENGHSPIN));
	dlgAxisLength = spin->GetFVal();
	ReleaseISpinner(spin);
	if (flags&END_EDIT_REMOVEUI) {
		ip->UnRegisterDlgWnd(hParams);
		ip->DeleteRollupPage(hParams);
		hParams = NULL;
	} else {
		SetWindowLongPtr(hParams,GWLP_USERDATA,0);
		}
	iObjParams = NULL;
	*/
	}


PSPropObject::PSPropObject()
	{	
	pointHelpObjDesc.MakeAutoParamBlocks(this);
	showAxis    = TRUE; //dlgShowAxis;
	axisLength  = 10.0f; //dlgAxisLength;
	suspendSnap = FALSE;
	SetAFlag(A_OBJ_CREATING);
	}

PSPropObject::~PSPropObject()
	{
	DeleteAllRefsFromMe();
	}

IParamArray *PSPropObject::GetParamBlock()
	{
	return (IParamArray*)pblock2;
	}

int PSPropObject::GetParamBlockIndex(int id)
	{
	if (pblock2 && id>=0 && id<pblock2->NumParams()) return id;
	else return -1;
	}


class PointHelpObjCreateCallBack: public CreateMouseCallBack {
	PSPropObject *ob;
	public:
		int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
		void SetObj(PSPropObject *obj) { ob = obj; }
	};

int PointHelpObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {	

	#ifdef _OSNAP
	if (msg == MOUSE_FREEMOVE)
	{
		#ifdef _3D_CREATE
			vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
		#else
			vpt->SnapPreview(m,m,NULL, SNAP_IN_PLANE);
		#endif
	}
	#endif

	if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
		switch(point) {
			case 0: {

				// Find the node and plug in the wire color
				ULONG handle;
				ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
				INode *node;
				node = GetCOREInterface()->GetINodeByHandle(handle);
				if (node) {
					Point3 color(0,0,1);// = GetUIColor(COLOR_POINT_OBJ);
					node->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
					}

				ob->suspendSnap = TRUE;
				#ifdef _3D_CREATE	
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_3D));
				#else	
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_PLANE));
				#endif				
				break;
				}

			case 1:
				#ifdef _3D_CREATE	
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_3D));
				#else	
					mat.SetTrans(vpt->SnapPoint(m,m,NULL,SNAP_IN_PLANE));
				#endif
				if (msg==MOUSE_POINT) {
					ob->suspendSnap = FALSE;
					return 0;
					}
				break;			
			}
	} else 
	if (msg == MOUSE_ABORT) {		
		return CREATE_ABORT;
		}
	return 1;
	}

static PointHelpObjCreateCallBack pointHelpCreateCB;

CreateMouseCallBack* PSPropObject::GetCreateMouseCallBack() {
	pointHelpCreateCB.SetObj(this);
	return(&pointHelpCreateCB);
	}

void PSPropObject::SetExtendedDisplay(int flags)
	{
	extDispFlags = flags;
	}


void PSPropObject::GetLocalBoundBox(
		TimeValue t, INode* inode, ViewExp* vpt, Box3& box ) 
	{
	Matrix3 tm = inode->GetObjectTM(t);	
	
	float size;
	int screenSize;
	pblock2->GetValue(pointobj_size, t, size, FOREVER);
	pblock2->GetValue(pointobj_screensize, t, screenSize, FOREVER);

	float zoom = 1.0f;
	if (screenSize) {
		zoom = vpt->GetScreenScaleFactor(tm.GetTrans())*ZFACT;
		}
	if (zoom==0.0f) zoom = 1.0f;

	size *= zoom;
	box =  Box3(Point3(0,0,0), Point3(0,0,0));
	box += Point3(size*0.5f,  0.0f, 0.0f);
	box += Point3( 0.0f, size*0.5f, 0.0f);
	box += Point3( 0.0f, 0.0f, size*0.5f);
	box += Point3(-size*0.5f,   0.0f,  0.0f);
	box += Point3(  0.0f, -size*0.5f,  0.0f);
	box += Point3(  0.0f,  0.0f, -size*0.5f);

	box.EnlargeBy(10.0f/zoom);

	/*
	if (showAxis)
		box = GetAxisBox(vpt,tm,showAxis?axisLength:0.0f, TRUE);
	else
		box = Box3(Point3(0,0,0), Point3(0,0,0));
	*/
	}

void PSPropObject::GetWorldBoundBox(
		TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
	{
	Matrix3 tm;
	tm = inode->GetObjectTM(t);
	Box3 lbox;

	GetLocalBoundBox(t, inode, vpt, lbox);
	box = Box3(tm.GetTrans(), tm.GetTrans());
	for (int i=0; i<8; i++) {
		box += lbox * tm;
		}
	/*
	if(!(extDispFlags & EXT_DISP_ZOOM_EXT) && showAxis)
		box = GetAxisBox(vpt,tm,showAxis?axisLength:0.0f, FALSE);
	else
		box = Box3(tm.GetTrans(), tm.GetTrans());
		*/
	}


void PSPropObject::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
	{
	if(suspendSnap)
		return;

	Matrix3 tm = inode->GetObjectTM(t);	
	GraphicsWindow *gw = vpt->getGW();	
	gw->setTransform(tm);

	Matrix3 invPlane = Inverse(snap->plane);

	// Make sure the vertex priority is active and at least as important as the best snap so far
	if(snap->vertPriority > 0 && snap->vertPriority <= snap->priority) {
		Point2 fp = Point2((float)p->x, (float)p->y);
		Point2 screen2;
		IPoint3 pt3;

		Point3 thePoint(0,0,0);
		// If constrained to the plane, make sure this point is in it!
		if(snap->snapType == SNAP_2D || snap->flags & SNAP_IN_PLANE) {
			Point3 test = thePoint * tm * invPlane;
			if(fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				return;
			}
		gw->wTransPoint(&thePoint,&pt3);
		screen2.x = (float)pt3.x;
		screen2.y = (float)pt3.y;

		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);
		if(len <= snap->strength) {
			// Is this priority better than the best so far?
			if(snap->vertPriority < snap->priority) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			else
			if(len < snap->bestDist) {
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
				}
			}
		}
	}




int PSPropObject::DrawAndHit(TimeValue t, INode *inode, ViewExp *vpt)
	{
	float size;
	int centerMarker, axisTripod, cross, box, screenSize, drawOnTop;

//	Color color = inode->GetWireColor();	

	Interval ivalid = FOREVER;
	pblock2->GetValue(pointobj_size, t,         size, ivalid);
	pblock2->GetValue(pointobj_centermarker, t, centerMarker, ivalid);
	pblock2->GetValue(pointobj_axistripod, t,   axisTripod, ivalid);
	pblock2->GetValue(pointobj_cross, t,        cross, ivalid);
	pblock2->GetValue(pointobj_box, t,          box, ivalid);
	pblock2->GetValue(pointobj_screensize, t,   screenSize, ivalid);
	pblock2->GetValue(pointobj_drawontop, t,    drawOnTop, ivalid);
	
	Matrix3 tm(1);
	Point3 pt(0,0,0);
	Point3 pts[5];

	vpt->getGW()->setTransform(tm);	
	tm = inode->GetObjectTM(t);

	int limits = vpt->getGW()->getRndLimits();
	if (drawOnTop) vpt->getGW()->setRndLimits(limits & ~GW_Z_BUFFER);

	if (inode->Selected()) {
		vpt->getGW()->setColor( TEXT_COLOR, GetUIColor(COLOR_SELECTION) );
		vpt->getGW()->setColor( LINE_COLOR, GetUIColor(COLOR_SELECTION) );
	} else if (!inode->IsFrozen() && !inode->Dependent()) {
		//vpt->getGW()->setColor( TEXT_COLOR, GetUIColor(COLOR_POINT_AXES) );
		//vpt->getGW()->setColor( LINE_COLOR, GetUIColor(COLOR_POINT_AXES) );		
		vpt->getGW()->setColor( TEXT_COLOR, Color(0,0,1));
		vpt->getGW()->setColor( LINE_COLOR, Color(0,0,1));
		}	

	if (axisTripod) {
		DrawAxis(vpt, tm, size, screenSize);
		}
	
	size *= 0.5f;

	float zoom = vpt->GetScreenScaleFactor(tm.GetTrans())*ZFACT;
	if (screenSize) {
		tm.Scale(Point3(zoom,zoom,zoom));
		}

	vpt->getGW()->setTransform(tm);

	if (!inode->IsFrozen() && !inode->Dependent() && !inode->Selected()) {
		//vpt->getGW()->setColor(LINE_COLOR, GetUIColor(COLOR_POINT_OBJ));
		vpt->getGW()->setColor( LINE_COLOR, Color(0,0,1));
		}

	if (centerMarker) {		
		vpt->getGW()->marker(&pt,X_MRKR);
		}

	if (cross) {
		// X
		pts[0] = Point3(-size, 0.0f, 0.0f); pts[1] = Point3(size, 0.0f, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		// Y
		pts[0] = Point3(0.0f, -size, 0.0f); pts[1] = Point3(0.0f, size, 0.0f);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		// Z
		pts[0] = Point3(0.0f, 0.0f, -size); pts[1] = Point3(0.0f, 0.0f, size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
		}

	if (box) {

		// Make the box half the size
		size = size * 0.5f;

		// Bottom
		pts[0] = Point3(-size, -size, -size); 
		pts[1] = Point3(-size,  size, -size);
		pts[2] = Point3( size,  size, -size);
		pts[3] = Point3( size, -size, -size);
		vpt->getGW()->polyline(4, pts, NULL, NULL, TRUE, NULL);

		// Top
		pts[0] = Point3(-size, -size,  size); 
		pts[1] = Point3(-size,  size,  size);
		pts[2] = Point3( size,  size,  size);
		pts[3] = Point3( size, -size,  size);
		vpt->getGW()->polyline(4, pts, NULL, NULL, TRUE, NULL);

		// Sides
		pts[0] = Point3(-size, -size, -size); 
		pts[1] = Point3(-size, -size,  size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3(-size,  size, -size); 
		pts[1] = Point3(-size,  size,  size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3( size,  size, -size); 
		pts[1] = Point3( size,  size,  size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

		pts[0] = Point3( size, -size, -size); 
		pts[1] = Point3( size, -size,  size);
		vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);
		}

	vpt->getGW()->setRndLimits(limits);

	return 1;
	}

int PSPropObject::HitTest(
		TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) 
	{
	Matrix3 tm(1);	
	HitRegion hitRegion;
	DWORD	savedLimits;
	Point3 pt(0,0,0);

	vpt->getGW()->setTransform(tm);
	GraphicsWindow *gw = vpt->getGW();	
	Material *mtl = gw->getMaterial();

   	tm = inode->GetObjectTM(t);		
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits())|GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	DrawAndHit(t, inode, vpt);

/*
	if (showAxis) {
		DrawAxis(vpt,tm,axisLength,screenSize);
		}
	vpt->getGW()->setTransform(tm);
	vpt->getGW()->marker(&pt,X_MRKR);
*/

	gw->setRndLimits(savedLimits);
	
	if((hitRegion.type != POINT_RGN) && !hitRegion.crossing)
		return TRUE;
	return gw->checkHitCode();
	}



int PSPropObject::Display(
		TimeValue t, INode* inode, ViewExp *vpt, int flags) 
	{
	DrawAndHit(t, inode, vpt);
	/*
	Matrix3 tm(1);
	Point3 pt(0,0,0);

	vpt->getGW()->setTransform(tm);	
	tm = inode->GetObjectTM(t);		
	
	if (showAxis) {
		DrawAxis(vpt,tm,axisLength,inode->Selected(),inode->IsFrozen());
		}


	vpt->getGW()->setTransform(tm);
	if(!inode->IsFrozen())
		vpt->getGW()->setColor(LINE_COLOR,GetUIColor(COLOR_POINT_OBJ));
	vpt->getGW()->marker(&pt,X_MRKR);
	*/

	return(0);
	}



//
// Reference Managment:
//

// This is only called if the object MAKES references to other things.
RefResult PSPropObject::NotifyRefChanged(
		Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID, RefMessage message ) 
    {
	switch (message) {
		case REFMSG_CHANGE:
			if (editOb==this) InvalidateUI();
			break;
		}
	return(REF_SUCCEED);
	}

void PSPropObject::InvalidateUI()
	{
	pointobj_param_blk.InvalidateUI(pblock2->LastNotifyParamID());
	}

Interval PSPropObject::ObjectValidity(TimeValue t)
	{
	float size;
	int centerMarker, axisTripod, cross, box, screenSize, drawOnTop;

	Interval ivalid = FOREVER;
	pblock2->GetValue(pointobj_size, t,         size, ivalid);
	pblock2->GetValue(pointobj_centermarker, t, centerMarker, ivalid);
	pblock2->GetValue(pointobj_axistripod, t,   axisTripod, ivalid);
	pblock2->GetValue(pointobj_cross, t,        cross, ivalid);
	pblock2->GetValue(pointobj_box, t,          box, ivalid);
	pblock2->GetValue(pointobj_screensize, t,   screenSize, ivalid);
	pblock2->GetValue(pointobj_drawontop, t,    drawOnTop, ivalid);

	return ivalid;
	}

ObjectState PSPropObject::Eval(TimeValue t)
	{
	return ObjectState(this);
	}

RefTargetHandle PSPropObject::Clone(RemapDir& remap) 
	{
	PSPropObject* newob = new PSPropObject();	
	newob->showAxis = showAxis;
	newob->axisLength = axisLength;
	newob->ReplaceReference(0, pblock2->Clone(remap));
	BaseClone(this, newob, remap);
	return(newob);
	}


void PSPropObject::UpdateParamblockFromVars()
	{
	SuspendAnimate();
	AnimateOff();
	
	pblock2->SetValue(pointobj_size, TimeValue(0), axisLength);
	pblock2->SetValue(pointobj_centermarker, TimeValue(0), TRUE);
	pblock2->SetValue(pointobj_axistripod, TimeValue(0), showAxis);
	pblock2->SetValue(pointobj_cross, TimeValue(0), FALSE);
	pblock2->SetValue(pointobj_box, TimeValue(0), FALSE);
	pblock2->SetValue(pointobj_screensize, TimeValue(0), TRUE);

	ResumeAnimate();
	}


class PointHelperPostLoadCallback : public PostLoadCallback {
	public:
		PSPropObject *pobj;

		PointHelperPostLoadCallback(PSPropObject *p) {pobj=p;}
		void proc(ILoad *iload) {
			pobj->UpdateParamblockFromVars();
			}
	};

#define SHOW_AXIS_CHUNK				0x0100
#define AXIS_LENGTH_CHUNK			0x0110
#define POINT_HELPER_R4_CHUNKID		0x0120 // new version of point helper for R4 (updated to use PB2)

IOResult PSPropObject::Load(ILoad *iload)
	{
	ULONG nb;
	IOResult res = IO_OK;
	BOOL oldVersion = TRUE;

	while (IO_OK==(res=iload->OpenChunk())) {
		switch (iload->CurChunkID()) {
			
			case SHOW_AXIS_CHUNK:
				res = iload->Read(&showAxis,sizeof(showAxis),&nb);
				break;

			case AXIS_LENGTH_CHUNK:
				res = iload->Read(&axisLength,sizeof(axisLength),&nb);
				break;

			case POINT_HELPER_R4_CHUNKID:
				oldVersion = FALSE;
				break;
			}

		res = iload->CloseChunk();
		if (res!=IO_OK)  return res;
		}
	
	if (oldVersion) {
		iload->RegisterPostLoadCallback(new PointHelperPostLoadCallback(this));
		}

	return IO_OK;
	}

IOResult PSPropObject::Save(ISave *isave)
	{
	/*
	isave->BeginChunk(SHOW_AXIS_CHUNK);
	isave->Write(&showAxis,sizeof(showAxis),&nb);
	isave->EndChunk();

	isave->BeginChunk(AXIS_LENGTH_CHUNK);
	isave->Write(&axisLength,sizeof(axisLength),&nb);
	isave->EndChunk();
	*/

	isave->BeginChunk(POINT_HELPER_R4_CHUNKID);
	isave->EndChunk();
	
	return IO_OK;
	}


/*--------------------------------------------------------------------*/
// 
// Stole this from scene.cpp
// Probably couldn't hurt to make an API...
//
//


void Text( ViewExp *vpt, TCHAR *str, Point3 &pt )
	{	
	vpt->getGW()->text( &pt, str );	
	}

static void DrawAnAxis( ViewExp *vpt, Point3 axis )
	{
	Point3 v1, v2, v[3];	
	v1 = axis * (float)0.9;
	if ( axis.x != 0.0 || axis.y != 0.0 ) {
		v2 = Point3( axis.y, -axis.x, axis.z ) * (float)0.1;
	} else {
		v2 = Point3( axis.x, axis.z, -axis.y ) * (float)0.1;
		}
	
	v[0] = Point3(0.0,0.0,0.0);
	v[1] = axis;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );	
	v[0] = axis;
	v[1] = v1+v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );
	v[0] = axis;
	v[1] = v1-v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );
	}


void DrawAxis(ViewExp *vpt, const Matrix3 &tm, float length, BOOL screenSize)
	{
	Matrix3 tmn = tm;
	float zoom;

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	
	if (screenSize) {
		tmn.Scale( Point3(zoom,zoom,zoom) );
		}
	vpt->getGW()->setTransform( tmn );		

	Text( vpt, _T("x"), Point3(length,0.0f,0.0f) ); 
	DrawAnAxis( vpt, Point3(length,0.0f,0.0f) );	
	
	Text( vpt, _T("y"), Point3(0.0f,length,0.0f) ); 
	DrawAnAxis( vpt, Point3(0.0f,length,0.0f) );	
	
	Text( vpt, _T("z"), Point3(0.0f,0.0f,length) ); 
	DrawAnAxis( vpt, Point3(0.0f,0.0f,length) );
	}



//--- RB 7/17/2000: the code below seems to be unused ---------------------------------------------------


Box3 GetAxisBox(ViewExp *vpt, const Matrix3 &tm,float length,int resetTM)
	{
	Matrix3 tmn = tm;
	Box3 box;
	float zoom;

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	if (zoom==0.0f) zoom = 1.0f;
//	tmn.Scale(Point3(zoom,zoom,zoom));
	length *= zoom;
	if(resetTM)
		tmn.IdentityMatrix();

	box += Point3(0.0f,0.0f,0.0f) * tmn;
	box += Point3(length,0.0f,0.0f) * tmn;
	box += Point3(0.0f,length,0.0f) * tmn;
	box += Point3(0.0f,0.0f,length) * tmn;	
	box += Point3(-length/5.f,0.0f,0.0f) * tmn;
	box += Point3(0.0f,-length/5.f,0.0f) * tmn;
	box += Point3(0.0f,0.0f,-length/5.0f) * tmn;
	box.EnlargeBy(10.0f/zoom);
	return box;
	}


inline void EnlargeRectIPoint3( RECT *rect, IPoint3& pt )
	{
	if ( pt.x < rect->left )   rect->left   = pt.x;
	if ( pt.x > rect->right )  rect->right  = pt.x;
	if ( pt.y < rect->top )    rect->top    = pt.y;
	if ( pt.y > rect->bottom ) rect->bottom = pt.y;
	}

// This is a guess - need to find real w/h.
#define FONT_HEIGHT	11
#define FONT_WIDTH  9	


static void AxisRect( GraphicsWindow *gw, Point3 axis, Rect *rect )
	{
	Point3 v1, v2, v;	
	IPoint3 iv;
	v1 = axis * (float)0.9;
	if ( axis.x != 0.0 || axis.y != 0.0 ) {
		v2 = Point3( axis.y, -axis.x, axis.z ) * (float)0.1;
	} else {
		v2 = Point3( axis.x, axis.z, -axis.y ) * (float)0.1;
		}
	v = axis;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);

	iv.x += FONT_WIDTH;
	iv.y -= FONT_HEIGHT;
	EnlargeRectIPoint3( rect, iv);

	v = v1+v2;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);
	v = v1-v2;
	gw->wTransPoint( &v, &iv );
	EnlargeRectIPoint3( rect, iv);
	}


void AxisViewportRect(ViewExp *vpt, const Matrix3 &tm, float length, Rect *rect)
	{
	Matrix3 tmn = tm;
	float zoom;
	IPoint3 wpt;
	Point3 pt;
	GraphicsWindow *gw = vpt->getGW();

	// Get width of viewport in world units:  --DS
	zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;
	
	tmn.Scale( Point3(zoom,zoom,zoom) );
	gw->setTransform( tmn );	
	pt = Point3(0.0f, 0.0f, 0.0f);
	gw->wTransPoint( &pt, &wpt );
	rect->left = rect->right  = wpt.x;
	rect->top  = rect->bottom = wpt.y;

	AxisRect( gw, Point3(length,0.0f,0.0f),rect );	
	AxisRect( gw, Point3(0.0f,length,0.0f),rect );	
	AxisRect( gw, Point3(0.0f,0.0f,length),rect );	

	rect->right  += 2;
	rect->bottom += 2;
	rect->left   -= 2;
	rect->top    -= 2;
	}



