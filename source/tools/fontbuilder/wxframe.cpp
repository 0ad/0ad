// $Id: wxframe.cpp,v 1.7 2004/08/10 15:51:06 philip Exp $

#include "stdafx.h"

#include "wx/menu.h"
#include "wx/config.h"
#include "wx/sizer.h"
#include "wx/stattext.h"
#include "wx/button.h"
#include "wx/textctrl.h"
#include "wx/filedlg.h"
#include "wx/msgdlg.h"
#include "wx/image.h"
#include "wx/wxexpr.h"
#include "wx/log.h"
#include "wx/filename.h"
#include "wx/progdlg.h"
#include "wx/dcbuffer.h"
#include "wx/checkbox.h"

#include "wxframe.h"
#include "wxconfig.h"
#include "wxspinner.h"

#include "platform/fontselect.h"

#include "imagemanip.h"
#include "font.h"
#include "packer.h"
#include "filemanip.h"

#include "version.h"

bool Changes = false;

enum
{
	ID_Quit = wxID_HIGHEST+1,
	ID_About,
	ID_New,
	ID_Save,
	ID_Open,
	ID_Recent,
	
	ID_GeneratePreview,
	ID_GenerateTexture,

	ID_FontSelect0,
	ID_FontSelect1,
	ID_CharSelect,

	// For accessing values:
	ID_PreviewText,
	ID_Style_Size,
	ID_Style_Boldness,
	ID_Style_Italicness,
	ID_Style_Tracking,
	ID_Style_Leading,
	ID_Style_Hinting
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_MENU(ID_Quit,  MainFrame::OnQuit)
	EVT_MENU(ID_About, MainFrame::OnAbout)
	EVT_MENU(ID_Save,  MainFrame::OnSave)
	EVT_MENU(ID_Open,  MainFrame::OnOpen)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile)

	EVT_BUTTON(ID_FontSelect0, MainFrame::OnFontSelect0)
	EVT_BUTTON(ID_FontSelect1, MainFrame::OnFontSelect1)
	EVT_BUTTON(ID_CharSelect, MainFrame::OnCharSelect)

	EVT_BUTTON(ID_GeneratePreview, MainFrame::OnGeneratePreview)
	EVT_BUTTON(ID_GenerateTexture, MainFrame::OnGenerateTexture)

	EVT_CLOSE(MainFrame::OnClose)
END_EVENT_TABLE()



BEGIN_EVENT_TABLE(BitmapPanel, wxPanel)
	EVT_PAINT(BitmapPanel::OnPaint)
END_EVENT_TABLE()


MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	RecentFiles = new wxFileHistory(5);

	wxMenu* menuFile = new wxMenu;

	//menuFile->Append(ID_New, wxT("&New"));
	menuFile->Append(ID_Open, wxT("&Open..."));
	menuFile->Append(ID_Save, wxT("&Save..."));
	menuFile->AppendSeparator();
	wxMenu* menuRecent = new wxMenu;
	menuFile->Append(ID_Recent, wxT("&Recent files"), menuRecent);
	menuFile->AppendSeparator();
	menuFile->Append(ID_About, wxT("&About"));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Quit, wxT("E&xit"));

	RecentFiles->UseMenu(menuRecent);

	wxConfig* config = (wxConfig*)wxConfig::Get();
	RecentFiles->Load(*config);

	wxMenuBar* menuBar = new wxMenuBar;
	menuBar->Append(menuFile, wxT("&File"));

	SetMenuBar(menuBar);

	CreateStatusBar();

	// For textboxes that need to display Unicode text:
	wxFont ArialUnicodeFont (12, wxDEFAULT, wxNORMAL, wxNORMAL, false, wxT("Arial Unicode MS"), wxFONTENCODING_SYSTEM);

	// Main panel that fills the whole window
	wxPanel* Panel = new wxPanel(this);

	// Split the window into three main rows - controls, preview text, graphic
	wxBoxSizer* OutlineSizer = new wxBoxSizer(wxVERTICAL);

	// Split the controls part into three main columns: font, styles, actions
	wxBoxSizer* ControlSizer = new wxBoxSizer(wxHORIZONTAL);

	// Get some default values
	FontSelectorDialog::DefaultFonts(FontName0, FontFilename0, FontName1, FontFilename1);
	CharFilename = wxT("<ascii>");
	CharName = wxT("Basic ASCII");

	wxFlexGridSizer* FontSizer = new wxFlexGridSizer(2);
	FontSizer->AddGrowableCol(0);

	FontSizer->Add(new wxStaticText(Panel, -1, wxT("Primary font:")), 0, wxALIGN_RIGHT | wxALL, 2);
	FontSizer->Add(new wxButton(Panel, ID_FontSelect0, FontName0), 0, wxGROW | wxALL, 2);

	FontSizer->Add(new wxStaticText(Panel, -1, wxT("Secondary font:")), 0, wxALIGN_RIGHT | wxALL, 2);
	FontSizer->Add(new wxButton(Panel, ID_FontSelect1, FontName1), 0, wxGROW | wxALL, 2);

	FontSizer->Add(new wxStaticText(Panel, -1, wxT("Character list:")), 0, wxALIGN_RIGHT | wxALL, 2);
	FontSizer->Add(new wxButton(Panel, ID_CharSelect, CharName), 0, wxGROW | wxALL, 2);

	ControlSizer->Add(FontSizer, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP, 8);

	wxFlexGridSizer* StyleSizer = new wxFlexGridSizer(2);
	StyleSizer->AddGrowableCol(0);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Size:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new StyleSpinCtrl(Panel, ID_Style_Size, 1, 1024, 18), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Boldness:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new StyleSpinCtrl(Panel, ID_Style_Boldness, 0, 16, 0), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Italicness:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new StyleSpinCtrl(Panel, ID_Style_Italicness, -16, 16, 0), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Tracking:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new StyleSpinCtrl(Panel, ID_Style_Tracking, -256, 256, 0), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Leading:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new StyleSpinCtrl(Panel, ID_Style_Leading, -256, 256, 0), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	StyleSizer->Add(new wxStaticText(Panel, -1, wxT("Disable hinting:")), 0, wxALIGN_RIGHT | wxLEFT | wxRIGHT, 2);
	StyleSizer->Add(new wxCheckBox(Panel, ID_Style_Hinting, wxT("")), 0, wxGROW | wxLEFT | wxRIGHT, 2);

	ControlSizer->Add(StyleSizer, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER, 8);

	wxBoxSizer* GenerateSizer = new wxBoxSizer(wxVERTICAL);
	GenerateSizer->Add(new wxButton(Panel, ID_GeneratePreview, wxT("Preview font")), 1, wxGROW | wxALL | wxALIGN_CENTER, 4);
	GenerateSizer->Add(new wxButton(Panel, ID_GenerateTexture, wxT("Generate texture")), 1, wxGROW | wxALL | wxALIGN_CENTER, 4);
	ControlSizer->Add(GenerateSizer, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP, 8);

	OutlineSizer->Add(ControlSizer);

	OutlineSizer->Add(new wxStaticText(Panel, -1, wxT("Text to display in preview:")), 0, wxALL, 2);

	// Create a textbox containing "abcABCfunnysymbols"
	wxString Font_string = wxT("abcABC"); Font_string += wchar_t(225); Font_string += wchar_t(223); Font_string += wchar_t(231); Font_string += wchar_t(779);Font_string += wchar_t(9812);
	wxTextCtrl* PreviewTextBox = new wxTextCtrl(Panel, ID_PreviewText, Font_string, wxDefaultPosition, wxSize(300, 80), wxTE_MULTILINE);
	PreviewTextBox->SetFont(ArialUnicodeFont);

	OutlineSizer->Add(PreviewTextBox, 0, wxGROW | wxLEFT | wxRIGHT, 4);

	PreviewPanel = new BitmapPanel(Panel);
	OutlineSizer->Add(PreviewPanel, 1, wxGROW);

	Panel->SetSizer(OutlineSizer);

	PreviewImageData = NULL;
	PreviewImage  = NULL;
	PreviewWidth  = 512;
	PreviewHeight = 256;
}


void MainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(false);
}

void MainFrame::OnClose(wxCloseEvent& event)
{
	if (Changes)
	{
		wxMessageDialog dlg (this, wxT("The font settings have been altered.\n\nDo you want to save the changes?"), wxT("Font Builder"), wxYES_NO|wxCANCEL | wxICON_QUESTION);
		int ret = dlg.ShowModal();
		if (ret == wxID_CANCEL)
		{
			event.Veto();
			return;
		}
		else if (ret == wxID_YES)
		{
			if (! SaveDialog())
			{
				event.Veto();
				return;
			}
		}
	}

	wxConfig* config = (wxConfig*)wxConfig::Get();
	RecentFiles->Save(*config);
	delete RecentFiles;

	delete PreviewImage;
	delete[] PreviewImageData;

	event.Skip();
}


