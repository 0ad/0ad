#ifndef _PAINTTEXTURECOMMAND_H
#define _PAINTTEXTURECOMMAND_H

#include <set>
#include "res/res.h"
#include "Command.h"
#include "Array2D.h"

struct TextureSet
{
	Handle m_Texture;
	int m_Priority;
};

class CTextureEntry;

class CPaintTextureCommand : public CCommand
{
public:
	// constructor, destructor
	CPaintTextureCommand(CTextureEntry* tex,int brushSize,int selectionCentre[2]);
	~CPaintTextureCommand();

	// return the texture name of this command
	const char* GetName() const { return "Apply Texture"; }
	
	// execute this command
	void Execute();
	
	// can undo command?
	bool IsUndoable() const { return true; }
	// undo 
	void Undo();
	// redo 
	void Redo();

private:
	bool IsValidDataIndex(const CArray2D<TextureSet>& array,int x,int y) {
		if (x<0 || y<0) return 0;
		int ix=x-m_SelectionOrigin[0];
		int iy=y-m_SelectionOrigin[1];
		return ix>=0 && ix<array.usize() && iy>=0 && iy<array.vsize();
	}

	TextureSet* DataIn(int x,int y) { 
		return IsValidDataIndex(m_DataIn,x,y) ? &m_DataIn(x-m_SelectionOrigin[0],y-m_SelectionOrigin[1]) : 0;
	}
	TextureSet* DataOut(int x,int y) { 
		return IsValidDataIndex(m_DataOut,x,y) ? &m_DataOut(x-m_SelectionOrigin[0],y-m_SelectionOrigin[1]) : 0;
	}

	void ApplyDataToSelection(const CArray2D<TextureSet>& data);

	// texture being painted
	CTextureEntry* m_Texture;
	// size of brush
	int m_BrushSize;
	// centre of brush
	int m_SelectionCentre[2];
	// origin of data set
	int m_SelectionOrigin[2];
	// input data (textures applied to the selection)
	CArray2D<TextureSet> m_DataIn;
	// output data (new textures applied to the selection after painting)
	CArray2D<TextureSet> m_DataOut;
};

#endif
