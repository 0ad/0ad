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

#include "VariationControl.h"

#include "ScenarioEditor/Tools/Common/ObjectSettings.h"

VariationControl::VariationControl(wxWindow* parent, Observable<ObjectSettings>& objectSettings)
: wxScrolledWindow(parent, -1),
m_ObjectSettings(objectSettings)
{
	m_Conn = m_ObjectSettings.RegisterObserver(1, &VariationControl::OnObjectSettingsChange, this);

	SetScrollRate(0, 5);

	m_Sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_Sizer);
}

// Event handler shared by all the combo boxes created by this window
void VariationControl::OnSelect(wxCommandEvent& evt)
{
	std::set<wxString> selections;

	// It's possible for a variant name to appear in multiple groups.
	// If so, assume that all the names in each group are the same, so
	// we don't have to worry about some impossible combinations (e.g.
	// one group "a,b", a second "b,c", and a third "c,a", where's there's
	// no set of selections that matches one (and only one) of each group).
	//
	// So... When a combo box is changed from 'a' to 'b', add 'b' to the new
	// selections and make sure any other combo boxes containing both 'a' and
	// 'b' no longer contain 'a'.

	wxComboBox* thisComboBox = wxDynamicCast(evt.GetEventObject(), wxComboBox);
	wxCHECK(thisComboBox != NULL, );
	wxString newValue = thisComboBox->GetValue();

	selections.insert(newValue);

	for (size_t i = 0; i < m_ComboBoxes.size(); ++i)
	{
		wxComboBox* comboBox = m_ComboBoxes[i];
		// If our newly selected value is used in another combobox, we want
		// that combobox to use the new value, so don't add its old value
		// to the list of selections
		if (comboBox->FindString(newValue) == wxNOT_FOUND)
			selections.insert(comboBox->GetValue());
	}

	m_ObjectSettings.SetActorSelections(selections);
	m_ObjectSettings.NotifyObserversExcept(m_Conn);
	RefreshObjectSettings();
}

void VariationControl::OnObjectSettingsChange(const ObjectSettings& settings)
{
	Freeze();

	const std::vector<ObjectSettings::Group>& variation = settings.GetActorVariation();

	// Creating combo boxes seems to be pretty expensive - so we create as
	// few as possible, by never deleting any.

	size_t oldCount = m_ComboBoxes.size();
	size_t newCount = variation.size();

	// If we have too many combo boxes, hide the excess ones
	for (size_t i = newCount; i < oldCount; ++i)
	{
		m_ComboBoxes[i]->Show(false);
	}

	for (size_t i = 0; i < variation.size(); ++i)
	{
		const ObjectSettings::Group& group = variation[i];

		if (i < oldCount)
		{
			// Already got enough boxes available, so use an old one
			wxComboBox* comboBox = m_ComboBoxes[i];
			// Replace the contents of the old combobox with the new data
			comboBox->Freeze();
			comboBox->Clear();
			comboBox->Append(group.variants);
			comboBox->SetValue(group.chosen);
			comboBox->Show(true);
			comboBox->Thaw();
		}
		else
		{
			// Create an initially empty combobox, because we can fill it
			// quicker than the default constructor can
			wxComboBox* combo = new wxComboBox(this, -1, wxEmptyString, wxDefaultPosition,
				wxSize(80, wxDefaultCoord), wxArrayString(), wxCB_READONLY);
			// Freeze it before adding all the values
			combo->Freeze();
			combo->Append(group.variants);
			combo->SetValue(group.chosen);
			combo->Thaw();
			// Add the on-select event handler
			combo->Connect(wxID_ANY, wxEVT_COMMAND_COMBOBOX_SELECTED,
				wxCommandEventHandler(VariationControl::OnSelect), NULL, this);
			// Add box to sizer and list
			m_Sizer->Add(combo, wxSizerFlags().Expand());
			m_ComboBoxes.push_back(combo);
		}
	}

	Layout();

	Thaw();

	// Make the scrollbars appear when appropriate
	FitInside();
}

void VariationControl::RefreshObjectSettings()
{
	const std::vector<ObjectSettings::Group>& variation = m_ObjectSettings.GetActorVariation();

	// For each group, set the corresponding combobox's value to the chosen one
	size_t i = 0;
	for (std::vector<ObjectSettings::Group>::const_iterator group = variation.begin();
		group != variation.end() && i < m_ComboBoxes.size();
		++group, ++i)
	{
		m_ComboBoxes[i]->SetValue(group->chosen);
	}
}