void MainFrame::LoadSettings(wxString& filename)
{
	wxExprDatabase db;

	db.Read(filename);

	wxTextCtrl* PreviewTextCtrl		= (wxTextCtrl*)wxWindow::FindWindowById(ID_PreviewText);
	StyleSpinCtrl* BoldnessCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Boldness);
	StyleSpinCtrl* ItalicnessCtrl	= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Italicness);
	StyleSpinCtrl* SizeCtrl			= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Size);
	StyleSpinCtrl* TrackingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Tracking);
	StyleSpinCtrl* LeadingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Leading);
	wxCheckBox* HintingCtrl			= (wxCheckBox*)wxWindow::FindWindowById(ID_Style_Hinting);
	wxButton* FontSelect0 = (wxButton*)wxWindow::FindWindowById(ID_FontSelect0);
	wxButton* FontSelect1 = (wxButton*)wxWindow::FindWindowById(ID_FontSelect1);
	wxButton* CharSelect = (wxButton*)wxWindow::FindWindowById(ID_CharSelect);

	db.BeginFind();

	wxExpr *Settings = db.FindClauseByFunctor(wxT("Settings"));
	Settings->GetAttributeValue(wxT("FontName0"), FontName0);
	Settings->GetAttributeValue(wxT("FontName1"), FontName1);
	Settings->GetAttributeValue(wxT("FontFilename0"), FontFilename0);
	Settings->GetAttributeValue(wxT("FontFilename1"), FontFilename1);
	FontSelect0->SetLabel(FontName0);
	FontSelect1->SetLabel(FontName1);

	Settings->GetAttributeValue(wxT("CharName"), CharName);
	Settings->GetAttributeValue(wxT("CharFilename"), CharFilename);
	CharSelect->SetLabel(CharName);

	int t;
	Settings->GetAttributeValue(wxT("Boldness"),	t); BoldnessCtrl->SetValue(t);
	Settings->GetAttributeValue(wxT("Italicness"),	t); ItalicnessCtrl->SetValue(t);
	Settings->GetAttributeValue(wxT("Size"),		t); SizeCtrl->SetValue(t);
	Settings->GetAttributeValue(wxT("Tracking"),	t); TrackingCtrl->SetValue(t);
	Settings->GetAttributeValue(wxT("Leading"),		t); LeadingCtrl->SetValue(t);
	Settings->GetAttributeValue(wxT("Hinting"),		t); HintingCtrl->SetValue(t ? true : false);


	// Convert back to UTF16 from hex, because wxExpr doesn't like non-ASCII

	wxString PreviewTextHex;
	Settings->GetAttributeValue(wxT("PreviewText"), PreviewTextHex);

	wxString PreviewText;
	for (size_t i=0; i<PreviewTextHex.Length(); i += 4)
	{
		wchar_t b[5];
		memcpy(b, &PreviewTextHex[i], sizeof(wchar_t)*4);
		b[4] = 0;
		PreviewText += (wxChar)wcstoul(b, NULL, 16);
	}
	PreviewTextCtrl->SetValue(PreviewText);


	Changes = false;
}

