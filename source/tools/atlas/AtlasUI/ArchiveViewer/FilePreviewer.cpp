/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "FilePreviewer.h"

#include "DatafileIO/XMB/XMB.h"
#include "DatafileIO/DDT/DDT.h"

#include "wx/sound.h"
#include "wx/image.h"

using namespace DatafileIO;

class ImagePanel : public /*wxPanel*/wxScrolledWindow
{
public:
	ImagePanel(wxWindow* parent, const wxImage& img)
		: m_Bmp(img),
//		wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(img.GetWidth(), img.GetHeight()))
		wxScrolledWindow(parent)
	{
		SetScrollbars(1, 1, img.GetWidth(), img.GetHeight());
	}

	void OnPaint(wxPaintEvent& WXUNUSED(event))
	{
		wxPaintDC dc(this);
		dc.Clear();
		if (m_Bmp.Ok())
		{
			dc.DrawBitmap(m_Bmp, CalcScrolledPosition(wxPoint(0, 0)));
		}
	}

private:
	wxBitmap m_Bmp;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(ImagePanel, /*wxPanel*/wxScrolledWindow)
	EVT_PAINT(ImagePanel::OnPaint)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////////

FilePreviewer::FilePreviewer(wxWindow* parent)
: wxPanel(parent), m_ContentPanel(NULL)
{
	m_MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_MainSizer);
}

enum FileType
{
	UNKNOWN,
	XMB,
	XML,
	DDT,
	WAV,
	UNKNOWN_TEXT
};

