class EditableListCtrl;

class ListCtrlValidator : public wxValidator
{
public:
	ListCtrlValidator();
	ListCtrlValidator(EditableListCtrl* listCtrl, long row, int col);

	wxObject* Clone() const;

	bool TransferToWindow();
	bool TransferFromWindow();
	bool Validate(wxWindow *parent);

private:
	EditableListCtrl* m_listCtrl;
	long m_Row;
	int m_Col;
};
