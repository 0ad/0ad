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

#include "ps/CLogger.h"
#define LOG_CATEGORY "gui"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CInput::CInput() : m_iBufferPos(0)
{
	AddSetting(GUIST_float,					"buffer-zone");
	AddSetting(GUIST_CStrW,					"caption");
	AddSetting(GUIST_CStr,					"font");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar-style");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_int,					"cell-id");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CStr,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip-style");
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

	UpdateText(); // will create an empty row, just so we have somewhere to position the insertion marker
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
	/*	case '\n':
			// TODO Gee: (2004-09-07) New line? I should just add '\n'
			*pCaption += wchar_t('\n');
			++m_iBufferPos;
			break;
*/
	/*	case '\r':
			// TODO Gee: (2004-09-07) New line? I should just add '\n'
			*pCaption += wchar_t('\n');
			++m_iBufferPos;
			break;
*/
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
				*pCaption = pCaption->Left( (long) pCaption->Length()-1);
			else
				*pCaption = pCaption->Left( m_iBufferPos-1 ) + 
							pCaption->Right( (long) pCaption->Length()-m_iBufferPos );

			UpdateText(m_iBufferPos-1, m_iBufferPos, m_iBufferPos-1);
			
			--m_iBufferPos;
			break;

		case SDLK_DELETE:
			if (pCaption->Length() == 0 ||
				m_iBufferPos == pCaption->Length())
				break;

			*pCaption = pCaption->Left( m_iBufferPos ) + 
						pCaption->Right( (long) pCaption->Length()-(m_iBufferPos+1) );

			UpdateText(m_iBufferPos, m_iBufferPos+1, m_iBufferPos);
			break;

		case SDLK_HOME:
			m_iBufferPos = 0;
			break;

		case SDLK_END:
			m_iBufferPos = (long) pCaption->Length();
			break;

		case SDLK_LEFT:
			if (m_iBufferPos) 
				--m_iBufferPos;
			break;

		case SDLK_RIGHT:
			if (m_iBufferPos != pCaption->Length())
				++m_iBufferPos;
			break;

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

		case SDLK_PAGEUP:
			//if (m_iMsgHistPos != (int)m_deqMsgHistory.size()) m_iMsgHistPos++;
			break;

		case SDLK_PAGEDOWN:
			//if (m_iMsgHistPos != 1) m_iMsgHistPos--;
			break;
		/* END: Message History Lookup */

		case '\r':
			cooked = '\n'; // Change to '\n' and do default:
			// NOTE: Fall-through

		default: //Insert a character
			if (cooked == 0)
				return EV_PASS; // Important, because we didn't use any key

			if (m_iBufferPos == pCaption->Length())
				*pCaption += cooked;
			else
				*pCaption = pCaption->Left(m_iBufferPos) + CStrW(cooked) + 
							pCaption->Right((long) pCaption->Length()-m_iBufferPos);

			UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

			++m_iBufferPos;

			// TODO
			//UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+4);

			//m_iBufferPos+=4;

			break;
	}

	//UpdateText();

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

		if (Message.value == CStr("caption"))
		{
			UpdateText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
		// Okay, this section is about pressing the mouse and
		//  choosing where the point should be placed. For
		//  instance, if we press between a and b, the point
		//  should of course be placed accordingly. Other
		//  special cases are handled like the input box norms.
		{

		CPos mouse = GetMousePos();

		// Pointer to caption, will come in handy
		CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

		// Now get the height of the font.
		CFont font ("Console");

		float spacing = (float)font.GetLineSpacing();
		float height = (float)font.GetHeight();

		// Change mouse position relative to text.
		//  Include scrolling.
		mouse -= m_CachedActualSize.TopLeft();

		//if ((m_CharacterPositions.size()-1) * spacing + height < mouse.y)
		//	m_iBufferPos = pCaption->Length();
		int row = (int)(mouse.y / spacing);//m_CharachterPositions.size()

		if (row > (int)m_CharacterPositions.size()-1)
			row = (int)m_CharacterPositions.size()-1;

		// TODO Gee (2004-11-21): Okay, I need a 'list' for some reasons, but I would really
		//  be able to get the specific element here. This is hopefully a temporary hack.

		list<SRow>::iterator current = m_CharacterPositions.begin();
		advance(current, row);

		//m_iBufferPos = m_CharacterPositions.get.m_ListStart;
		m_iBufferPos = current->m_ListStart;

		// Okay, no loop through the glyphs to find the appropriate X position
		float previous=0.f;
		int i=0;
		for (vector<float>::iterator it=current->m_ListOfX.begin();
			 it!=current->m_ListOfX.end();
			 ++it, ++i)
		{
			if (*it >= mouse.x)
			{
				if (i != 0)
				{
					if (mouse.x - previous > *it - mouse.x)
						m_iBufferPos += i+1;
					else
						m_iBufferPos += i;
				}
				// else let the value be current->m_ListStart which it already is.

				// check if the previous or the current is closest.
				break;
			}
			previous = *it;
		}
		// If a position wasn't found, we will assume the last
		//  character of that line.
		if (i == current->m_ListOfX.size())
			m_iBufferPos += i;

		break;
		}
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
		CGUISpriteInstance *sprite;
		int cell_id;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<int>::GetSetting(this, "cell-id", cell_id);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		CColor color (1.f, 1.f, 1.f, 1.f);
		GUI<CColor>::GetSetting(this, "textcolor", color);

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		CFont font ("Console");
		font.Bind();

		glPushMatrix();

		CStrW caption;
		GUI<CStrW>::GetSetting(this, "caption", caption);

		// Get the height of this font.
		float h = (float)font.GetHeight();
		float ls = (float)font.GetLineSpacing();

		glTranslatef((GLfloat)int(m_CachedActualSize.left), (GLfloat)int(m_CachedActualSize.top+h), bz);
		glColor4fv(color.FloatArray());
		
		float buffered_y=0.f;

		for (list<SRow>::const_iterator it = m_CharacterPositions.begin();
			 it != m_CharacterPositions.end();
			 ++it)
		{
			if (buffered_y > m_CachedActualSize.GetHeight())
				break;

			glPushMatrix();
			// We might as well use 'i' here, because we need it
			for (int i=0; i < (int)it->m_ListOfX.size(); ++i)
			{
				if (it->m_ListStart + i == m_iBufferPos)
				{
					// U+FE33: PRESENTATION FORM FOR VERTICAL LOW LINE
					// (sort of like a | which is aligned to the left of most characters)
					glPushMatrix();
					glwprintf(L"%lc", 0xFE33);
					glPopMatrix();
				}

				glwprintf(L"%lc", caption[it->m_ListStart + i]);
			}

			if (it->m_ListStart + it->m_ListOfX.size() == m_iBufferPos)
			{
				glwprintf(L"%lc", 0xFE33);
			}

			glPopMatrix();
			glTranslatef(0.f, ls, 0.f);

			buffered_y += ls;

		}

		glPopMatrix();

		glDisable(GL_TEXTURE_2D);
	}
}

