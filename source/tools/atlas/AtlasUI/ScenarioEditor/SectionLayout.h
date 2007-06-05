#ifndef INCLUDED_SECTIONLAYOUT
#define INCLUDED_SECTIONLAYOUT

#include <map>
#include <string>

class ScenarioEditor;
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
	void Build(ScenarioEditor&);

	void SelectPage(const wxString& classname);

private:
	SidebarBook* m_SidebarBook;
	wxWindow* m_Canvas;
	SnapSplitterWindow* m_HorizSplitter;
	SnapSplitterWindow* m_VertSplitter;
	std::map<std::wstring, int> m_PageMappings;
};

#endif // INCLUDED_SECTIONLAYOUT
