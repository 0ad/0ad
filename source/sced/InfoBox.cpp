#include "InfoBox.h"
#include "UIGlobals.h"
#include "types.h"
#include "ogl.h"
#include "timer.h"
#include "NPFont.h"
#include "NPFontManager.h"
#include "OverlayText.h"
#include "Renderer.h"


static const char* DefaultFontName="mods/official/fonts/verdana18.fnt";

CInfoBox::CInfoBox() : m_Font(0), m_Visible(false)
{
	m_LastFPSTime=get_time();
	m_Stats.Reset();	
}

void CInfoBox::Initialise()
{
	m_Font=NPFontManager::instance().add(DefaultFontName);	
}

void CInfoBox::Render()
{
	if (!m_Visible) return;

	const u32 panelColor=0x80f9527d;
	const u32 borderColor=0xfffa0043;

	// setup renderstate
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// load identity modelview
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// setup ortho view
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	int w=g_Renderer.GetWidth();
	int h=g_Renderer.GetHeight();
	glOrtho(0,w,0,h,-1,1);

	glColor4ubv((const GLubyte*) &panelColor);

	// render infobox as quad
	glBegin(GL_QUADS);
	glVertex2i(3,h-180);
	glVertex2i(150,h-180);
	glVertex2i(150,h-2);
	glVertex2i(3,h-2);
	glEnd();

	// render border
	glColor4ubv((const GLubyte*) &borderColor);

	glLineWidth(2);
	glBegin(GL_LINE_LOOP);
	glVertex2i(1,h-182);
	glVertex2i(152,h-182);
	glVertex2i(152,h-1);
	glVertex2i(1,h-1);
	glEnd();

	// render any info now, assuming we've got a font
	if (m_Font) {
		RenderInfo();
	}

	// restore matrices
	glPopMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// restore renderstate
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
}

void render(COverlayText* overlaytext)
{
	// get font on overlay
	NPFont* font=overlaytext->GetFont();
	if (!font) return;

	const CStr& str=overlaytext->GetString();
	int len=str.Length();
	if (len==0) return;

	// bind to font texture
	g_Renderer.SetTexture(0,&font->texture());

	// setup texenv
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

	glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

	glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// get overlay's position
	float x,y;
	overlaytext->GetPosition(x,y);

	// setup color
	const CColor& color=overlaytext->GetColor();
	glColor4fv((float*) &color);
	
	float x0=x;
	float ftw=float(font->textureWidth());
	float fth=float(font->textureHeight());
	float fdy=float(font->height())/fth;

	// submit quad verts for each character
	glBegin(GL_QUADS);

	for (int i=0;i<len;i++) {
		// get pointer to char widths
		const NPFont::CharData& cdata=font->chardata(str[i]);
		const int* cw=&cdata._widthA;

		if (cw[0]>0) x0+=cw[0];

		// find the tile for this character
		unsigned char c0=unsigned char(str[i]-32);
		int ix=c0%font->numCols();
		int iy=c0/font->numCols();

		// calc UV coords of character in texture
		float u0=float(ix*font->maxcharwidth()+cw[0]-1)/ftw;
		float v0=float(iy*(font->height()+1))/fth;
		float u1=u0+float(cw[1]+1)/ftw;
		float v1=v0+fdy;

		glTexCoord2f(u0,v0);
		glVertex2f(x0,y);

		glTexCoord2f(u1,v0);
		glVertex2f(x0+float(cw[1]+1),y);

		glTexCoord2f(u1,v1);
		glVertex2f(x0+float(cw[1]+1),y+font->height());

		glTexCoord2f(u0,v1);
		glVertex2f(x0,y+font->height());

		// translate such that next character is rendered in correct position
		x0+=float(cw[1]+1);
		if (cw[2]>0) x0+=cw[2];
	}
	glEnd();

	// unbind texture ..
	g_Renderer.SetTexture(0,0);
}	 
	 

void CInfoBox::RenderInfo()
{
	int w=g_Renderer.GetWidth();
	int h=g_Renderer.GetHeight();

	char buf[32];

	if (m_LastStats.m_Counter) {
		u32 dy=m_Font->height()+2;
		float y=float(h-dy);

		sprintf(buf,"FPS: %d",int(m_LastStats.m_Counter/m_LastTickTime));
		COverlayText fpstext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&fpstext);
		y-=dy;

		sprintf(buf,"FT: %.2f",1000*m_LastTickTime/float(m_LastStats.m_Counter));
		COverlayText fttext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&fttext);
		y-=dy;

		u32 totalTris=m_LastStats.m_TerrainTris+m_LastStats.m_ModelTris+m_LastStats.m_TransparentTris;
		sprintf(buf,"TPF: %d",int(totalTris/m_LastStats.m_Counter));
		COverlayText tpftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&tpftext);
		y-=dy;

		sprintf(buf,"TeTPF: %d",int(m_LastStats.m_TerrainTris/m_LastStats.m_Counter));
		COverlayText tetpftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&tetpftext);
		y-=dy;

		sprintf(buf,"MTPF: %d",int(m_LastStats.m_ModelTris/m_LastStats.m_Counter));
		COverlayText mtpftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&mtpftext);
		y-=dy;

		sprintf(buf,"TrTPF: %d",int(m_LastStats.m_TransparentTris/m_LastStats.m_Counter));
		COverlayText trtpftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&trtpftext);
		y-=dy;

		sprintf(buf,"DCPF: %d",int(m_LastStats.m_DrawCalls/m_LastStats.m_Counter));
		COverlayText dcpftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&dcpftext);
		y-=dy;

		sprintf(buf,"SPPF: %d",int(m_LastStats.m_BlendSplats/m_LastStats.m_Counter));
		COverlayText sppftext(5,y,0,DefaultFontName,buf,CColor(1,1,1,1));	
		render(&sppftext);
		y-=dy;
	}
}

void CInfoBox::OnFrameComplete()
{
	// accumulate stats
	m_Stats+=g_Renderer.GetStats();

	// fps check
	double cur_time=get_time();
	if (cur_time-m_LastFPSTime>1) {
		// save tick time
		m_LastTickTime=cur_time-m_LastFPSTime;
		// save stats
		m_LastStats=m_Stats;
		// save time
		m_LastFPSTime=cur_time;
		// reset stats
		m_Stats.Reset();
	}
}