class FileConverter : public wxFrame
{
public:
	FileConverter(wxWindow* parent);
	virtual bool Show(bool);

private:
	bool m_Transient; // if true, the window won't be shown (since it's assumed to have been destroyed immediately)
};