void CInput::UpdateText(int from, int to_before, int to_after)
{
	CStrW caption;
	GUI<CStrW>::GetSetting(this, "caption", caption);

	SRow row;
	row.m_ListStart = 0;
	
	int to;

	if (to_before == -1)
		to = (int)caption.Length();

	CFont font ("Console");


	//LOG(ERROR, LOG_CATEGORY, "Point 1 %d %d", to_before, to_after);

	list<SRow>::iterator current_line;

	// Used to ... TODO
	int check_point_row_start = -1;
	int check_point_row_end = -1;

	// Reset
	if (from == 0 && to_before == -1)
	{
		m_CharacterPositions.clear();
		current_line = m_CharacterPositions.begin();
	}
	else
	{
		assert(to_before != -1);

		//LOG(ERROR, LOG_CATEGORY, "Point 2 - %d %d", from, to_before);

		list<SRow>::iterator destroy_row_from, destroy_row_to;

		// Iterate, and remove everything between 'from' and 'to_before'
		//  actually remove the entire lines they are on, it'll all have
		//  to be redone. And when going along, we'll delete a row at a time
		//  when continuing to see how much more after 'to' we need to remake.
		int i=0;
		for (list<SRow>::iterator it=m_CharacterPositions.begin(); 
			 it!=m_CharacterPositions.end(); ++it, ++i)
		{
			if (destroy_row_from == list<SRow>::iterator() &&
				it->m_ListStart > from)
			{
				// Destroy the previous line, and all to 'to_before'
				//if (i >= 2)
				//	destroy_row_from = it-2;
				//else
				//	destroy_row_from = it-1;
				destroy_row_from = it;
				--destroy_row_from;

				// For the rare case that we might remove characters to a word
				//  so that it should go on the previous line, we need to
				//  by standards re-do the whole previous line too (if one
				//  exists)
				if (destroy_row_from != m_CharacterPositions.begin())
					--destroy_row_from;
			}

			if (destroy_row_to == list<SRow>::iterator() &&
				it->m_ListStart > to_before)
			{
				destroy_row_to = it;

				// If it isn't the last row, we'll add another row to delete,
				//  just so we can see if the last restorted line is 
				//  identical to what it was before. If it isn't, then we'll
				//  have to continue.
				// 'check_point_row_start' is where we store how the that
				//  line looked.
				if (destroy_row_to != m_CharacterPositions.end())
				{
					check_point_row_start = destroy_row_to->m_ListStart;
					check_point_row_end = check_point_row_start + (int)destroy_row_to->m_ListOfX.size();
					if (destroy_row_to->m_ListOfX.empty())
						++check_point_row_end;
				}

				++destroy_row_to;
				break;
			}
		}

		if (destroy_row_from == list<SRow>::iterator())
		{
			destroy_row_from = m_CharacterPositions.end();
			--destroy_row_from;

			current_line = destroy_row_from;
		}

		if (destroy_row_to == list<SRow>::iterator())
		{
			destroy_row_to = m_CharacterPositions.end();
			check_point_row_start = -1;
		}

		// set 'from' to the row we'll destroy from
		//  and 'to' to the row we'll destroy to
		from = destroy_row_from->m_ListStart;
		
		if (destroy_row_to != m_CharacterPositions.end())
			to = destroy_row_to->m_ListStart; // notice it will iterate [from, to), so it will never reach to.
		else
			to = (int)caption.Length();


		// Setup the first row
		row.m_ListStart = destroy_row_from->m_ListStart;

		//LOG(ERROR, LOG_CATEGORY, "Point 3 %d", to);

		// Set current line, new rows will be added before current_line, so
		//  we'll choose the destroy_row_to, because it won't be deleted
		//  in the coming erase.
		current_line = destroy_row_to;

		list<SRow>::iterator temp_it = destroy_row_to;
		--temp_it;

		CStr c_caption1(caption.GetSubstring(destroy_row_from->m_ListStart, (temp_it->m_ListStart + temp_it->m_ListOfX.size()) -destroy_row_from->m_ListStart));
		//LOG(ERROR, LOG_CATEGORY, "Now destroying: \"%s\"", c_caption1.c_str());

		m_CharacterPositions.erase(destroy_row_from, destroy_row_to);
		
		//LOG(ERROR, LOG_CATEGORY, "--> %d %d", from, to);
		////CStr c_caption(caption.GetSubstring(from, to-from));
		//LOG(ERROR, LOG_CATEGORY, "Re-doing string: \"%s\"", c_caption.c_str());

		// If there has been a change in number of characters
		//  we need to change all m_ListStart that comes after
		//  the interval we just destroyed. We'll change all
		//  values with the delta change of the string length.
		int delta = to_after - to_before;
		if (delta != 0)
		{
			for (list<SRow>::iterator it=current_line;
				 it!=m_CharacterPositions.end();
				 ++it)
			{
				it->m_ListStart += delta;
			}

			// Update our check point too!
			check_point_row_start += delta;
			check_point_row_end += delta;

			if (to != caption.Length())
				to += delta;
		}
	}
	
	int last_word_started=from;
	int last_list_start=-1;
	float x_pos = 0.f;

	//if (to_before != -1)
	//	return;

	//LOG(ERROR, LOG_CATEGORY, "%d %d", from, to);

	for (int i=from; i<to; ++i)
	{
		if (caption[i] == wchar_t('\n'))
		{
			// Input row, and clear it.
			//m_CharacterPositions.push_back(row);
			/*if (m_CharacterPositions.empty())
				m_CharacterPositions.push_back( row );
			else
			{
				vector<SRow>::iterator pos( &m_CharacterPositions[current_line] );
				m_CharacterPositions.insert( pos, row );
				++current_line;
			}*/
			if (i==to-1)
				break; // it will be added outside
			
			CStr c_caption1(caption.GetSubstring(row.m_ListStart, row.m_ListOfX.size()));
			//LOG(ERROR, LOG_CATEGORY, "Adding1: \"%s\" (%d,%d,%d)", c_caption1.c_str(), from, to, i);

			current_line = m_CharacterPositions.insert( current_line, row );
			++current_line;


			// Setup the next row:
			row.m_ListOfX.clear();
			row.m_ListStart = i+1;
			x_pos = 0.f;

			// If it's done now, no more word-wrapping
			//  needs to be done.
			//if (i==to-1)
			//	break;
		}
		else
		{
			if (caption[i] == wchar_t(' ')/* || TODO Gee (2004-10-13): the '-' disappears, fix.
				caption[i] == wchar_t('-')*/)
				last_word_started = i+1;

			x_pos += (float)font.GetCharacterWidth(caption[i]);

			//LOG(ERROR, LOG_CATEGORY, "%c %f", (char)caption[i], (float)font->GetCharacterWidth(caption[i]));

			if (x_pos >= m_CachedActualSize.GetWidth())
			{
				// The following decides whether it will word-wrap a word,
				//  or if it's only one word on the line, where it has to
				//  break the word apart.
				if (last_word_started == row.m_ListStart)
				{
					last_word_started = i;
					row.m_ListOfX.resize(row.m_ListOfX.size() - (i-last_word_started));
					//row.m_ListOfX.push_back( x_pos );
					//continue;
				}
				else
				{
					// regular word-wrap
					row.m_ListOfX.resize(row.m_ListOfX.size() - (i-last_word_started+1));
				}
				
				// Now, create a new line:
				//  notice: when we enter a newline, you can stand with the cursor
				//  both before and after that character, being on different
				//  rows. With automatic word-wrapping, that is not possible. Which
				//  is intuitively correct.
				CStr c_caption1(caption.GetSubstring(row.m_ListStart, row.m_ListOfX.size()));
				//LOG(ERROR, LOG_CATEGORY, "Adding2: \"%s\" - %d [%d, %d[", c_caption1.c_str(), last_word_started, from, to);

				current_line = m_CharacterPositions.insert( current_line, row );
				++current_line;

				// Setup the next row:
				row.m_ListOfX.clear();
				row.m_ListStart = last_word_started;

				i=last_word_started-1;

				x_pos = 0.f;
			}
			else
				// Get width of this character:
				row.m_ListOfX.push_back( x_pos );
		}

		// Check if it's the last iteration, and we're not revising the whole string
		//  because in that case, more word-wrapping might be needed.
		//  also check if the current line isn't the end
		if (to_before != -1 && i == to-1 && current_line != m_CharacterPositions.end())
		{
			//LOG(ERROR, LOG_CATEGORY, "row.m_ListStart = %d", row.m_ListStart);
			//LOG(ERROR, LOG_CATEGORY, "check_point_row_start = %d", check_point_row_start);
			//LOG(ERROR, LOG_CATEGORY, "last_list_start = %d", last_list_start);

		/*	if (last_list_start != -1 &&
				last_list_start == row.m_ListStart)
			{
				to = current_line->m_ListStart;
				LOG(ERROR, LOG_CATEGORY, "*** %d %d", i, to);
			}
			else
			{
*/
				// check all rows and see if any existing 

			//LOG(ERROR, LOG_CATEGORY, "(%d %d) (%d %d)", i, to, row.m_ListStart, check_point_row_start);

				if (row.m_ListStart != check_point_row_start)
				{

					list<SRow>::iterator destroy_row_from, destroy_row_to;

					// Iterate, and remove everything between 'from' and 'to_before'
					//  actually remove the entire lines they are on, it'll all have
					//  to be redone. And when going along, we'll delete a row at a time
					//  when continuing to see how much more after 'to' we need to remake.
					int i=0;
					for (list<SRow>::iterator it=m_CharacterPositions.begin(); 
						it!=m_CharacterPositions.end(); ++it, ++i)
					{
						if (destroy_row_from == list<SRow>::iterator() &&
							it->m_ListStart > check_point_row_start)
						{
							// Destroy the previous line, and all to 'to_before'
							//if (i >= 2)
							//	destroy_row_from = it-2;
							//else
							//	destroy_row_from = it-1;
							destroy_row_from = it;
							//--destroy_row_from;

							//LOG(ERROR, LOG_CATEGORY, "[ %d %d %d", i, it->m_ListStart, check_point_row_start);
						}

						if (destroy_row_to == list<SRow>::iterator() &&
							it->m_ListStart > check_point_row_end)
						{
							destroy_row_to = it;

							// If it isn't the last row, we'll add another row to delete,
							//  just so we can see if the last restorted line is 
							//  identical to what it was before. If it isn't, then we'll
							//  have to continue.
							// 'check_point_row_start' is where we store how the that
							//  line looked.
				//			if (destroy_row_to != 
							if (destroy_row_to != m_CharacterPositions.end())
							{
								check_point_row_start = destroy_row_to->m_ListStart;
								check_point_row_end = check_point_row_start + (int)destroy_row_to->m_ListOfX.size();
								if (destroy_row_to->m_ListOfX.empty())
									++check_point_row_end;

								//LOG(ERROR, LOG_CATEGORY, "] %d %d %d", i, destroy_row_to->m_ListStart, check_point_row_end);
							}
							else
								check_point_row_start = check_point_row_end = -1;

							++destroy_row_to;
							break;
						}
					}

					if (destroy_row_from == list<SRow>::iterator())
					{
						destroy_row_from = m_CharacterPositions.end();
						--destroy_row_from;

						current_line = destroy_row_from;
					}

					if (destroy_row_to == list<SRow>::iterator())
					{
						destroy_row_to = m_CharacterPositions.end();
						check_point_row_start = check_point_row_end = -1;
					}

					// set 'from' to the from row we'll destroy
					//  and 'to' to 'to_after'
					from = destroy_row_from->m_ListStart;
					
					if (destroy_row_to != m_CharacterPositions.end())
						to = destroy_row_to->m_ListStart; // notice it will iterate [from, to[, so it will never reach to.
					else
						to = (int)caption.Length();


					//LOG(ERROR, LOG_CATEGORY, "Point 3 %d", to);

					// Set current line, new rows will be added before current_line, so
					//  we'll choose the destroy_row_to, because it won't be deleted
					//  in the coming erase.
					current_line = destroy_row_to;

					m_CharacterPositions.erase(destroy_row_from, destroy_row_to);

					//LOG(ERROR, LOG_CATEGORY, "--> %d %d", from, to);
					CStr c_caption(caption.GetSubstring(from, to-from));
					//LOG(ERROR, LOG_CATEGORY, "Re-doing string: \"%s\"", c_caption.c_str());

					/*if (current_line != m_CharacterPositions.end())
					{
						current_line = m_CharacterPositions.erase(current_line);

						if (current_line != m_CharacterPositions.end())
						{
							check_point_row_start = current_line->m_ListStart;
							to = check_point_row_start;
						}
						else
						{
							to = caption.Length();
						}
					}
					else
					{
						check_point_row_start = -1; // just one more row, which is the last, we don't need this anymore
						to = caption.Length();
					}*/

					/*if (destroy_row_to != m_CharacterPositions.end())
						to = destroy_row_to->m_ListStart-1;
					else
						to = caption.Length();*/
				}
				// else, the for loop will end naturally.
			//}

			//last_list_start = row.m_ListStart;
		}
	}
	// add the final row (even if empty)
	CStr c_caption1(caption.GetSubstring(row.m_ListStart, row.m_ListOfX.size()));
	//LOG(ERROR, LOG_CATEGORY, "Adding3: \"%s\"", c_caption1.c_str());

	m_CharacterPositions.insert( current_line, row );
	//++current_line;
}
