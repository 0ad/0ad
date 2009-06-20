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

#include "FileConverter.h"

#include "Misc/Version.h"

#include "AtlasObject/AtlasObject.h"
#include "DatafileIO/XMB/XMB.h"
#include "DatafileIO/DDT/DDT.h"
#include "DatafileIO/Stream/wx.h"
#include "DatafileIO/Stream/Memory.h"

#include "wx/wfstream.h"
#include "wx/progdlg.h"
#include "wx/config.h"
#include "wx/regex.h"

using namespace DatafileIO;

//#define QUIET

enum FileType { XMB, XML, DDT, TGA };

const wxChar* xmlExtensions[] = {
	_T("amt"), _T("blueprint"), _T("d3dconfig"), _T("dmg"), _T("effect"), _T("impacteffect"),
	_T("lgt"), _T("multieffect"), _T("multips"), _T("multirs"),_T("multitechnique"),
	_T("multitss"), _T("multivs"), _T("particle"), _T("physics"), _T("ps"), _T("rs"),
	_T("tactics"), _T("technique"), _T("tss"), _T("vs"), _T("xml"),
	NULL };

bool IsXMLExtension(const wxString& str)
{
	const wxChar** e = xmlExtensions;
	while (*e)
		if (str == wxString(_T(".")) + *e++)
			return true;
	return false;
}

bool ConvertFiles(const wxArrayString& files, wxWindow* parent);

bool ConvertFile(const wxString& sourceFilename, FileType sourceType,
				 const wxString& targetFilename, FileType targetType,
				 XMLReader* io);

FileConverter::FileConverter(wxWindow* parent)
: wxFrame(parent, wxID_ANY, wxString::Format(_("%s - File Converter"), g_ProgramNameVersion.c_str()))
{
	SetIcon(wxIcon(_T("ICON_FileConverter")));

	m_Transient = true;

	bool succeeded = false;
	wxApp* app = wxTheApp;
	if (app->argc > 1)
	{
		wxArrayString files;
		for (int i = 1; i < app->argc; ++i)
			files.Add(app->argv[i]);
		succeeded = ConvertFiles(files, this);
	}
	else
	{
		wxConfigBase* cfg = wxConfigBase::Get(false);
		wxString defaultDir;
		if (cfg)
			cfg->Read(_T("File Converter/OpenDir"), &defaultDir);

		wxString extns = _("Recognised files");
		extns += _T(" (*.xmb, *.ddt, *.xml, *.tga)|*.xmb;*.ddt;*.xml;*.tga");
		// Add all the extra XML types
		const wxChar** e = xmlExtensions;
		while (*e)
		{
			extns += _T(";*.");
			extns += *e++;
		}
		extns += wxString(_T("|")) + _("All files") + _T(" (*.*)|*.*");

		wxFileDialog dlg (this, _("Select file(s) to convert"), defaultDir, _T(""),
			extns, wxOPEN|/*wxFILE_MUST_EXIST|*/wxMULTIPLE); // for some reason, it complains that files don't exist when they actually do...
		if (dlg.ShowModal() == wxID_OK)
		{
			wxArrayString files;
			dlg.GetPaths(files);
			succeeded = ConvertFiles(files, this);

			if (cfg)
				cfg->Write(_T("File Converter/OpenDir"), dlg.GetDirectory());
		}
		else
		{
			succeeded = false;
		}
	}

	wxLog::FlushActive(); // ensure errors are displayed before the "finished" message

#ifndef QUIET
	if (succeeded)
		wxMessageBox(_("Conversion complete."), _("Finished"), wxOK|wxICON_INFORMATION);
#endif

	Destroy();
}

bool FileConverter::Show(bool show)
{
	if (m_Transient)
		return true;
	else
		return wxFrame::Show(show);
}



