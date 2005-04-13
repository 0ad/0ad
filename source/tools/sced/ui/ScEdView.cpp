// ScEdView.cpp : implementation of the CScEdView class
//

#include "precompiled.h"
#include "stdafx.h"
#define _IGNORE_WGL_H_
#include "ScEd.h"

#include "ScEdDoc.h"
#include "ScEdView.h"
#include "ToolManager.h"
#include "EditorData.h"
#include "UserConfig.h"
#include "MainFrm.h"

#include "timer.h"
#include "ogl.h"
#undef _IGNORE_WGL_H_

#include "SelectObjectTool.h"
#include "Game.h"

#include "res/vfs.h"

#undef CRect // because it was redefined to PS_Rect in Overlay.h

int g_ClickMode=0;

static unsigned int GetModifierKeyFlags()
{
	unsigned int flags=0;
	if (::GetAsyncKeyState(VK_MENU) & 0x8000) flags|=TOOL_MOUSEFLAG_ALTDOWN;
	if (::GetAsyncKeyState(VK_SHIFT) & 0x8000) flags|=TOOL_MOUSEFLAG_SHIFTDOWN;
	if (::GetAsyncKeyState(VK_CONTROL) & 0x8000) flags|=TOOL_MOUSEFLAG_CTRLDOWN;
	return flags;
}


/////////////////////////////////////////////////////////////////////////////
// CScEdView

IMPLEMENT_DYNCREATE(CScEdView, CView)

BEGIN_MESSAGE_MAP(CScEdView, CView)
	//{{AFX_MSG_MAP(CScEdView)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_MESSAGE(WM_MOUSEWHEEL,OnMouseWheel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScEdView construction/destruction

CScEdView::CScEdView() : m_hGLRC(0), m_LastTickTime(-1.0f), m_LastFrameDuration(-1.0f)
{
}

CScEdView::~CScEdView()
{
}

BOOL CScEdView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = ::AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC, 
		::LoadCursor(NULL, IDC_ARROW), NULL, NULL); 
	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN; 
	return CView::PreCreateWindow(cs); 
}

/////////////////////////////////////////////////////////////////////////////
// CScEdView drawing



void CScEdView::OnDraw(CDC* pDC)
{
	HWND hWnd = GetSafeHwnd();
	HDC hDC = ::GetDC(hWnd);
	wglMakeCurrent(hDC,m_hGLRC);

	g_EditorData.OnDraw();

	SwapBuffers(hDC);
}

/////////////////////////////////////////////////////////////////////////////
// CScEdView diagnostics

#ifdef _DEBUG
void CScEdView::AssertValid() const
{
	CView::AssertValid();
}

void CScEdView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CScEdDoc* CScEdView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CScEdDoc)));
	return (CScEdDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScEdView message handlers

BOOL CScEdView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

int CScEdView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	// make sure the delay-loaded OpenGL has been loaded before calling
	// any graphical functions
	glGetError();

	// base initialisation first
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	// get device context for this window
	HDC dc=::GetDC(m_hWnd);

	// try and setup a default pixel format
	if (!SetupPixelFormat(dc)) {
		return -1;
	}

	// create context, make it current
	m_hGLRC=wglCreateContext(dc);
	wglMakeCurrent(dc,m_hGLRC);
	
	// initialise gl stuff (extensions, etc)
	oglInit();

	// check for minimum requirements
	if(!oglHaveExtension("GL_ARB_multitexture") || !oglHaveExtension("GL_ARB_texture_env_combine")) {
		const char* err="No graphics card support for multitexturing found; please visit the 0AD Forums for more information.";
		::MessageBox(0,err,"Error",MB_OK);
		exit(0);
	}


	extern void ScEd_Init();
	ScEd_Init();

	// initialise document data
	if (!g_EditorData.Init()) return -1;

	// store current mouse pos
	::GetCursorPos(&m_LastMousePos);

	return 0;
}

