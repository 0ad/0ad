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

#include "precompiled.h"

#include "ModelAbstract.h"

#include "ps/CLogger.h"

const CBoundingBoxOriented& CModelAbstract::GetSelectionBox()
{
	if (!m_SelectionBoxValid)
	{
		CalcSelectionBox();
		m_SelectionBoxValid = true;
	}
	return m_SelectionBox;
}

void CModelAbstract::CalcSelectionBox()
{
	if (m_CustomSelectionShape)
	{
		// custom shape
		switch(m_CustomSelectionShape->m_Type)
		{
		case CustomSelectionShape::BOX:
			{
				// create object-space bounds according to the information in the descriptor, and transform them to world-space.
				// the box is centered on the X and Z axes, but extends from 0 to its height on the Y axis.
				const float width = m_CustomSelectionShape->m_Size0;
				const float depth = m_CustomSelectionShape->m_Size1;
				const float height = m_CustomSelectionShape->m_Height;

				CBoundingBoxAligned bounds;
				bounds += CVector3D(-width/2.f, 0,     -depth/2.f);
				bounds += CVector3D( width/2.f, height, depth/2.f);

				bounds.Transform(GetTransform(), m_SelectionBox);
			}
			break;
		case CustomSelectionShape::CYLINDER:
			{
				// TODO: unimplemented
				m_SelectionBox.SetEmpty();
				LOGWARNING("[ModelAbstract] TODO: Cylinder selection boxes are not yet implemented. Use BOX or BOUNDS selection shapes instead.");
			}
			break;
		default:
			{
				m_SelectionBox.SetEmpty();
				//LOGWARNING("[ModelAbstract] Unrecognized selection shape type: %ld", m_CustomSelectionShape->m_Type);
				debug_warn("[ModelAbstract] Unrecognized selection shape type");
			}
			break;
		}
	}
	else
	{
		// standard method

		// Get the object-space bounds that should be used to construct this model (and its children)'s selection box
		CBoundingBoxAligned objBounds = GetObjectSelectionBoundsRec();
		if (objBounds.IsEmpty())
		{
			m_SelectionBox.SetEmpty(); // model does not wish to participate in selection
			return;
		}

		// Prevent the bounding box from extending through the terrain; clip the lower plane at Y=0 in object space.
		if (objBounds[1].Y > 0.f) // should always be the case, unless the models are defined really weirdly
			objBounds[0].Y = std::max(0.f, objBounds[0].Y);

		// transform object-space axis-aligned bounds to world-space arbitrary-aligned box
		objBounds.Transform(GetTransform(), m_SelectionBox);
	}

}
