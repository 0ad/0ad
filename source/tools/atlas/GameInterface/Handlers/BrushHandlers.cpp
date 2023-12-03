/* Copyright (C) 2009 Wildfire Games.
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

#include "MessageHandler.h"

#include "../Brushes.h"

namespace AtlasMessage {

MESSAGEHANDLER(Brush)
{
	g_CurrentBrush.SetData(msg->width, msg->height, *msg->data);
}

MESSAGEHANDLER(BrushPreview)
{
	g_CurrentBrush.SetRenderEnabled(msg->enable);
	g_CurrentBrush.m_Centre = msg->pos->GetWorldSpace();
}

}
