#include "stdafx.h"

#include "ActionButton.h"

BEGIN_EVENT_TABLE(ActionButton, wxButton)
	EVT_BUTTON(wxID_ANY, ActionButton::OnClick)
END_EVENT_TABLE()