void FilePreviewer::PreviewFile(const wxString& filename, SeekableInputStream& stream)
{
	if (m_ContentPanel)
	{
		m_MainSizer->Detach(m_ContentPanel);
		m_ContentPanel->Destroy();
	}

	Freeze();

	m_ContentPanel = new wxPanel(this);
	m_MainSizer->Add(m_ContentPanel, wxSizerFlags().Expand().Proportion(1));

	wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* fileInfo = new wxStaticText(m_ContentPanel, wxID_ANY, wxString::Format(_("Filename: %s"), filename.c_str()));
	sizer->Add(fileInfo, wxSizerFlags().Border(wxALL, 5));
	m_ContentPanel->SetSizer(sizer);

	FileType type = UNKNOWN;

	wxString extn;
	int dot = filename.Find(_T('.'), true);
	if (dot != -1)
		extn = filename.Mid(dot).Lower();

#define X(x) extn==_T(x)
	if (X(".xmb"))
		type = XMB;

	else if (X(".xml") || X(".lgt") || X(".dtd") || X(".shp"))
		type = XML;

	else if (X(".ddt"))
		type = DDT;

	else if (X(".wav"))
		type = WAV;

	else if (X(".txt") || X(".xs") || X(".psh") || X(".vsh") || X(".hlsl") || X(".inc"))
		type = UNKNOWN_TEXT;
#undef X

	wxString extraFileInfo;

	if (type == XMB)
	{
		DatafileIO::XMBFile* file = DatafileIO::XMBFile::LoadFromXMB(stream);
		std::wstring text = file->SaveAsXML();
		delete file;
		
		wxString wtext (text.c_str(), text.length());
		wtext = L"<!-- converted from XMB to XML -->\n" + wtext;

		sizer->Add(new wxTextCtrl(m_ContentPanel, wxID_ANY, wtext, wxDefaultPosition, wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL | wxTE_RICH), // use RICH so we can display big files
			wxSizerFlags().Expand().Proportion(1).Border(wxALL, 5));

		extraFileInfo = _("Format: XMB");
	}

	else if (type == XML || type == UNKNOWN_TEXT)
	{
		void* buf;
		size_t bufSize;
		if (! stream.AcquireBuffer(buf, bufSize))
		{
			wxFAIL_MSG(_T("Buffer acquisition failed"));
		}
		else
		{
			// Assume UTF8, and convert to wxString
			wxString wtext ((char*)buf, wxConvUTF8, bufSize);

			sizer->Add(new wxTextCtrl(m_ContentPanel, wxID_ANY, wtext, wxDefaultPosition, wxDefaultSize,
				wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL),
				wxSizerFlags().Expand().Proportion(1).Border(wxALL, 5));

			if (type == XML)
				extraFileInfo = _("Format: XML");
			else
				extraFileInfo = _("Format: unidentified text");
		}
	}

	else if (type == DDT)
	{
		DDTFile file(stream);
		if (! file.Read(DDTFile::DDT))
		{
			wxLogError(_("Failed to read DDT file"));
		}
		else
		{
			wxArrayString formatStrings;
			switch (file.m_Type_Usage) {
				case DDTFile::BUMP: formatStrings.Add(_("normal map")); break;
				case DDTFile::CUBE: formatStrings.Add(_("cube map")); break;
			}
			switch (file.m_Type_Alpha) {
				case DDTFile::NONE: formatStrings.Add(_("no alpha")); break;
//				case DDTFile::PLAYER: formatStrings.Add(_("player colour")); break;
//				case DDTFile::TRANS: formatStrings.Add(_("transparency")); break;
				case DDTFile::BLEND: formatStrings.Add(_("terrain blend")); break;
			}
			switch (file.m_Type_Format) {
				case DDTFile::BGRA: formatStrings.Add(_("32-bit BGRA")); break;
				case DDTFile::DXT1: formatStrings.Add(_("DXT1")); break;
				case DDTFile::GREY: formatStrings.Add(_("8-bit grey")); break;
				case DDTFile::DXT3: formatStrings.Add(_("DXT3")); break;
				case DDTFile::NORMSPEC: formatStrings.Add(_("specular+normal")); break;
			}

			wxString formatString;
			for (size_t i = 0; i < formatStrings.GetCount(); ++i)
				formatString += (i ? _T(", ") : _T("")) + formatStrings[i];
			if (! formatString.Len())
				formatString = _("unknown");

			extraFileInfo = wxString::Format(_("Format: DDT texture\nSubformat: %d %d %d (%s)"),
				file.m_Type_Usage, file.m_Type_Alpha, file.m_Type_Format, formatString.c_str());

			void* buffer;
			int width, height;
			if (! file.GetImageData(buffer, width, height, false))
			{
				extraFileInfo += _("\n(Unrecognised format - unable to display)");
			}
			else
			{
				sizer->Add(new ImagePanel(m_ContentPanel, wxImage(width, height, (unsigned char*)buffer)),
					wxSizerFlags().Expand().Proportion(1).Border(wxALL, 5));
			}
		}
	}

	else if (type == WAV)
	{
		void* buf;
		size_t bufSize;
		if (! stream.AcquireBuffer(buf, bufSize))
		{
			wxFAIL_MSG(_T("Buffer acquisition failed"));
		}
		else
		{
			// HACK: If we destroy the sound object before it's finished playing,
			// it deallocates the audio data and fails to play (or crashes).
			// (That's assuming we fix wxSound to not just leak the memory.)
			// So, just use a static object, and hope it stops playing before
			// the program is exited.
			// The wxSound-from memory constructor does not exist on OS X, so
			// just show a warning there.
#ifdef __APPLE__
			wxFAIL_MSG(_T("WAV playback not available on Mac OS X"));
#else
			static wxSound snd;
			snd.Stop();
			// HACK, FIXME, XXX: I'd like to call the wx people idiots for
			// having different API:s on different platforms, as well as for
			// having public non-API methods.
#if __APPLE__
			snd.~wxSound();
			new (&snd) wxSound((int)bufSize, (const wxByte*)buf);
#else
			snd.Create((int)bufSize, (const wxByte*)buf);
#endif
			snd.Play();
#endif
			stream.ReleaseBuffer(buf);
		}
		
		extraFileInfo = _("Format: WAV audio");
	}

	fileInfo->SetLabel(fileInfo->GetLabel() + _T("\n") + extraFileInfo);

	m_MainSizer->Layout();

	Thaw();
}
