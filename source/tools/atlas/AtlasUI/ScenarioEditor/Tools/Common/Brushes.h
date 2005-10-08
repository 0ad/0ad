#ifndef BRUSHES_H__
#define BRUSHES_H__

class BrushShapeCtrl;
class BrushSizeCtrl;

class Brush
{
	friend class BrushShapeCtrl;
	friend class BrushSizeCtrl;
public:
	Brush();
	~Brush();

	int GetWidth();
	int GetHeight();
	float* GetNewedData(); // freshly allocated via new[]

	void CreateUI(wxWindow* parent, wxSizer* sizer);

	// Set this brush to be active - sends SetBrush message now, and also
	// whenever the brush is altered (until a different one is activated).
	void MakeActive();

private:
	// If active, send SetBrush message to the game
	void Send();

	enum BrushShape { SQUARE = 0, CIRCLE };
	BrushShape m_BrushShape;
	int m_BrushSize;
	bool m_IsActive;
};

extern Brush g_Brush_Elevation;

extern Brush* g_Brush_CurrentlyActive;

#endif // BRUSHES_H__
