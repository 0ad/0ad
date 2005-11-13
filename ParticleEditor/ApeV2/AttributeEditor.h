#ifndef _ATTRIBUTEEDITOR_H_
#define _ATTRIBUTEEDITOR_H_

#include "glui.h"
#include "ParticleEngine.h"
#include "DefaultEmitter.h"

class CAttributeEditor
{
public:
	virtual ~CAttributeEditor(void);

	static CAttributeEditor* GetInstance();
	static void DeleteInstance();

	GLUI* InitAttributeEditor();
	void UpdateAttributeEditor();
	void UpdateEmitterChanges();

	GLUI* GetGLUI() { return glui; }

private:
	CAttributeEditor();
	static CAttributeEditor*  m_pInstance;    // The singleton instance

	CParticleEngine *m_pParticleEngine;
	CDefaultEmitter *pEmitter;

	GLUI *glui;

	GLUI_Checkbox   *checkbox;
	GLUI_Panel      *obj_panel, *dir_panel, *pos_panel, *color_panel;
	GLUI_EditText	*yaw_edit;
	GLUI_EditText	*pitch_edit;
	GLUI_EditText	*speed_edit;
	GLUI_EditText	*yawVar_edit;
	GLUI_EditText	*pitchVar_edit;
	GLUI_EditText	*speedVar_edit;
	GLUI_Panel		*emit_panel;
	GLUI_EditText	*emitsperframe_edit;
	GLUI_EditText	*particlelife_edit;
	GLUI_EditText	*emitsVar_edit;
	GLUI_EditText	*particlelifeVar_edit;
	GLUI_EditText	*totalparticle_edit;
	GLUI_EditText	*size_edit;
	GLUI_Panel		*startColor_panel;
	GLUI_EditText	*startcolorR_edit;
	GLUI_EditText	*startcolorG_edit;
	GLUI_EditText	*startcolorB_edit;
	GLUI_Panel		*endColor_panel;
	GLUI_EditText	*endcolorR_edit;
	GLUI_EditText	*endcolorG_edit;
	GLUI_EditText	*endcolorB_edit;
	GLUI_Panel		*startColorVar_panel;
	GLUI_EditText	*startcolorvarR_edit;
	GLUI_EditText	*startcolorvarG_edit;
	GLUI_EditText	*startcolorvarB_edit;
	GLUI_Panel		*endColorVar_panel;
	GLUI_EditText	*endcolorvarR_edit;
	GLUI_EditText	*endcolorvarG_edit;
	GLUI_EditText	*endcolorvarB_edit;
	GLUI_Panel		*force_panel;
	GLUI_EditText	*forceX_edit;
	GLUI_EditText	*forceY_edit;
	GLUI_EditText	*forceZ_edit;
};

#endif