#include "stdafx.h"
#define _IGNORE_WGL_H_
#include "TexToolsDlgBar.h"
#include "TextureManager.h"
#include "PaintTextureTool.h"
#include "Renderer.h"

#include "ogl.h"
#include "res/tex.h"
#include "res/vfs.h"
#undef _IGNORE_WGL_H_

BEGIN_MESSAGE_MAP(CTexToolsDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CTexToolsDlgBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_CLICK, IDC_LIST_TEXTUREBROWSER, OnClickListTextureBrowser)
	ON_CBN_SELCHANGE(IDC_COMBO_TERRAINTYPES, OnSelChangeTerrainTypes)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BRUSHSIZE, OnReleasedCaptureSliderBrushSize)
END_MESSAGE_MAP()


CTexToolsDlgBar::CTexToolsDlgBar()
{
}

CTexToolsDlgBar::~CTexToolsDlgBar()
{
}

BOOL CTexToolsDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CTexToolsDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}

void CTexToolsDlgBar::Select(CTextureEntry* entry) 
{
	CStatic* curbmp=(CStatic*) GetDlgItem(IDC_STATIC_CURRENTTEXTURE);
	CBitmap* bmp=(CBitmap*) (entry ? entry->GetBitmap() : 0);
	curbmp->SetBitmap((HBITMAP) (*bmp));
	CPaintTextureTool::GetTool()->SetSelectedTexture(entry);
}

// 32 bit colour data struct
struct Color8888 {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

// 24 bit colour data struct
struct Color888 {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

// 16-bit colour data struct
struct Color565 {
	unsigned r : 5;
	unsigned g : 6;
	unsigned b : 5;
};

static void ConvertColor(const Color8888* src,Color565* dst)
{
	dst->r=(src->r>>3);
	dst->g=(src->g>>2);
	dst->b=(src->b>>3);
}

static void ConvertColor(const Color8888* src,Color888* dst)
{
	dst->r=src->r;
	dst->g=src->g;
	dst->b=src->b;
}

int CTexToolsDlgBar::GetCurrentTerrainType()
{
	CComboBox* terraintypes=(CComboBox*) GetDlgItem(IDC_COMBO_TERRAINTYPES);
	return terraintypes->GetCurSel();
}

BOOL CTexToolsDlgBar::BuildImageListIcon(CTextureEntry* texentry)
{
	// bind to texture
	g_Renderer.BindTexture(0,tex_id(texentry->GetHandle()));

	// get image data in BGRA format
	int w,h;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&h);

	unsigned char* texdata=new unsigned char[w*h*4];
	glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA_EXT,GL_UNSIGNED_BYTE,texdata);

	// generate scaled bitmap of correct size
	int bmpsize=32*32*4;
	unsigned int bmpdata[32*32];
	gluScaleImage(GL_BGRA_EXT,w,h,GL_UNSIGNED_BYTE,texdata,32,32,GL_UNSIGNED_BYTE,bmpdata);
		
	// create the actual CBitmap object
	BOOL success=TRUE;
	CDC* dc=GetDC();
	CBitmap* bmp=new CBitmap;
	texentry->SetBitmap(bmp);

	if (bmp->CreateCompatibleBitmap(dc,32,32)) {				

		// query bpp of bitmap
		BITMAP bm;
		if (bmp->GetBitmap(&bm)) {
			int bpp=bm.bmBitsPixel;
			if (bpp==16) {
				// build 16 bit image
				unsigned short* tmp=new unsigned short[32*32];
				for (int i=0;i<32*32;i++) {
					ConvertColor((Color8888*) &bmpdata[i],(Color565*) &tmp[i]);
				}
				bmp->SetBitmapBits(32*32*2,tmp);
				delete[] tmp;
			} else if (bpp==24) {
				// ditch alpha from image
				unsigned short* tmp=new unsigned short[32*32];
				for (int i=0;i<32*32;i++) {
					ConvertColor((Color8888*) &bmpdata[i],(Color888*) &tmp[i]);
				}
				bmp->SetBitmapBits(32*32*3,tmp);
				delete[] tmp;
			} else if (bpp==32) {
				// upload original image
				bmp->SetBitmapBits(bmpsize,bmpdata);
			}
			
			// now add to image list
			m_ImageList.Add(bmp,RGB(255,255,255));
			
			// success; note this
			success=TRUE;
		}
	}

