#ifndef SECTIONLAYOUT_H__
#define SECTIONLAYOUT_H__

#include <map>
#include <string>

class SnapSplitterWindow;
class SidebarBook;
class wxWindow;

class SectionLayout
{
public:
	SectionLayout();
	~SectionLayout();

	void SetWindow(wxWindow* window);
	wxWindow* GetCanvasParent();
	void SetCanvas(wxWindow*);
	void Build();

	void SelectPage(const wxString& classname);

private:
	SidebarBook* m_SidebarBook;
	wxWindow* m_Canvas;
	SnapSplitterWindow* m_HorizSplitter;
	SnapSplitterWindow* m_VertSplitter;
	std::map<std::wstring, int> m_PageMappings;
};

#endif // SECTIONLAYOUT_H__
