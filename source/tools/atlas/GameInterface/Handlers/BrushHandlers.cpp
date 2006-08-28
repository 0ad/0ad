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
