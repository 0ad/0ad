#include "precompiled.h"

#include "CommandManager.h"
#include "TextureEntry.h"
#include "PaintTextureTool.h"
#include "PaintTextureCommand.h"

// default tool instance
CPaintTextureTool CPaintTextureTool::m_PaintTextureTool;

CPaintTextureTool::CPaintTextureTool() 
{
	m_SelectedTexture=0;
}

void CPaintTextureTool::PaintSelection()
{
	// apply current texture to current selection
	CPaintTextureCommand* paintCmd=new CPaintTextureCommand(m_SelectedTexture,m_BrushSize,m_SelectionCentre);
	g_CmdMan.Execute(paintCmd);
}


