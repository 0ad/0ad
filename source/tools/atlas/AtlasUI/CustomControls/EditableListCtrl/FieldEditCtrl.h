class EditableListCtrl;

class FieldEditCtrl
{
	friend class EditableListCtrl;

public:
	virtual ~FieldEditCtrl() {};

protected:
	virtual void StartEdit(wxWindow* parent, wxRect rect, long row, int col)=0;
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_Text : public FieldEditCtrl
{
protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_Dialog : public FieldEditCtrl
{
public:
	FieldEditCtrl_Dialog(wxString dialogType);

protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);

private:
	wxString m_DialogType;
};