void MainFrame::SaveSettings(wxString& filename)
{
	wxExprDatabase db;

	wxTextCtrl* PreviewTextCtrl		= (wxTextCtrl*)wxWindow::FindWindowById(ID_PreviewText);
	StyleSpinCtrl* BoldnessCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Boldness);
	StyleSpinCtrl* ItalicnessCtrl	= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Italicness);
	StyleSpinCtrl* SizeCtrl			= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Size);
	StyleSpinCtrl* TrackingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Tracking);
	StyleSpinCtrl* LeadingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Leading);
	wxCheckBox* HintingCtrl			= (wxCheckBox*)wxWindow::FindWindowById(ID_Style_Hinting);

	wxExpr *Settings = new wxExpr(wxT("Settings"));
	Settings->AddAttributeValueString(wxT("FontName0"),		FontName0);
	Settings->AddAttributeValueString(wxT("FontName1"),		FontName1);
	Settings->AddAttributeValueString(wxT("FontFilename0"),	FontFilename0);
	Settings->AddAttributeValueString(wxT("FontFilename1"),	FontFilename1);
	Settings->AddAttributeValueString(wxT("CharName"),		CharName);
	Settings->AddAttributeValueString(wxT("CharFilename"),	CharFilename);
	Settings->AddAttributeValue(wxT("Boldness"),	(long)BoldnessCtrl->GetValidValue());
	Settings->AddAttributeValue(wxT("Italicness"),	(long)ItalicnessCtrl->GetValidValue());
	Settings->AddAttributeValue(wxT("Size"),		(long)SizeCtrl->GetValidValue());
	Settings->AddAttributeValue(wxT("Tracking"),	(long)TrackingCtrl->GetValidValue());
	Settings->AddAttributeValue(wxT("Leading"),		(long)LeadingCtrl->GetValidValue());
	Settings->AddAttributeValue(wxT("Hinting"),		(long)HintingCtrl->GetValue());

	// Convert UTF16 to hex, because wxExpr doesn't like non-ASCII
	wxString PreviewText = PreviewTextCtrl->GetValue();
	wxString PreviewTextHex;
	for (size_t i=0; i<PreviewText.Length(); ++i)
	{
		wchar_t b[5];
		swprintf(b, 5, wxT("%04x"), PreviewText[i]);
		PreviewTextHex += wxString(b, 4);
	}
	Settings->AddAttributeValueString(wxT("PreviewText"), PreviewTextHex);

	db.Append(Settings);

	db.Write(filename);

	Changes = false;
}


void MainFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog Dlg (this, wxT("Open font settings"), ConfigGet(wxT("FSF path")), wxEmptyString, wxT("Font settings (*.fst)|*.fst|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY);
	if (Dlg.ShowModal() == wxID_OK)
	{
		RecentFiles->AddFileToHistory(Dlg.GetPath());
		wxString path = Dlg.GetPath();
		LoadSettings(path);
		ConfigSet(wxT("FSF path"), Dlg.GetDirectory());
	}
}

bool MainFrame::SaveDialog()
{
	wxFileDialog Dlg (this, wxT("Save current settings"), ConfigGet(wxT("FSF path")), wxEmptyString, wxT("Font settings (*.fst)|*.fst|All files (*.*)|*.*"), wxSAVE | wxOVERWRITE_PROMPT);
	if (Dlg.ShowModal() == wxID_OK)
	{
		RecentFiles->AddFileToHistory(Dlg.GetPath());
		wxString path = Dlg.GetPath();
		SaveSettings(path);
		ConfigSet(wxT("FSF path"), Dlg.GetDirectory());
		return true;
	}
	return false;
}


void MainFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
	SaveDialog();
}


void MainFrame::OnMRUFile(wxCommandEvent& event)
{
	wxString filename = RecentFiles->GetHistoryFile(event.GetId() - wxID_FILE1);
	LoadSettings(filename);
	RecentFiles->AddFileToHistory(filename);
}



void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxString::Format(wxT("Unicode Font Builder %s - created by Philip Taylor for WildFire Games"), version.c_str()), wxT("About"), wxOK | wxICON_INFORMATION );
}


void MainFrame::OnGeneratePreview(wxCommandEvent& WXUNUSED(event))
{
	GeneratePreview();
}

