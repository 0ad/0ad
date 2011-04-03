/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File        : Scene.cpp
 * Project     : graphics
 * Description : This file contains default implementations and utilities
 *             : to be used together with the Scene interface and related
 *             : classes.
 *
 * @note This file would fit just as well into the graphics/ subdirectory.
 */

#include "precompiled.h"

#include "graphics/Model.h"
#include "graphics/ParticleEmitter.h"
#include "renderer/Scene.h"

///////////////////////////////////////////////////////////
// Default implementation traverses the model recursively and uses
// SubmitNonRecursive for the actual work.
void SceneCollector::SubmitRecursive(CModelAbstract* model)
{
	if (model->ToCModel())
	{
		SubmitNonRecursive(model->ToCModel());

		const std::vector<CModel::Prop>& props = model->ToCModel()->GetProps();
		for (size_t i = 0; i < props.size(); i++)
		{
			if (!props[i].m_Hidden)
				SubmitRecursive(props[i].m_Model);
		}
	}
	else if (model->ToCModelDecal())
	{
		Submit(model->ToCModelDecal());
	}
	else if (model->ToCModelParticleEmitter())
	{
		Submit(model->ToCModelParticleEmitter()->m_Emitter.get());
	}
	else
		debug_warn(L"unknown model type");
}
