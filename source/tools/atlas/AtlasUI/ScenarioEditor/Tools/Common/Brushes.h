#ifndef BRUSHES_H__
#define BRUSHES_H__

class BrushShapeCtrl;
class BrushSizeCtrl;
class BrushStrengthCtrl;

#include <vector>

class Brush
{
	friend class BrushShapeCtrl;
	friend class BrushSizeCtrl;
	friend class BrushStrengthCtrl;
public:
	Brush();
	~Brush();

	int GetWidth() const;
	int GetHeight() const;
	std::vector<float> GetData() const;

	float GetStrength() const;

	void CreateUI(wxWindow* parent, wxSizer* sizer);

	// Set this brush to be active - sends SetBrush message now, and also
	// whenever the brush is altered (until a different one is activated).
	void MakeActive();

private:
	// If active, send SetBrush message to the game
	void Send();

	enum BrushShape { CIRCLE = 0, SQUARE};
	BrushShape m_Shape;
	int m_Size;
	float m_Strength;
	bool m_IsActive;
};

extern Brush g_Brush_Elevation;

extern Brush* g_Brush_CurrentlyActive;

#endif // BRUSHES_H__
