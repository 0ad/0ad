class ActionButton : public wxButton
{
	typedef void (*actionFun)();

public:
	ActionButton(wxWindow *parent,
	             const wxString& label,
	             actionFun fun,
	             const wxSize& size = wxDefaultSize,
	             long style = 0)
		: wxButton(parent, wxID_ANY, label, wxDefaultPosition, size, style),
		m_Fun(fun)
	{
	}

protected:
	virtual void OnClick(wxCommandEvent&)
	{
		m_Fun();
	}

private:
	actionFun m_Fun;
	DECLARE_EVENT_TABLE();
};