	// clean up				
	delete[] texdata;
	ReleaseDC(dc);

	return success;
}

BOOL CTexToolsDlgBar::AddImageListIcon(CTextureEntry* texentry)
{
	// get a bitmap for imagelist yet?
	if (!texentry->GetBitmap()) {
		// nope; create one now
		BuildImageListIcon(texentry);
	}

	// add bitmap to imagelist
	m_ImageList.Add((CBitmap*) texentry->GetBitmap(),RGB(255,255,255));

	return TRUE;
}

BOOL CTexToolsDlgBar::OnInitDialog()
{
	 // get the current window size and position
	CRect rect;
	GetWindowRect(rect);

	// now change the size, position, and Z order of the window.
	::SetWindowPos(m_hWnd,HWND_TOPMOST,10,rect.top,rect.Width(),rect.Height(),SWP_HIDEWINDOW);

	m_ImageList.Create(32,32,ILC_COLORDDB,0,16);
	m_ImageList.SetBkColor(RGB(255,255,255));

	// build combo box for terrain types
	CComboBox* terraintypes=(CComboBox*) GetDlgItem(IDC_COMBO_TERRAINTYPES);
	
	const std::vector<CTextureManager::STextureType>& ttypes=g_TexMan.m_TerrainTextures;
	for (uint i=0;i<ttypes.size();i++) {
		terraintypes->AddString((const char*) ttypes[i].m_Name);
	}
	if (ttypes.size()>0) {
		// select first type
		terraintypes->SetCurSel(0);
	}

	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_TEXTUREBROWSER);
	// set lists images
	listctrl->SetImageList(&m_ImageList,LVSIL_NORMAL);

	// build icons for existing textures
	if (ttypes.size()) {
		const std::vector<CTextureEntry*>& textures=ttypes[0].m_Textures;
		for (uint i=0;i<textures.size();i++) {
			// add image icon for this
			AddImageListIcon(textures[i]);
		
			// add to list ctrl
			int index=listctrl->GetItemCount();
			listctrl->InsertItem(index,(const char*) textures[i]->GetName(),index);
		}

		// select first entry if we've got any entries
		if (textures.size()>0) {
			Select(textures[0]);
		} else {
			Select(0);
		}	
	}

	OnSelChangeTerrainTypes();

	// set up brush size slider
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sliderctrl->SetRange(0,CPaintTextureTool::MAX_BRUSH_SIZE);
	sliderctrl->SetPos(CPaintTextureTool::GetTool()->GetBrushSize());

	return TRUE;  	              
}

void CTexToolsDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

void CTexToolsDlgBar::OnClickListTextureBrowser(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_TEXTUREBROWSER);

	POSITION pos=listctrl->GetFirstSelectedItemPosition();
	if (!pos) return;

	int index=listctrl->GetNextSelectedItem(pos);
	Select(g_TexMan.m_TerrainTextures[GetCurrentTerrainType()].m_Textures[index]);
	
	*pResult = 0;
}

void CTexToolsDlgBar::OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	CPaintTextureTool::GetTool()->SetBrushSize(sliderctrl->GetPos());
	*pResult = 0;
}


void CTexToolsDlgBar::OnSelChangeTerrainTypes()
{
	// clear out the old image list
	int i;
	int count=m_ImageList.GetImageCount();
	for (i=count-1;i>=0;--i) {
		m_ImageList.Remove(i);
	}

	// clear out the listctrl
	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_TEXTUREBROWSER);
	listctrl->DeleteAllItems();
	
	// add icons to image list from new selected terrain types
	if (GetCurrentTerrainType()!=CB_ERR)
	{
		std::vector<CTextureEntry*>& textures=g_TexMan.m_TerrainTextures[GetCurrentTerrainType()].m_Textures;
		for (uint j=0;j<textures.size();j++) {
			// add image icon for this
			AddImageListIcon(textures[j]);
			
			// add to list ctrl
			listctrl->InsertItem(j,(const char*) textures[j]->GetName(),j);
		}
	}
}