void MainFrame::GeneratePreview()
{
	if (FontFilename0.IsEmpty() || FontFilename1.IsEmpty())
		return;

	PreviewPanel->GetSize(&PreviewWidth, &PreviewHeight);

	delete[] PreviewImageData;
	PreviewImageData = GenerateImage(PreviewWidth, PreviewHeight);

	// Find all the relevant controls (slightly nicer than storing lots of pointers in the class)
	wxTextCtrl* PreviewTextCtrl		= (wxTextCtrl*)wxWindow::FindWindowById(ID_PreviewText);
	StyleSpinCtrl* BoldnessCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Boldness);
	StyleSpinCtrl* ItalicnessCtrl	= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Italicness);
	StyleSpinCtrl* SizeCtrl			= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Size);
	StyleSpinCtrl* TrackingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Tracking);
	StyleSpinCtrl* LeadingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Leading);
	wxCheckBox* HintingCtrl			= (wxCheckBox*)wxWindow::FindWindowById(ID_Style_Hinting);

	try
	{

		FontRenderer Font(
			FontFilename0.ToAscii(),
			FontFilename1.ToAscii(),
			SizeCtrl->GetValidValue(),
			false,
			!HintingCtrl->GetValue() );

		Font.Boldness = BoldnessCtrl->GetValidValue();
		Font.Italicness = 5 * ItalicnessCtrl->GetValidValue();
		Font.Tracking = TrackingCtrl->GetValidValue();
		Font.Leading = LeadingCtrl->GetValidValue();
		Font.Outline = false;

		int x = 16, y = Font.GetLineSpacing();

		wxString PreviewText = PreviewTextCtrl->GetValue();
		for (size_t i = 0; i < PreviewText.Length(); ++i)
		{
			if (PreviewText[i] == wxT('\n'))
			{
				x = 16;
				y += Font.GetLineSpacing();
			}
			else
			{
				Font.LoadGlyph(PreviewText[i]);
				Font.RenderGlyph(PreviewImageData, x, y, PreviewWidth, PreviewHeight, PreviewWidth*3, false);
			}
		}
	}
	catch (const char* m) {
		wxLogError(wxString::Format(wxT("Failed to generate preview: %s"), wxString::FromAscii(m).c_str()));
		return;
	}

	delete PreviewImage;
	PreviewImage = new wxImage(PreviewWidth, PreviewHeight, PreviewImageData, true);
	PreviewPanel->SetBitmap(new wxBitmap(PreviewImage));

	PreviewPanel->Refresh();

	Changes = true;
}


void MainFrame::OnGenerateTexture(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog Dlg (this, wxT("Save font definition and texture"), ConfigGet(wxT("FNT path")), wxEmptyString, wxT("Font definition files (*.fnt)|*.fnt|All files (*.*)|*.*"), wxSAVE | wxOVERWRITE_PROMPT);
	if (Dlg.ShowModal() == wxID_OK)
	{
		wxFileName FontDefnFilename (Dlg.GetPath());
		wxFileName TextureFilename = FontDefnFilename;
		TextureFilename.SetExt(wxT("tga"));
		GenerateTexture(TextureFilename.GetFullPath(), FontDefnFilename.GetFullPath());
		ConfigSet(wxT("FNT path"), TextureFilename.GetPath());
	}
}


bool ProgressDialogCallback(float Progress, wxString Msg, void* data) {
	return ! ((wxProgressDialog *)data)->Update((int)(Progress*1024.0), Msg);
}

void MainFrame::GenerateTexture(wxString TextureFilename, wxString FontDefnFilename)
{
	if (FontFilename0.IsEmpty() || FontFilename1.IsEmpty())
		return;

	// Find all the relevant controls (slightly nicer than storing lots of pointers in the class)
	StyleSpinCtrl* BoldnessCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Boldness);
	StyleSpinCtrl* ItalicnessCtrl	= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Italicness);
	StyleSpinCtrl* SizeCtrl			= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Size);
	StyleSpinCtrl* TrackingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Tracking);
	StyleSpinCtrl* LeadingCtrl		= (StyleSpinCtrl*)wxWindow::FindWindowById(ID_Style_Leading);
	wxCheckBox* HintingCtrl			= (wxCheckBox*)wxWindow::FindWindowById(ID_Style_Hinting);


	// Work out what characters need to be included in the texture
	
	std::set<wchar_t> Chars;
	if (CharFilename == wxT("<ascii>"))
	{
		for (wchar_t c = 0x20; c < 0x7f; ++c)
			Chars.insert(c);
	}
	else
	{
		try
		{
			Chars = AnalyseChars(CharFilename);
		}
		catch (const char* m) {
			wxLogError(wxString::Format(wxT("Failed to analyse character file: %s"), wxString::FromAscii(m).c_str()));
			return;
		}
	}
	// Add the 'missing' symbol (if it's not missing, it'll use a ? instead)
	Chars.insert(0xFFFD);

	// Generate the texture
	try
	{
		FontRenderer Font(
			FontFilename0.ToAscii(),
			FontFilename1.ToAscii(),
			SizeCtrl->GetValidValue(),
			false,
			!HintingCtrl->GetValue() );

		Font.Boldness = BoldnessCtrl->GetValidValue();
		Font.Italicness = 5 * ItalicnessCtrl->GetValidValue();
		Font.Tracking = TrackingCtrl->GetValidValue();
		Font.Leading = LeadingCtrl->GetValidValue();
		Font.Outline = false;

		PackedFont Packed (&Font, Chars);

		wxProgressDialog ProgressDialog (wxT("Progress"), wxT("Thinking..."), 1024, this, wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME);
		
		try
		{
			Packed.Generate(&ProgressDialogCallback, &ProgressDialog);
		}
		catch (const char* m) {
			wxLogError(wxString::Format(wxT("Failed to generate texture: %s"), wxString::FromAscii(m).c_str()));
			return;
		}

		ProgressDialog.Destroy();

		PreviewWidth = Packed.TextureWidth;
		PreviewHeight = Packed.TextureHeight;
		delete[] PreviewImageData;
		PreviewImageData = new unsigned char[PreviewWidth*PreviewHeight*3];
		memcpy(PreviewImageData, Packed.TextureData, PreviewWidth*PreviewHeight*3);

		delete PreviewImage;
		PreviewImage = new wxImage(PreviewWidth, PreviewHeight, PreviewImageData, true);
		PreviewPanel->SetBitmap(new wxBitmap(PreviewImage));

		PreviewPanel->Refresh();

		wxFFile TGAFile(TextureFilename, "wb");
		if (! TGAFile.IsOpened())
			throw "Error opening texture file for output";
		RGB_OutputGreyscaleTGA(PreviewImageData, PreviewWidth, PreviewHeight, PreviewWidth*3, TGAFile);
		TGAFile.Close();

		wxFFile FntFile(FontDefnFilename, "w");
		if (! FntFile.IsOpened())
			throw "Error opening font definition file for output";
		FntFile.Write(Packed.FontDefinition);
		FntFile.Close();
	}
	catch (const char* m) {
		wxLogError(wxString::Format(wxT("Failed to generate texture: %s"), wxString::FromAscii(m).c_str()));
		return;
	}

	Changes = true;
}


