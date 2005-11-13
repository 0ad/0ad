#include "AttributeEditor.h"

CAttributeEditor *CAttributeEditor::m_pInstance = 0;
CAttributeEditor::CAttributeEditor(void)
{
	// Particle Engine Code:
	m_pParticleEngine = CParticleEngine::GetInstance();
	m_pParticleEngine->initParticleSystem();

	pEmitter = new CDefaultEmitter(300, -1);
	m_pParticleEngine->addEmitter(pEmitter);
}

CAttributeEditor::~CAttributeEditor(void)
{
	delete pEmitter;
}

CAttributeEditor *CAttributeEditor::GetInstance(void)
{
	// Check to see if one hasn't been made yet.
	if (m_pInstance == 0)
		m_pInstance = new CAttributeEditor;

	// Return the address of the instance.
	return m_pInstance;
}

void CAttributeEditor::DeleteInstance()
{ 
	if (m_pInstance)
		delete m_pInstance;
	m_pInstance = 0;
}

GLUI* CAttributeEditor::InitAttributeEditor()
{
	float yaw, yawVar;
	float pitch, pitchVar;
	float speed, speedVar;
	float forceX = 0, forceY = 0, forceZ = 0;
	float size;
	int emitsperframe, emitsVar;
	int startColorR, startColorG, startColorB;
	int startColorVarR, startColorVarG, startColorVarB;
	int endColorR, endColorG, endColorB;
	int endColorVarR, endColorVarG, endColorVarB;
	int life, lifeVar;
	int totalparticles;

	glui = GLUI_Master.create_glui( "Attribute Editor" );

	// Properties Rollout Box
	obj_panel = glui->add_rollout( "Emitter Properties", true );

	dir_panel = glui->add_panel_to_panel(obj_panel, "Emitter Direction");

	//pos_panel->set_alignment(GLUI_ALIGN_LEFT);
	//dir_panel->set_alignment(GLUI_ALIGN_LEFT);

	// Yaw
	yaw_edit = glui->add_edittext_to_panel(dir_panel, "Yaw", GLUI_EDITTEXT_INT, &yaw, 1);
	yaw_edit->set_int_limits(0, 360);
	yaw_edit->set_alignment(GLUI_ALIGN_LEFT);
	yaw_edit->set_int_val(pEmitter->getYaw());

	// Pitch
	pitch_edit = glui->add_edittext_to_panel(dir_panel, "Pitch", GLUI_EDITTEXT_INT, &pitch, 1);
	pitch_edit->set_int_limits(0, 360);
	pitch_edit->set_alignment(GLUI_ALIGN_LEFT);
	pitch_edit->set_int_val(pEmitter->getPitch());

	// Speed
	speed_edit = glui->add_edittext_to_panel(dir_panel, "Speed", GLUI_EDITTEXT_FLOAT, &speed, 1);
	speed_edit->set_float_limits(0, 1);
	speed_edit->set_alignment(GLUI_ALIGN_LEFT);
	speed_edit->set_float_val(pEmitter->getSpeed());

	glui->add_column_to_panel(dir_panel);

	// Yaw Variation
	yawVar_edit = glui->add_edittext_to_panel(dir_panel, "Variation", GLUI_EDITTEXT_INT, &yawVar, 1);
	yawVar_edit->set_int_limits(0, 360);
	yawVar_edit->set_alignment(GLUI_ALIGN_LEFT);
	yawVar_edit->set_int_val(pEmitter->getYawVar());

	// Pitch Variation
	pitchVar_edit = glui->add_edittext_to_panel(dir_panel, "Variation", GLUI_EDITTEXT_INT, &pitchVar, 1);
	pitchVar_edit->set_int_limits(0, 360);
	pitchVar_edit->set_alignment(GLUI_ALIGN_LEFT);
	pitchVar_edit->set_int_val(pEmitter->getPitchVar());

	// Speed Variation
	speedVar_edit = glui->add_edittext_to_panel(dir_panel, "Variation", GLUI_EDITTEXT_FLOAT, &speedVar, 1);
	speedVar_edit->set_float_limits(0, 1);
	speedVar_edit->set_alignment(GLUI_ALIGN_LEFT);
	speedVar_edit->set_float_val(pEmitter->getSpeedVar());

	// END OF DIRECTION PANEL


	// Emits Per Frame
	emit_panel = glui->add_panel_to_panel(obj_panel, "Emits Per Frame", false);
	//emit_panel->set_alignment(GLUI_ALIGN_LEFT);

	emitsperframe_edit = glui->add_edittext_to_panel(emit_panel, "Emits Per Frame", GLUI_EDITTEXT_INT, &emitsperframe, 1);
	emitsperframe_edit->set_int_limits(0, 1000);
	emitsperframe_edit->set_alignment(GLUI_ALIGN_LEFT);
	emitsperframe_edit->set_int_val(pEmitter->getEmitsPerFrame());

	particlelife_edit = glui->add_edittext_to_panel(emit_panel, "Particle Life", GLUI_EDITTEXT_INT,  &life, 1);
	particlelife_edit->set_int_limits(0, 10000);
	particlelife_edit->set_alignment(GLUI_ALIGN_LEFT);
	particlelife_edit->set_int_val(pEmitter->getLife());

	glui->add_column_to_panel(emit_panel);

	emitsVar_edit = glui->add_edittext_to_panel(emit_panel, "Variation", GLUI_EDITTEXT_INT, &emitsVar, 1);
	emitsVar_edit->set_int_limits(0, 100);
	emitsVar_edit->set_alignment(GLUI_ALIGN_LEFT);
	emitsVar_edit->set_int_val(pEmitter->getEmitVar());

	particlelifeVar_edit = glui->add_edittext_to_panel(emit_panel, "Variation", GLUI_EDITTEXT_INT, &lifeVar, 1);
	particlelifeVar_edit->set_int_limits(0, 100);
	particlelifeVar_edit->set_alignment(GLUI_ALIGN_LEFT);

	// END OF EMITS PANEL

	// Total Particles:
	totalparticle_edit = glui->add_edittext_to_panel(obj_panel, "Total Particles", GLUI_EDITTEXT_INT, &totalparticles, 1);
	totalparticle_edit->set_int_limits( 100, 10000 );
	totalparticle_edit->set_alignment(GLUI_ALIGN_CENTER);

	// Emitter Life:
	/*GLUI_EditText *emitterlife_edit = glui->add_edittext_to_panel(obj_panel, "Emitter Life", GLUI_EDITTEXT_INT, &emitterLife, 1);
	emitterlife_edit->set_int_limits(-1, 10000);
	emitterlife_edit->set_alignment(GLUI_ALIGN_CENTER);
	emitterlife_edit->set_int_val(100);*/

	size_edit = glui->add_edittext_to_panel(obj_panel, "Size", GLUI_EDITTEXT_FLOAT, &size, 1);
	size_edit->set_float_val(pEmitter->getSize());
	size_edit->set_float_limits(0.0f, 2.0f);
	size_edit->set_alignment(GLUI_ALIGN_CENTER);


	color_panel = glui->add_rollout_to_panel( obj_panel, "Color", false );

	startColor_panel = glui->add_panel_to_panel(color_panel, "Start Color");
	startcolorR_edit = glui->add_edittext_to_panel(startColor_panel, "R", GLUI_EDITTEXT_INT, &startColorR, 1);
	startcolorG_edit = glui->add_edittext_to_panel(startColor_panel, "G", GLUI_EDITTEXT_INT, &startColorG, 1);
	startcolorB_edit = glui->add_edittext_to_panel(startColor_panel, "B", GLUI_EDITTEXT_INT, &startColorB, 1);
	startcolorR_edit->set_int_limits(0, 255);
	startcolorG_edit->set_int_limits(0, 255);
	startcolorB_edit->set_int_limits(0, 255);
	startcolorR_edit->set_int_val(pEmitter->getStartColor().r);
	startcolorG_edit->set_int_val(pEmitter->getStartColor().g);
	startcolorB_edit->set_int_val(pEmitter->getStartColor().b);


	endColor_panel = glui->add_panel_to_panel(color_panel, "End Color");
	endcolorR_edit = glui->add_edittext_to_panel(endColor_panel, "R", GLUI_EDITTEXT_INT, &endColorR, 1);
	endcolorG_edit = glui->add_edittext_to_panel(endColor_panel, "G", GLUI_EDITTEXT_INT, &endColorG, 1);
	endcolorB_edit = glui->add_edittext_to_panel(endColor_panel, "B", GLUI_EDITTEXT_INT, &endColorB, 1);
	endcolorR_edit->set_int_limits(0, 255);
	endcolorG_edit->set_int_limits(0, 255);
	endcolorB_edit->set_int_limits(0, 255);
	endcolorR_edit->set_int_val(pEmitter->getEndColor().r);
	endcolorG_edit->set_int_val(pEmitter->getEndColor().g);
	endcolorB_edit->set_int_val(pEmitter->getEndColor().b);

	glui->add_column_to_panel(color_panel, false);

	startColorVar_panel = glui->add_panel_to_panel(color_panel, "Start Color Variation");
	startcolorvarR_edit = glui->add_edittext_to_panel(startColorVar_panel, "R", GLUI_EDITTEXT_INT, &startColorVarR, 1);
	startcolorvarG_edit = glui->add_edittext_to_panel(startColorVar_panel, "G", GLUI_EDITTEXT_INT, &startColorVarG, 1);
	startcolorvarB_edit = glui->add_edittext_to_panel(startColorVar_panel, "B", GLUI_EDITTEXT_INT, &startColorVarB, 1);
	startcolorvarR_edit->set_int_limits(0, 50);
	startcolorvarG_edit->set_int_limits(0, 50);
	startcolorvarB_edit->set_int_limits(0, 50);
	startcolorvarR_edit->set_int_val(pEmitter->getStartColorVar().r);
	startcolorvarG_edit->set_int_val(pEmitter->getStartColorVar().g);
	startcolorvarB_edit->set_int_val(pEmitter->getStartColorVar().b);

	endColorVar_panel = glui->add_panel_to_panel(color_panel, "End Color Variation");
	endcolorvarR_edit = glui->add_edittext_to_panel(endColorVar_panel, "R", GLUI_EDITTEXT_INT, &endColorVarR, 1);
	endcolorvarG_edit = glui->add_edittext_to_panel(endColorVar_panel, "G", GLUI_EDITTEXT_INT, &endColorVarG, 1);
	endcolorvarB_edit = glui->add_edittext_to_panel(endColorVar_panel, "B", GLUI_EDITTEXT_INT, &endColorVarB, 1);
	endcolorvarR_edit->set_int_limits(0, 50);
	endcolorvarG_edit->set_int_limits(0, 50);
	endcolorvarB_edit->set_int_limits(0, 50);
	endcolorvarR_edit->set_int_val(pEmitter->getEndColorVar().r);
	endcolorvarG_edit->set_int_val(pEmitter->getEndColorVar().g);
	endcolorvarB_edit->set_int_val(pEmitter->getEndColorVar().b);
	
	// END OF COLOR ROLLOUT*/

	force_panel = glui->add_panel_to_panel(obj_panel, "Force");
	forceX_edit = glui->add_edittext_to_panel(force_panel, "X", GLUI_EDITTEXT_FLOAT, &forceX, 1);
	forceY_edit = glui->add_edittext_to_panel(force_panel, "Y", GLUI_EDITTEXT_FLOAT, &forceY, 1);
	forceZ_edit = glui->add_edittext_to_panel(force_panel, "Z", GLUI_EDITTEXT_FLOAT, &forceZ, 1);
	forceX_edit->set_float_val(pEmitter->getForceX());
	forceY_edit->set_float_val(pEmitter->getForceY());
	forceZ_edit->set_float_val(pEmitter->getForceZ());

	return glui;
}

