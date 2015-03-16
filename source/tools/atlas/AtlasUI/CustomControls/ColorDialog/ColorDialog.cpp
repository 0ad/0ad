/* Copyright (C) 2014 Wildfire Games.
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

#include "wx/regex.h"
#include "wx/config.h"

#include "ColorDialog.h"

ColorDialog::ColorDialog(wxWindow* parent, const wxString& customColorConfigPath, const wxColor& defaultColor)
: wxColourDialog(parent), m_ConfigPath(customColorConfigPath)
{
	GetColourData().SetColour(defaultColor);

	// Load custom colors from the config database

	wxRegEx re (_T("([0-9]+) ([0-9]+) ([0-9]+)"), wxRE_EXTENDED);

	wxConfigBase* cfg = wxConfigBase::Get(false);
	if (cfg)
	{
		for (int i = 0; i < 16; ++i)
		{
			wxString customColor;
			if (cfg->Read(wxString::Format(_T("%s%d"), m_ConfigPath.c_str(), i), &customColor)
				&& re.Matches(customColor))
			{
				long r, g, b;
				re.GetMatch(customColor, 1).ToLong(&r);
				re.GetMatch(customColor, 2).ToLong(&g);
				re.GetMatch(customColor, 3).ToLong(&b);
				GetColourData().SetCustomColour(i, wxColor(r, g, b));
			}
		}
	}
}

int ColorDialog::ShowModal()
{
	int ret = wxColourDialog::ShowModal();
	if (ret == wxID_OK)
	{
		// Save all the custom colors back into the config database

		wxConfigBase* cfg = wxConfigBase::Get(false);
		if (cfg)
		{
			for (int i = 0; i < 16; ++i)
			{
				wxString name = wxString::Format(_T("%s%d"), m_ConfigPath.c_str(), i);
				wxColor color = GetColourData().GetCustomColour(i);

				if (color.IsOk())
				{
					wxString customColor = wxString::Format(_T("%d %d %d"), color.Red(), color.Green(), color.Blue());
					cfg->Write(name, customColor);
				}
			}
		}
	}

	return ret;
}