void CScEdView::OnDestroy() 
{
	// close down editor resources
	g_EditorData.Terminate();

	extern void ScEd_Shutdown();
	ScEd_Shutdown();
	
	// release rendering context
	if (m_hGLRC) {
		wglMakeCurrent(0,0);
		wglDeleteContext(m_hGLRC);
		m_hGLRC=0;
	}

	// base destruction
	CView::OnDestroy();
}

void CScEdView::OnSize(UINT nType, int cx, int cy) 
{
	// give base class a shout ..
	CView::OnSize(nType, cx, cy);
	
	m_Width=cx;
	m_Height=cy;
	g_Renderer.Resize(m_Width,m_Height);

	g_EditorData.OnCameraChanged();
}

bool CScEdView::SetupPixelFormat(HDC dc)
{
	int bpp=::GetDeviceCaps(dc,BITSPIXEL);
	PIXELFORMATDESCRIPTOR pfd =  {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		  PFD_SUPPORT_OPENGL |			// support OpenGL
		  PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		bpp==16 ? 16 : 24,				// 16/24 bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		bpp==16 ? 0 : 8,				// alpha bits
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		24,								// 24-bit z-buffer	
		0,								// 8-bit stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};

	int format=::ChoosePixelFormat(dc,&pfd);
	if (format==0) {
		// ack - can't choose format; bail out
		return false;
	}
	if(::SetPixelFormat(dc,format,&pfd) == false) {
		// ugh - still can't get anything; bail out
		return false;
	}

	// check we've got an accelerated format available
	if (::DescribePixelFormat(dc,format,sizeof(pfd),&pfd)) {
		if (pfd.dwFlags & PFD_GENERIC_FORMAT) {
			const char* err="No hardware accelerated graphics support found; please visit the 0AD Forums for more information.";
			::MessageBox(0,err,"Error",MB_OK);
			exit(0);
		}
	}

    return true;
}


static CPoint lastClientMousePos;

void CScEdView::OnMouseLeave(const CPoint& point)
{
}

void CScEdView::OnMouseEnter(const CPoint& point)
{
	unsigned int flags=GetModifierKeyFlags();

	// trigger left button up and right button up events as required
	if (!(::GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
		g_ToolMan.OnLButtonUp(flags,point.x,point.y);
	}
	
	if (!(::GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
		g_ToolMan.OnRButtonUp(flags,point.x,point.y);
	}
}

void CScEdView::OnMouseMove(UINT nFlags, CPoint point) 
{
	SetFocus();

	if (nFlags & MK_MBUTTON) {
		// middle mouse down .. forward move to the navicam and
		// we're done
		u32 flags=GetModifierKeyFlags();
		g_NaviCam.OnMouseMove(flags,point.x,point.y);
		return;
	}

	static bool mouseInView=false; 
	
	// get view rect 
	CRect rect; 
	GetClientRect(rect);

	// inside?	
	if(rect.PtInRect(point)) {
		// yes - previously inside?
		if (!mouseInView) {
			// nope - enable mouse capture
			SetCapture();
			// run any necessary mouse leave code
			OnMouseEnter(point);
		}
		mouseInView=true;
		// store mouse position
		lastClientMousePos=point;

		// now actually handle everything required for the actual move event

		// assume we want to update tile selection on mouse move
		bool updateSelection=true;

		// check for left mouse down on minimap following click on minimap
		if (nFlags & MK_LBUTTON) {
			if (g_ClickMode==0) {
				if (point.x>m_Width-200 && point.y>m_Height-200) {
					AdjustCameraViaMinimapClick(point);
	
					// minimap moved, so don't update selection
					updateSelection=false;
				}
			}
		}

		if (updateSelection) {
			unsigned int flags=GetModifierKeyFlags();
			g_ToolMan.OnMouseMove(flags,point.x,point.y);
		}

		// store this position as last
		m_LastMousePos=point;

	} else {
		// previously inside view?
		if (mouseInView) {
			// yes - run any necessary mouse leave code
			OnMouseLeave(point);
			// release capture
			ReleaseCapture();
		}
		// note mouse no longer in view
		mouseInView=false;
	}

}


void CScEdView::OnScreenShot()
{
	static int counter=1;
		
	// generate filename, create directory if required
	char buf[512];
	sprintf(buf,"../screenshots");
	mkdir(buf,0);
	sprintf(buf,"%s/%08d.tga",buf,counter++);

	// make context current
	HWND hWnd = GetSafeHwnd();
	HDC hDC = ::GetDC(hWnd);
	wglMakeCurrent(hDC,m_hGLRC);

	// call on editor to make screenshot
	g_EditorData.OnScreenShot(buf);
}

	
void CScEdView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	g_ClickMode=1;

	unsigned int flags=GetModifierKeyFlags();
	g_ToolMan.OnRButtonUp(flags,point.x,point.y);
}

void CScEdView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if (point.x>m_Width-200 && point.y>m_Height-200) {
	} else {
		g_ClickMode=1;
		unsigned int flags=GetModifierKeyFlags();
		g_ToolMan.OnRButtonDown(flags,point.x,point.y);
	}
}


void CScEdView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	g_ClickMode=1;

	unsigned int flags=GetModifierKeyFlags();
	g_ToolMan.OnLButtonUp(flags,point.x,point.y);
}