void MainFrame::OnFontSelect0(wxCommandEvent& event)
{
#ifdef _WIN32
	FontSelectorDialog Dlg(this);
	if (Dlg.ShowModal() == wxID_OK)
	{
		FontFilename0 = Dlg.FontFilename;
		FontName0 = Dlg.FontName;
		((wxButton*)event.GetEventObject()) -> SetLabel(FontName0);
	}
#else
	wxFileDialog Dlg (this, wxT("Select a font file"), wxEmptyString, wxEmptyString, wxT("TTF files (*.ttf)|*.ttf|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY);
	if (Dlg.ShowModal() == wxID_OK)
	{
		FontFilename0 = Dlg.GetPath();
		FontName0 = Dlg.GetFilename();
		((wxButton*)event.GetEventObject()) -> SetLabel(FontName0);
	}
#endif
}

void MainFrame::OnFontSelect1(wxCommandEvent& event)
{
#ifdef _WIN32
	FontSelectorDialog Dlg(this);
	if (Dlg.ShowModal() == wxID_OK)
	{
		FontFilename1 = Dlg.FontFilename;
		FontName1 = Dlg.FontName;
		((wxButton*)event.GetEventObject()) -> SetLabel(FontName1);
	}
#else
	wxFileDialog Dlg (this, wxT("Select a font file"), wxEmptyString, wxEmptyString, wxT("TTF files (*.ttf)|*.ttf|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY);
	if (Dlg.ShowModal() == wxID_OK)
	{
		FontFilename1 = Dlg.GetPath();
		FontName1 = Dlg.GetFilename();
		((wxButton*)event.GetEventObject()) -> SetLabel(FontName1);
	}
#endif
}

void MainFrame::OnCharSelect(wxCommandEvent& event)
{
	wxFileDialog Dlg(this, wxT("UTF16 text file containing desired characters"), ConfigGet(wxT("Charset path")), wxEmptyString, wxT("Text files (*.txt)|*.txt|All files (*.*)|*.*"), wxOPEN | wxHIDE_READONLY);
	if (Dlg.ShowModal() == wxID_OK)
	{
		CharFilename = Dlg.GetPath();
		CharName = Dlg.GetFilename();
		((wxButton*)event.GetEventObject()) -> SetLabel(CharName);
		ConfigSet(wxT("Charset path"), Dlg.GetDirectory());
	}
}



void BitmapPanel::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);
	if (Bitmap)
		dc.DrawBitmap(*Bitmap, 0, 0, false);
}
