/**
 * File        : Scene.cpp
 * Project     : graphics
 * Description : This file contains default implementations and utilities
 *             : to be used together with the Scene interface and related
 *             : classes.
 *
 * @note This file would fit just as well into the graphics/ subdirectory.
 **/

#include "precompiled.h"

#include "graphics/Model.h"

#include "renderer/Scene.h"



///////////////////////////////////////////////////////////
// Default implementation traverses the model recursively and uses
// SubmitNonRecursive for the actual work.
void SceneCollector::SubmitRecursive(CModel* model)
{
	SubmitNonRecursive(model);

	const std::vector<CModel::Prop>& props = model->GetProps();
	for (size_t i=0;i<props.size();i++) {
		SubmitRecursive(props[i].m_Model);
	}
}


