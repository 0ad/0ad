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

#include "VideoRecorder.h"

#include "FFmpeg.h"

#include "GameInterface/Messages.h"

using namespace AtlasMessage;

class RecorderDialog : public wxDialog
{
	enum
	{
		ID_FILENAME,
		ID_FILECHOOSE,
		ID_FRAMERATE,
		ID_BITRATE,
		ID_DURATION,
		ID_FILESIZE
	};

public:
	RecorderDialog(wxWindow* parent, float duration)
		: wxDialog(parent, wxID_ANY, (wxString)_("Video recording options")),
		m_Duration(duration)
	{
		wxArrayString framerates;
		framerates.Add(_T("10"));
		framerates.Add(_T("15"));
		framerates.Add(_T("25"));
		framerates.Add(_T("30"));
		framerates.Add(_T("60"));
		wxString framerateDefault = _T("25");

		wxArrayString bitrates;
		bitrates.Add(_T("500"));
		bitrates.Add(_T("800"));
		bitrates.Add(_T("1200"));
		bitrates.Add(_T("1600"));
		bitrates.Add(_T("3200"));
		wxString bitrateDefault = _T("1200");


		wxSizer* filenameSizer = new wxBoxSizer(wxHORIZONTAL);
		filenameSizer->Add(new wxTextCtrl(this, ID_FILENAME, _T("")), wxSizerFlags().Proportion(1));
		filenameSizer->Add(new wxButton(this, ID_FILECHOOSE, _("Browse...")));

		wxGridSizer* gridSizer = new wxGridSizer(4);

		gridSizer->Add(new wxStaticText(this, -1, _("Framerate (fps)")), wxSizerFlags().Right().Border(wxRIGHT, 4));
		gridSizer->Add(new wxComboBox(this, ID_FRAMERATE, framerateDefault, wxDefaultPosition, wxDefaultSize, framerates), wxSizerFlags().Proportion(1).Expand());
		// TODO: use a wxValidator to only allow digits

		gridSizer->Add(new wxStaticText(this, -1, _("Duration (seconds):")), wxSizerFlags().Right().Border(wxLEFT, 4));
		gridSizer->Add(new wxStaticText(this, ID_DURATION, wxString::Format(_T("%.1f"), m_Duration)), wxSizerFlags().Expand().Border(wxLEFT, 4));

		gridSizer->Add(new wxStaticText(this, -1, _("Bitrate (Kbit/s)")), wxSizerFlags().Right().Border(wxRIGHT, 4));
		gridSizer->Add(new wxComboBox(this, ID_BITRATE, bitrateDefault, wxDefaultPosition, wxDefaultSize, bitrates), wxSizerFlags().Proportion(1).Expand());

		gridSizer->Add(new wxStaticText(this, -1, _("Estimated file size:")), wxSizerFlags().Right().Border(wxRIGHT, 4));
		gridSizer->Add(new wxStaticText(this, ID_FILESIZE, L"???"), wxSizerFlags().Expand().Border(wxLEFT, 4));

		RecalculateSizes_();

		wxSizer* gridSizerBox = new wxStaticBoxSizer(wxVERTICAL, this, _("Compression options"));
		gridSizerBox->Add(gridSizer, wxSizerFlags().Border(wxALL, 10));

		wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
		wxButton* okButton = new wxButton(this, wxID_OK, _("Save"));
		okButton->SetDefault();
		buttonSizer->AddButton(okButton);
		buttonSizer->AddButton(new wxButton(this, wxID_CANCEL, _("Cancel")));
		buttonSizer->Realize();

		wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

		sizer->Add(filenameSizer, wxSizerFlags().Border(wxALL, 10).Expand());
		sizer->Add(gridSizerBox, wxSizerFlags().Border(wxALL, 10));
		sizer->Add(buttonSizer, wxSizerFlags().Border(wxALL, 10).Centre());

		SetSizer(sizer);
		sizer->SetSizeHints(this);
	}

	// Outputs:
	wxString m_Filename;
	unsigned long m_Framerate;
	unsigned long m_Bitrate;

protected:

