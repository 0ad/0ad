#ifndef SIDEBAR_H__
#define SIDEBAR_H__

class Sidebar : public wxPanel
{
public:
	Sidebar(wxWindow* parent);

protected:
	wxSizer* m_MainSizer; // vertical box sizer
};

#endif // SIDEBAR_H__