bool ConvertFiles(const wxArrayString& files, wxWindow* parent)
{
	XMLReader io; // TODO: don't create this unless necessary, because Xerces is slow

	wxProgressDialog progress(_("Converting files"), _("Please wait"), (int)files.GetCount(), parent,
		wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_SMOOTH);

	for (size_t i = 0; i < files.GetCount(); ++i)
	{
		wxString sourceFilename = files[i];
		wxString targetFilename;
		FileType sourceType, targetType;

		if (! progress.Update((int)i, wxString::Format(_("Converting %s"), sourceFilename.c_str())))
			return false;

		wxString sourceExtn, sourceName;
		{
			int dot = sourceFilename.Find(_T('.'), true);
			if (dot == -1)
			{
				wxLogError(_("No file extension for %s - don't know how to convert"), sourceFilename.c_str());
				continue;
			}
			else
			{
				sourceExtn = sourceFilename.Mid(dot).Lower();
				sourceName = sourceFilename.Mid(0, dot); // no extension
			}
		}

		if (sourceExtn == _T(".xmb"))
		{
			sourceType = XMB;
			targetType = XML;
			// Ignore trailing .xmb
			targetFilename = sourceName;
		}
		else if (sourceExtn == _T(".ddt"))
		{
			sourceType = DDT;
			targetType = TGA; // TODO: allow BMP
			targetFilename = sourceName + _T(".tga");
		}
		else if (sourceExtn == _T(".tga"))
		{
			sourceType = TGA; // TODO: allow BMP
			targetType = DDT;
			targetFilename = sourceName + _T(".ddt");
		}
		else if (IsXMLExtension(sourceExtn))
		{
			sourceType = XML;
			targetType = XMB;
			// Add a trailing .xmb (in addition to the normal .xml/etc)
			targetFilename = sourceFilename + _T(".xmb");
		}
		else
		{
			wxLogError(_("Unknown file extension for %s - don't know how to convert"), sourceFilename.c_str());
			continue;
		}


#ifndef QUIET
		// Warn about overwriting files
		if (wxFile::Exists(targetFilename))
		{
			int ret = wxMessageBox(wxString::Format(_("Output file already exists: %s\nOverwrite file?"), targetFilename.c_str()), _("Overwrite?"), wxYES_NO|wxCANCEL);
			if (ret == wxCANCEL) return false;
			else if (ret == wxNO) continue;
			else /* carry on converting */;
		}
#endif

		// Do the actual conversion
		ConvertFile(sourceFilename, sourceType, targetFilename, targetType, &io);
	}

	return true;
}

