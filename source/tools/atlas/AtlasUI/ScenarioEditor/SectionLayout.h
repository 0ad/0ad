#ifndef SECTIONLAYOUT_H__
#define SECTIONLAYOUT_H__

class SnapSplitterWindow;

class SectionLayout
{
public:
	SectionLayout();
	~SectionLayout();

	void SetWindow(wxWindow* window);
	wxWindow* GetCanvasParent();
	void SetCanvas(wxWindow*);
	void Build();

private:
	wxWindow* m_Canvas;
	SnapSplitterWindow* m_HorizSplitter;
	SnapSplitterWindow* m_VertSplitter;
};

#endif // SECTIONLAYOUT_H__
