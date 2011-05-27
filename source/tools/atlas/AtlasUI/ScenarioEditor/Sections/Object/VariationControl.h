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

#ifndef INCLUDED_VARIATIONCONTROL
#define INCLUDED_VARIATIONCONTROL

#include "General/Observable.h"

class ObjectSettings;

class VariationControl : public wxScrolledWindow
{
public:
	VariationControl(wxWindow* parent, Observable<ObjectSettings>& objectSettings);

private:
	void OnSelect(wxCommandEvent& evt);
	void OnObjectSettingsChange(const ObjectSettings& settings);
	void RefreshObjectSettings();

	ObservableScopedConnection m_Conn;

	Observable<ObjectSettings>& m_ObjectSettings;
	std::vector<wxComboBox*> m_ComboBoxes;
	wxSizer* m_Sizer;
};

#endif // INCLUDED_VARIATIONCONTROL
