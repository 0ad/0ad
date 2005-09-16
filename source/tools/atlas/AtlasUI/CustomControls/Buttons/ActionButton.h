class ActionButton : public wxButton
{
	typedef void (*actionFun)(void* data);

public:
	ActionButton(wxWindow *parent,
	             const wxString& label,
	             actionFun fun,
				 void* data,
	             const wxSize& size = wxDefaultSize,
	             long style = 0)
		: wxButton(parent, wxID_ANY, label, wxDefaultPosition, size, style),
		m_Fun(fun), m_Data(data)
	{
	}

protected:
	virtual void OnClick(wxCommandEvent&)
	{
		m_Fun(m_Data);
	}

private:
	actionFun m_Fun;
	void* m_Data;
	DECLARE_EVENT_TABLE();
};
