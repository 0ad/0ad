/*
CInput
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"
#include "CInput.h"

#include "ps/Font.h"
#include "ogl.h"

// TODO Gee: new
#include "OverlayText.h"
#include "lib/res/unifont.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CInput::CInput() : m_iBufferPos(0)
{
	AddSetting(GUIST_float,			"buffer-zone");
	AddSetting(GUIST_CStrW,			"caption");
	AddSetting(GUIST_CStr,			"font");
	AddSetting(GUIST_bool,			"scrollbar");
	AddSetting(GUIST_CStr,			"scrollbar-style");
	AddSetting(GUIST_CStr,			"sprite");
	AddSetting(GUIST_CColor,		"textcolor");
	// TODO Gee: (2004-08-14)
	//  Add a setting for buffer zone
	//AddSetting(GUIST_int,			"

	//GUI<bool>::SetSetting(this, "ghost", true);
	GUI<bool>::SetSetting(this, "scrollbar", false);

	// Add scroll-bar
	CGUIScrollBarVertical * bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	bar->SetUseEdgeButtons(true);
	AddScrollBar(bar);
}

CInput::~CInput()
{
}

int CInput::ManuallyHandleEvent(const SDL_Event* ev)
{
	assert(m_iBufferPos != -1);

	int szChar = ev->key.keysym.sym;
	wchar_t cooked = (wchar_t)ev->key.keysym.unicode;

	// Since the GUI framework doesn't handle to set settings
	//  in Unicode (CStrW), we'll simply retrieve the actual
	//  pointer and edit that.
	CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

	switch (szChar){
		//TODOcase '\r':
		case '\n':
			// TODO Gee: (2004-09-07) New line? I should just add '\n'
			*pCaption += wchar_t('\n');
			++m_iBufferPos;
			break;

		case '\t':
			/* Auto Complete */
			// TODO Gee: (2004-09-07) What to do with tab?
			break;

		case '\b':
			// TODO Gee: (2004-09-07) What is this?
			if (pCaption->Length() == 0 ||
				m_iBufferPos == 0)
				break;

			if (m_iBufferPos == pCaption->Length())
				*pCaption = pCaption->Left( pCaption->Length()-1 );
			else
				*pCaption = pCaption->Left( m_iBufferPos-1 ) + 
							pCaption->Right( pCaption->Length()-m_iBufferPos );

			--m_iBufferPos;
			break;

		case SDLK_DELETE:
			if (pCaption->Length() == 0 ||
				m_iBufferPos == pCaption->Length())
				break;

			*pCaption = pCaption->Left( m_iBufferPos ) + 
						pCaption->Right( pCaption->Length()-(m_iBufferPos+1) );

			break;

		case SDLK_HOME:
			m_iBufferPos = 0;
			break;

		case SDLK_END:
			m_iBufferPos = pCaption->Length();
			break;

		case SDLK_LEFT:
			if (m_iBufferPos) 
				--m_iBufferPos;
			break;

		case SDLK_RIGHT:
			if (m_iBufferPos != pCaption->Length())
				++m_iBufferPos;
			break;

		/* BEGIN: Buffer History Lookup */
		case SDLK_UP:
			/*if (m_deqBufHistory.size() && iHistoryPos != (int)m_deqBufHistory.size() - 1)
			{
				iHistoryPos++;
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			}*/
			break;

		case SDLK_DOWN:
			/*if (iHistoryPos != -1) iHistoryPos--;

			if (iHistoryPos != -1)
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			else FlushBuffer();*/
			break;
		/* END: Buffer History Lookup */

		/* BEGIN: Message History Lookup */
		case SDLK_PAGEUP:
			//if (m_iMsgHistPos != (int)m_deqMsgHistory.size()) m_iMsgHistPos++;
			break;

		case SDLK_PAGEDOWN:
			//if (m_iMsgHistPos != 1) m_iMsgHistPos--;
			break;
		/* END: Message History Lookup */

		default: //Insert a character
			if (cooked == 0)
				return EV_PASS; // Important, because we didn't use any key

			if (m_iBufferPos == pCaption->Length())
				*pCaption += cooked;
			else
				*pCaption = pCaption->Left(m_iBufferPos) + CStrW(cooked) + 
							pCaption->Right(pCaption->Length()-m_iBufferPos);

			++m_iBufferPos;
			break;
	}
		
	return EV_HANDLED;
}

void CInput::HandleMessage(const SGUIMessage &Message)
{
	// TODO Gee:
	IGUIScrollBarOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (scrollbar && 
		    (Message.value == CStr("size") || Message.value == CStr("z") ||
			 Message.value == CStr("absolute")))
		{
			
			GetScrollBar(0).SetX( m_CachedActualSize.right );
			GetScrollBar(0).SetY( m_CachedActualSize.top );
			GetScrollBar(0).SetZ( GetBufferedZ() );
			GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );
		}

		// Update scrollbar
		if (Message.value == CStr("scrollbar-style"))
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(this, Message.value, scrollbar_style);

			GetScrollBar(0).SetScrollBarStyle( scrollbar_style );
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	//	SetFocus();
		//m_iBufferPos = 0;

	case GUIM_MOUSE_WHEEL_DOWN:
		GetScrollBar(0).ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	case GUIM_MOUSE_WHEEL_UP:
		GetScrollBar(0).ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		HandleMessage(SGUIMessage(GUIM_MOUSE_MOTION));
		break;

	case GUIM_LOAD:
		//SetFocus();

	case GUIM_GOT_FOCUS:
		m_iBufferPos = 0;
		break;

	case GUIM_LOST_FOCUS:
		m_iBufferPos = -1;
		break;

	default:
		break;
	}
}

void CInput::Draw() 
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	if (scrollbar)
	{
		// Draw scrollbar
		IGUIScrollBarOwner::Draw();
	}

	if (GetGUI())
	{
		CStr sprite;
		GUI<CStr>::GetSetting(this, "sprite", sprite);

		GetGUI()->DrawSprite(sprite, bz, m_CachedActualSize);

		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		CColor color;
		GUI<CColor>::GetSetting(this, "textcolor", color);

		CFont *font=NULL;

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		font = new CFont("Console");
		font->Bind();

		glPushMatrix();

		CStrW caption;
		GUI<CStrW>::GetSetting(this, "caption", caption);

		// Get the height of this font.
		float h = (float)font->GetHeight();

		glTranslatef((GLfloat)int(m_CachedActualSize.left), (GLfloat)int(m_CachedActualSize.top+h), bz);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		
		// U+FE33: PRESENTATION FORM FOR VERTICAL LOW LINE
		// (sort of like a | which is aligned to the left of most characters)

		for (int i=0; i<caption.Length(); ++i)
		{
			if (m_iBufferPos == i)
			{
				glPushMatrix();
				glwprintf(L"%lc", 0xFE33);
				glPopMatrix();
			}

			//glwprintf(L"%ls", caption.c_str());
			glwprintf(L"%lc", caption[i]);
		}

		if (m_iBufferPos == caption.Length())
			glwprintf(L"%lc", 0xFE33);

		glPopMatrix();

		glDisable(GL_TEXTURE_2D);

		delete font;
	}
}