void CScEdView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	if (point.x>m_Width-200 && point.y>m_Height-200) {
		g_ClickMode=0;
		AdjustCameraViaMinimapClick(point);
	} else {
		g_ClickMode=1;
		unsigned int flags=GetModifierKeyFlags();
		g_ToolMan.OnLButtonDown(flags,point.x,point.y);
	}
}

void CScEdView::AdjustCameraViaMinimapClick(CPoint point) 
{
	// convert from screen space point back to world space point representing intersection of 
	// ray with terrain plane
	CVector3D pos;
	pos.X=float(CELL_SIZE * g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()) * float(point.x+200-m_Width)/200.0f;
	pos.Y=-g_EditorData.m_TerrainPlane.m_Dist;
	pos.Z=float(CELL_SIZE * g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()) * float(m_Height-point.y)/197.0f;

	// calculate desired camera point from this
	CVector3D startpos=g_NaviCam.GetCamera().m_Orientation.GetTranslation();
	CVector3D rayDir=g_NaviCam.GetCamera().m_Orientation.GetIn();
	
	float distToPlane=g_EditorData.m_TerrainPlane.DistanceToPlane(startpos);
	float dot=rayDir.Dot(g_EditorData.m_TerrainPlane.m_Norm);
	CVector3D endpos=pos+(rayDir*(distToPlane/dot));

	// translate camera from old point to new
	CVector3D trans=endpos-startpos;
	g_NaviCam.GetCamera().m_Orientation.Translate(trans);

	g_EditorData.OnCameraChanged();
}

bool CScEdView::AppHasFocus()
{
	CWnd* wnd=AfxGetMainWnd();
	if (!wnd) return false;

	CWnd* focuswnd=GetFocus();

	while (focuswnd) {
		if (focuswnd==wnd) return true;
		focuswnd=focuswnd->GetParent();
	}

	return false;
}

