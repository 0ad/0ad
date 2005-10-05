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
};

#endif // __RENDERPATHVERTEXSHADER_H__
