// $Id: wxspinner.h,v 1.2 2004/06/19 12:56:09 philip Exp $

#include "wx/spinctrl.h"

class StyleSpinCtrl : public wxSpinCtrl
{
public:

	StyleSpinCtrl(wxWindow* parent, wxWindowID win_id, int min_val, int max_val, int initial_val)
		: wxSpinCtrl(parent, win_id, wxEmptyString, wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, min_val, max_val, initial_val)
	{
		SetValue(initial_val);
	}

	// Like GetValue, but guaranteed to be inside the range (Min..Max)
	// no matter what people type in
	int GetValidValue()
	{
		int Value = GetValue();
		int Min = GetMin();
		int Max = GetMax();
		return Value <= Min ? Min : Value >= Max ? Max : Value;
	}
};
