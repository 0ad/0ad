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
