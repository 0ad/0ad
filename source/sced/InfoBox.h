#ifndef _INFOBOX_H
#define _INFOBOX_H

class NPFont;

#include "Renderer.h"

class CInfoBox
{
public:
	CInfoBox();

	// setup the infobox
	void Initialise();
	// render out the info box
	void Render();
	// frame has been rendered out, update stats
	void OnFrameComplete();
	
	// set visibility
	void SetVisible(bool flag) { m_Visible=flag; }
	// get visibility
	bool GetVisible() const { return m_Visible; }

private:
	// text output: render any relevant current information
	void RenderInfo();

	// currently visible?
	bool m_Visible;
	// font to use rendering out text to the info box
	NPFont* m_Font;
	// current frame rate
	double m_FPS;
	// last known frame time
	double m_LastFPSTime;
	// accumulated stats
	CRenderer::Stats m_Stats;
	// time of the last 1 second "tick"
	double m_LastTickTime;
	// stats from last 1 second "tick"
	CRenderer::Stats m_LastStats;
};

#endif