void CScEdView::IdleTimeProcess() 
{
	if (m_LastTickTime==-1) {
		m_LastTickTime=get_time();
		return;
	}

	// fake a mouse move from current position if either mouse button is down
	unsigned int flags=0;
	if ((::GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (::GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
		unsigned int flags=GetModifierKeyFlags();
		g_ToolMan.OnMouseMove(flags,m_LastMousePos.x,m_LastMousePos.y);
	}

	double curtime=get_time();
	double diff=curtime-m_LastTickTime;

	if (m_LastFrameDuration>0) {
		
		g_EditorData.UpdateWorld(float(m_LastFrameDuration));

		// check app has focus
		if (AppHasFocus()) {
			POINT pt;
			if (GetCursorPos(&pt)) {
				// want to scroll?
				int scrollspeed=g_UserCfg.GetOptionInt(CFG_SCROLLSPEED);
				if (scrollspeed>0) {
					RECT rect;
					AfxGetMainWnd()->GetWindowRect(&rect);

					// scale translation by distance from terrain
					float h=g_NaviCam.GetCamera().m_Orientation.GetTranslation().Y;
					float speed=h*0.1f;
					
					// scale translation to account for fact that we might not be running at the same rate as the timer
					speed*=float(diff);
					
					// scale by user requested speed
					speed*=scrollspeed;

					bool changed=false;
					if (pt.x<rect.left+16) {
						CVector3D left=g_NaviCam.GetCamera().m_Orientation.GetLeft();
						// strip vertical movement, to prevent moving into/away from terrain plane 
						left.Y=0;
						left.Normalize();
						g_NaviCam.GetCamera().m_Orientation.Translate(left*speed);
						changed=true;
					} else if (pt.x>rect.right-16) {
						CVector3D left=g_NaviCam.GetCamera().m_Orientation.GetLeft();
						// strip vertical movement, to prevent moving into/away from terrain plane 
						left.Y=0;
						left.Normalize();
						g_NaviCam.GetCamera().m_Orientation.Translate(left*(-speed));
						changed=true;
					}

					if (pt.y>rect.bottom-16) {
						CVector3D up=g_NaviCam.GetCamera().m_Orientation.GetUp();
						// strip vertical movement, to prevent moving into/away from terrain plane 
						up.Y=0;
						up.Normalize();
						g_NaviCam.GetCamera().m_Orientation.Translate(up*(-speed));
						changed=true;
					} else if (pt.y<rect.top+16) {
						CVector3D up=g_NaviCam.GetCamera().m_Orientation.GetUp();
						// strip vertical movement, to prevent moving into/away from terrain plane 
						up.Y=0;
						up.Normalize();
						g_NaviCam.GetCamera().m_Orientation.Translate(up*speed);
						changed=true;
					}

					if (changed) {
						g_EditorData.OnCameraChanged();
					}
				}
			}
		}
	}

	// store frame time	
	m_LastFrameDuration=diff;

	// store current time
	m_LastTickTime=curtime;

	// finally, redraw view
	HWND hWnd = GetSafeHwnd();
	HDC hDC = ::GetDC(hWnd);
	wglMakeCurrent(hDC,m_hGLRC);
	g_EditorData.OnDraw();
	SwapBuffers(hDC);
}


LRESULT CScEdView::OnMouseWheel(WPARAM wParam,LPARAM lParam) 
{
	int xPos = LOWORD(lParam); 
	int yPos = HIWORD(lParam); 
	SHORT fwKeys = LOWORD(wParam);
	SHORT zDelta = HIWORD(wParam);

	g_NaviCam.OnMouseWheelScroll(0,xPos,yPos,float(zDelta)/120.0f);
	return 0;
}


void CScEdView::OnMButtonDown(UINT nFlags, CPoint point) 
{
	g_NaviCam.OnMButtonDown(0,point.x,point.y);
}

void CScEdView::OnMButtonUp(UINT nFlags, CPoint point) 
{
	g_NaviCam.OnMButtonUp(0,point.x,point.y);
}


BOOL CScEdView::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
		if (pMsg->message==WM_KEYDOWN) {
			int key=(int) pMsg->wParam;
			CMainFrame* mainfrm=(CMainFrame*)AfxGetApp()->m_pMainWnd;
			switch (key) {
				case VK_F1:
					mainfrm->OnViewRenderStats();
					break;
				case VK_F9:
					mainfrm->OnViewScreenshot();
					break;
				case 'Z':
					if (GetAsyncKeyState(VK_CONTROL)) 
						mainfrm->OnEditUndo();
					break;
				case 'Y':
					if (GetAsyncKeyState(VK_CONTROL)) 
						mainfrm->OnEditRedo();
					break;
				case VK_DELETE:
					CSelectObjectTool::GetTool()->DeleteSelected();
					break;
			}
		}
		return 1;
	} else {
		return CView::PreTranslateMessage(pMsg);
	}
}