void CAttributeEditor::UpdateEmitterChanges()
{
	pEmitter->setYaw(yaw_edit->get_int_val());
	pEmitter->setPitch(pitch_edit->get_int_val());
	pEmitter->setSpeed(speed_edit->get_float_val());
	pEmitter->setYawVar(yawVar_edit->get_int_val());
	pEmitter->setPitchVar(pitchVar_edit->get_int_val());
	pEmitter->setSpeedVar(speedVar_edit->get_int_val());
	pEmitter->setEmitsPerFrame(emitsperframe_edit->get_int_val());
	pEmitter->setLife(particlelife_edit->get_int_val());
	pEmitter->setEmitVar(emitsVar_edit->get_int_val());
	pEmitter->setLifeVar(particlelifeVar_edit->get_int_val());
	pEmitter->setSize(size_edit->get_float_val());
	pEmitter->setStartColorR(startcolorR_edit->get_int_val());
	pEmitter->setStartColorG(startcolorG_edit->get_int_val());
	pEmitter->setStartColorB(startcolorB_edit->get_int_val());
	pEmitter->setStartColorVarR(startcolorvarR_edit->get_int_val());
	pEmitter->setStartColorVarG(startcolorvarG_edit->get_int_val());
	pEmitter->setStartColorVarB(startcolorvarB_edit->get_int_val());
	pEmitter->setEndColorR(endcolorR_edit->get_int_val());
	pEmitter->setEndColorG(endcolorG_edit->get_int_val());
	pEmitter->setEndColorB(endcolorB_edit->get_int_val());
	pEmitter->setEndColorVarR(endcolorvarR_edit->get_int_val());
	pEmitter->setEndColorVarG(endcolorvarG_edit->get_int_val());
	pEmitter->setEndColorVarB(endcolorvarB_edit->get_int_val());
	pEmitter->setForceX(forceX_edit->get_float_val());
	pEmitter->setForceY(forceY_edit->get_float_val());
	pEmitter->setForceZ(forceZ_edit->get_float_val());
}

void CAttributeEditor::UpdateAttributeEditor()
{
	// Update the emitter from the changes made in the text box
	UpdateEmitterChanges();

	// Update and render the emitter
	m_pParticleEngine->updateEmitters();
	m_pParticleEngine->renderParticles();
}