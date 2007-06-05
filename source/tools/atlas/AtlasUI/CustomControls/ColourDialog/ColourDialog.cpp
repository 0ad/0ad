#include "precompiled.h"

#include "wx/regex.h"
#include "wx/config.h"

#include "ColourDialog.h"

ColourDialog::ColourDialog(wxWindow* parent, const wxString& customColourConfigPath, const wxColour& defaultColour)
: wxColourDialog(parent), m_ConfigPath(customColourConfigPath)
{
	GetColourData().SetColour(defaultColour);

	// Load custom colours from the config database

	wxRegEx re (_T("(\\d+) (\\d+) (\\d+)"), wxRE_ADVANCED);

	wxConfigBase* cfg = wxConfigBase::Get(false);
	if (cfg)
	{
		for (int i = 0; i < 16; ++i)
		{
			wxString customColour;
			if (cfg->Read(wxString::Format(_T("%s%d"), m_ConfigPath.c_str(), i), &customColour)
				&& re.Matches(customColour))
			{
				long r, g, b;
				re.GetMatch(customColour, 1).ToLong(&r);
				re.GetMatch(customColour, 2).ToLong(&g);
				re.GetMatch(customColour, 3).ToLong(&b);
				GetColourData().SetCustomColour(i, wxColour(r, g, b));
			}
		}
	}
}

int ColourDialog::ShowModal()
{
	int ret = wxColourDialog::ShowModal();
	if (ret == wxID_OK)
	{
		// Save all the custom colours back into the config database

		wxConfigBase* cfg = wxConfigBase::Get(false);
		if (cfg)
		{
			for (int i = 0; i < 16; ++i)
			{
				wxString name = wxString::Format(_T("%s%d"), m_ConfigPath.c_str(), i);
				wxColour colour = GetColourData().GetCustomColour(i);

				wxString customColour = wxString::Format(_T("%d %d %d"), colour.Red(), colour.Green(), colour.Blue());
				cfg->Write(name, customColour);
			}
		}
	}

	return ret;
}
