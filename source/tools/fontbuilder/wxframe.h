// $Id: wxframe.h,v 1.2 2004/06/19 12:56:09 philip Exp $

#include "wx/panel.h"
#include "wx/bitmap.h"
#include "wx/frame.h"
#include "wx/docview.h"

class BitmapPanel : public wxPanel
{
public:
	BitmapPanel(wxWindow* parent)
		: wxPanel(parent)
	{
		Bitmap = NULL;
	}

	~BitmapPanel()
	{
		delete Bitmap;
	}

	void OnPaint(wxPaintEvent& event);
	void SetBitmap(wxBitmap* bmp)
	{
		delete Bitmap;
		Bitmap = bmp;
	}

private:
	wxBitmap* Bitmap;

	DECLARE_EVENT_TABLE()
};



class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	
	void OnQuit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnAbout(wxCommandEvent& event);

	bool SaveDialog();
	void OnSave(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);

	void OnMRUFile(wxCommandEvent& event);

	void OnFontSelect0(wxCommandEvent& event);
	void OnFontSelect1(wxCommandEvent& event);
	void OnCharSelect(wxCommandEvent& event);

	void OnGeneratePreview(wxCommandEvent& event);
	void OnGenerateTexture(wxCommandEvent& event);

private:
	BitmapPanel* PreviewPanel;
	unsigned char* PreviewImageData;
	wxImage* PreviewImage;

	void GeneratePreview();
	int PreviewWidth, PreviewHeight;

	void GenerateTexture(wxString TextureFilename, wxString FontDefnFilename);

	void SaveSettings(wxString& filename);
	void LoadSettings(wxString& filename);

	wxString FontName0;
	wxString FontName1;
	wxString FontFilename0;
	wxString FontFilename1;

	wxString CharName;
	wxString CharFilename;

	wxFileHistory* RecentFiles;

	DECLARE_EVENT_TABLE()
};