bool ConvertFile(const wxString& sourceFilename, FileType sourceType,
				 const wxString& targetFilename, FileType targetType,
				 XMLReader* io)
{
	// Open input file (in binary read mode)
	wxFFileInputStream file (sourceFilename);
	if (! file.Ok())
	{
		wxLogError(_("Failed to open input file %s"), sourceFilename.c_str());
		return false;
	}
	// Decompress input file if necessary (for any file type)
	//std::auto_ptr<InputStream> inStream (new Maybel33tInputStream(new SeekableInputStreamFromWx(file)));
	Maybel33tInputStream inStream (new SeekableInputStreamFromWx(file));
	if (! inStream.IsOk())
	{
		wxLogError(_("Failed to decompress input file %s"), sourceFilename.c_str());
		return false;
	}

	// Handle XMB<->XML conversions
	if (sourceType == XMB || sourceType == XML)
	{
		std::auto_ptr<XMBFile> data (NULL);

		// Read data with the appropriate format
		if (sourceType == XML)
		{
//			if (! io) io = new XMLReader(); // TODO - see earlier comment
			data.reset(io->LoadFromXML(inStream));
			data->format = XMBFile::AOE3; // TODO: let users control this?
		}
		else if (sourceType == XMB)
		{
			data.reset(XMBFile::LoadFromXMB(inStream));
		}

		// Write data with the appropriate format
		if (targetType == XML)
		{
			std::wstring xml = data->SaveAsXML();
			wxFFileOutputStream out(targetFilename, _T("w")); // open in text mode
			if (! out.Ok())
			{
				wxLogError(_("Failed to open output file %s"), targetFilename.c_str());
				return false;
			}

			// Output with UTF-8 encoding
			xml = L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" + xml;
			wxCharBuffer buf = wxString(xml.c_str()).mb_str(wxConvUTF8);
			out.Write(buf.data(), strlen(buf.data()));
		}
		else if (targetType == XMB)
		{
			wxFFileOutputStream out (targetFilename, _T("wb"));
			if (! out.Ok())
			{
				wxLogError(_("Failed to open output file %s"), targetFilename.c_str());
				return false;
			}

			SeekableOutputStreamFromWx out2 (out);
			data->SaveAsXMB(out2);
		}
	}

	else if (sourceType == DDT)
	{
		DDTFile ddt(inStream);
		if (! ddt.Read(DDTFile::DDT))
		{
			wxLogError(_("Failed to read DDT file %s"), sourceFilename.c_str());
			return false;
		}
		// Stick some format-identifying data just before the extension
		// part of the filename:
		wxRegEx re (_T("(.*)\\."), wxRE_ADVANCED);
		wxString newFilename = targetFilename;
		re.ReplaceFirst(&newFilename,
			wxString::Format(_T("\\1.(%d,%d,%d,%d)."),
				ddt.m_Type_Usage, ddt.m_Type_Alpha, ddt.m_Type_Format, ddt.m_Type_Levels));

		wxFFileOutputStream out(newFilename, _T("wb"));
		if (! out.Ok())
		{
			wxLogError(_("Failed to open output file %s"), newFilename.c_str());
			return false;
		}
		SeekableOutputStreamFromWx out2 (out);
		ddt.SaveFile(out2, DDTFile::TGA);
	}

	else if (sourceType == TGA)
	{
		DDTFile ddt(inStream);
		if (! ddt.Read(DDTFile::TGA))
		{
			wxLogError(_("Failed to read TGA file %s"), sourceFilename.c_str());
			return false;
		}
		// Extract the format-identifying data from just before the extension
		// part of the filename:
		wxRegEx re (_T("\\.\\((\\d+),(\\d+),(\\d+),(\\d+)\\)\\."), wxRE_ADVANCED); // regexps in C++ are ugly :-(
		wxString newFilename = targetFilename;
		if (re.Matches(newFilename.c_str()))
		{
			wxASSERT(re.GetMatchCount() == 5);
			long l0 = 0, l1 = 0, l2 = 0, l3 = 0;
			if (re.GetMatch(newFilename, 1).ToLong(&l0)
				&& re.GetMatch(newFilename, 2).ToLong(&l1)
				&& re.GetMatch(newFilename, 3).ToLong(&l2)
				&& re.GetMatch(newFilename, 4).ToLong(&l3))
			{
				ddt.m_Type_Usage = (DDTFile::Type_Usage)l0;
				ddt.m_Type_Alpha = (DDTFile::Type_Alpha)l1;
				ddt.m_Type_Format = (DDTFile::Type_Format)l2;
				ddt.m_Type_Levels = l3;
			}
			else
			{
				// TODO: ask the user for settings? or at least be more helpful
				wxLogError(_("Invalid filename - should be something.(n,n,n,n).tga"));
				return false;
			}
			// Remove the format-identifying part when constructing the DDT filename
			re.ReplaceFirst(&newFilename, _T("."));
		}
		else
		{
			wxLogError(_("Invalid filename - should be something.(n,n,n,n).tga"));
			return false;
		}

		wxFFileOutputStream out(newFilename, _T("wb"));
		if (! out.Ok())
		{
			wxLogError(_("Failed to open output file %s"), newFilename.c_str());
			return false;
		}
		SeekableOutputStreamFromWx out2 (out);
		ddt.SaveFile(out2, DDTFile::DDT);
	}
	
	else
	{
		wxFAIL_MSG(_T("TODO"));
		return false;
	}

	return true;
}
