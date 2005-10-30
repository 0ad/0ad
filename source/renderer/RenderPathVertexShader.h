#ifndef __RENDERPATHVERTEXSHADER_H__
#define __RENDERPATHVERTEXSHADER_H__

class RenderPathVertexShader
{
public:
	RenderPathVertexShader();
	~RenderPathVertexShader();
	
	// Initialize this render path.
	bool Init();

	// Call once per frame to update program stuff
	void BeginFrame();

public:
	Handle m_ModelLight;
	GLint m_ModelLight_SHCoefficients;
	
	Handle m_InstancingLight;
	GLint m_InstancingLight_SHCoefficients;
	GLint m_InstancingLight_Instancing1; // matrix rows
	GLint m_InstancingLight_Instancing2;
	GLint m_InstancingLight_Instancing3;

	Handle m_Instancing;
	GLint m_Instancing_Instancing1; // matrix rows
	GLint m_Instancing_Instancing2;
	GLint m_Instancing_Instancing3;
};

#endif // __RENDERPATHVERTEXSHADER_H__
