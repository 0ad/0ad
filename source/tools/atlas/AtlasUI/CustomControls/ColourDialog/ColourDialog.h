#ifndef COLOURDIALOG_H__
#define COLOURDIALOG_H__

class ColourDialog : public wxColourDialog
{
public:
	// Typical customColourConfigPath would be "Scenario Editor/LightingColour"
	ColourDialog(wxWindow* parent, const wxString& customColourConfigPath, const wxColour& defaultColour);

	int ShowModal();

private:
	wxString m_ConfigPath;
};

#endif // COLOURDIALOG_H__