	void OnButtonOK(wxCommandEvent& event)
	{
		wxTextCtrl* filenameWin = wxDynamicCast(FindWindow(ID_FILENAME), wxTextCtrl);
		wxComboBox* framerateWin = wxDynamicCast(FindWindow(ID_FRAMERATE), wxComboBox);
		wxComboBox* bitrateWin = wxDynamicCast(FindWindow(ID_BITRATE), wxComboBox);
		
		wxCHECK(filenameWin && framerateWin && bitrateWin, );

		m_Filename = filenameWin->GetValue();
		if (m_Filename.IsEmpty())
		{
			wxLogError(_("No filename specified."));
			return;
		}

		if (!framerateWin->GetValue().ToULong(&m_Framerate) ||
		    !bitrateWin->GetValue().ToULong(&m_Bitrate))
		{
			wxLogError(_("Invalid framerate/bitrate."));
			return;
		}

		event.Skip();	// janwas: wxDialog::OnOK has been removed in 2.8 wxw; see http://wxforum.shadonet.com/viewtopic.php?t=11103
	}

private:

	void RecalculateSizes(wxCommandEvent& WXUNUSED(event)) { RecalculateSizes_(); }
	void RecalculateSizes_()
	{
		wxComboBox* framerateWin = wxDynamicCast(FindWindow(ID_FRAMERATE), wxComboBox);
		wxComboBox* bitrateWin = wxDynamicCast(FindWindow(ID_BITRATE), wxComboBox);
		wxStaticText* filesizeWin = wxDynamicCast(FindWindow(ID_FILESIZE), wxStaticText);
		wxCHECK(framerateWin && bitrateWin && filesizeWin, );
		unsigned long framerate = 0, bitrate = 0;
		if (!framerateWin->GetValue().ToULong(&framerate) || !bitrateWin->GetValue().ToULong(&bitrate))
			return;

		int size = m_Duration * bitrate * 1000/8;
		wxString sizeStr;
		if (size < 1024*1024)           sizeStr = wxString::Format(_T("~%.0f KiB"), (float)size/1024);
		else if (size < 1024*1024*100)  sizeStr = wxString::Format(_T("~%.1f MiB"), (float)size/(1024*1024));
		else if (size < 1024*1024*1024) sizeStr = wxString::Format(_T("~%.0f MiB"), (float)size/(1024*1024));
		else                            sizeStr = wxString::Format(_T("~%.2f GiB"), (float)size/(1024*1024*1024));
		filesizeWin->SetLabel(sizeStr);
	}

	void FileBrowse(wxCommandEvent& WXUNUSED(event))
	{
		wxTextCtrl* filenameWin = wxDynamicCast(FindWindow(ID_FILENAME), wxTextCtrl);
		wxCHECK(filenameWin, );

		wxFileDialog dlg (this, wxFileSelectorPromptStr, filenameWin->GetValue(), filenameWin->GetValue(), _("MP4 files (*.mp4)|*.mp4"), wxSAVE);
		if (dlg.ShowModal() != wxID_OK)
			return;

		filenameWin->SetValue(dlg.GetPath());
	}

	float m_Duration;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(RecorderDialog, wxDialog)
	EVT_COMBOBOX(ID_FRAMERATE, RecorderDialog::RecalculateSizes)
	EVT_COMBOBOX(ID_BITRATE, RecorderDialog::RecalculateSizes)
	EVT_TEXT(ID_FRAMERATE, RecorderDialog::RecalculateSizes)
	EVT_TEXT(ID_BITRATE, RecorderDialog::RecalculateSizes)
	EVT_BUTTON(ID_FILECHOOSE, RecorderDialog::FileBrowse)
	EVT_BUTTON(wxID_OK, RecorderDialog::OnButtonOK)
END_EVENT_TABLE()


void callback(const sCinemaRecordCB* data, void* cbdata)
{
	VideoEncoder* enc = (VideoEncoder*)cbdata;
	enc->Frame(data->buffer);
}

void VideoRecorder::RecordCinematic(wxWindow* window, const wxString& trackName, float duration)
{
	RecorderDialog dlg(window, duration);
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxStopWatch sw;

//	int w = 320, h = 240;
	int w = 640, h = 480;

	VideoEncoder venc (dlg.m_Filename, dlg.m_Framerate, dlg.m_Bitrate, duration, w, h);

	qCinemaRecord qry((std::wstring)trackName.wc_str(), dlg.m_Framerate, duration, w, h, Callback<sCinemaRecordCB>(&callback, (void*)&venc));
	qry.Post();

	wxLogMessage(_("Finished recording (took %.1f seconds)\n"), sw.Time()/1000.f);
}
