#include "stdafx.h"

#include "Brushes.h"

#include "GameInterface/Messages.h"

Brush::Brush()
: m_BrushShape(SQUARE), m_BrushSize(4), m_IsActive(false)
{
}

Brush::~Brush()
{
}

int Brush::GetWidth()
{
	switch (m_BrushShape)
	{
	case CIRCLE:
		return m_BrushSize*2 + 1;
	case SQUARE:
		return m_BrushSize;
	default:
		wxFAIL;
		return -1;
	}
}

int Brush::GetHeight()
{
	switch (m_BrushShape)
	{
	case CIRCLE:
		return m_BrushSize*2 + 1;
	case SQUARE:
		return m_BrushSize;
	default:
		wxFAIL;
		return -1;
	}
}

float* Brush::GetNewedData()
{
	int width = GetWidth();
	int height = GetHeight();
	
	float* data = new float[width*height];
	
	switch (m_BrushShape)
	{
	case CIRCLE:
		{
			int i = 0;
			for (int y = -m_BrushSize; y <= m_BrushSize; ++y)
			{
				for (int x = -m_BrushSize; x <= m_BrushSize; ++x)
				{
					// TODO: proper circle rasterization, with variable smoothness etc
					if (x*x + y*y <= m_BrushSize*m_BrushSize - 4)
						data[i++] = 1.f;
					else if (x*x + y*y <= m_BrushSize*m_BrushSize)
						data[i++] = 0.5f;
					else
						data[i++] = 0.f;
				}
			}
			break;
		}

	case SQUARE:
		{
			int i = 0;
			for (int y = 0; y < height; ++y)
				for (int x = 0; x < width; ++x)
					data[i++] = 1.f;
			break;
		}
	}

	return data;
}

//////////////////////////////////////////////////////////////////////////

class BrushShapeCtrl : public wxRadioBox
{
public:
	BrushShapeCtrl(wxWindow* parent, wxArrayString& shapes, Brush& brush)
		: wxRadioBox(parent, wxID_ANY, _("Shape"), wxDefaultPosition, wxDefaultSize, shapes, 0, wxRA_SPECIFY_ROWS),
		m_Brush(brush)
	{
		SetSelection(m_Brush.m_BrushShape);
	}

private:
	Brush& m_Brush;

	void OnChange(wxCommandEvent& WXUNUSED(evt))
	{
		m_Brush.m_BrushShape = (Brush::BrushShape)GetSelection();
		m_Brush.Send();
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(BrushShapeCtrl, wxRadioBox)
	EVT_RADIOBOX(wxID_ANY, BrushShapeCtrl::OnChange)
END_EVENT_TABLE()


class BrushSizeCtrl: public wxSpinCtrl
{
public:
	BrushSizeCtrl(wxWindow* parent, Brush& brush)
		: wxSpinCtrl(parent, wxID_ANY, wxString::Format(_T("%d"), brush.m_BrushSize), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 50, brush.m_BrushSize),
		m_Brush(brush)
	{
	}

private:
	Brush& m_Brush;

	void OnChange(wxSpinEvent& WXUNUSED(evt))
	{
		m_Brush.m_BrushSize = GetValue();
		m_Brush.Send();
	}

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(BrushSizeCtrl, wxSpinCtrl)
	EVT_SPINCTRL(wxID_ANY, BrushSizeCtrl::OnChange)
END_EVENT_TABLE()



void Brush::CreateUI(wxWindow* parent, wxSizer* sizer)
{
	wxArrayString shapes; // Must match order of BrushShape enum
	shapes.Add(_("Square"));
	shapes.Add(_("Circle"));
	// TODO (maybe): get rid of the extra static box, by not using wxRadioBox
	sizer->Add(new BrushShapeCtrl(parent, shapes, *this));

	wxSizer* spinnerSizer = new wxBoxSizer(wxHORIZONTAL);
	spinnerSizer->Add(new wxStaticText(parent, wxID_ANY, _("Size")));
	spinnerSizer->Add(new BrushSizeCtrl(parent, *this));
	sizer->Add(spinnerSizer);
}

void Brush::MakeActive()
{
	if (g_Brush_CurrentlyActive)
		g_Brush_CurrentlyActive->m_IsActive = false;

	g_Brush_CurrentlyActive = this;
	m_IsActive = true;

	Send();
}

void Brush::Send()
{
	if (m_IsActive)
		POST_COMMAND(SetBrush(GetWidth(), GetHeight(), GetNewedData()));
}


Brush g_Brush_Elevation;
Brush* g_Brush_CurrentlyActive = NULL;
