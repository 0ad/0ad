#include "stdafx.h"

#include "AtlasWindowCommand.h"

#include "IAtlasSerialiser.h"

IMPLEMENT_ABSTRACT_CLASS(AtlasWindowCommand, wxCommand);

IMPLEMENT_CLASS(AtlasCommand_Begin, AtlasWindowCommand);
IMPLEMENT_CLASS(AtlasCommand_End, AtlasWindowCommand);

AtlasCommand_Begin::AtlasCommand_Begin(const wxString& description, IAtlasSerialiser* object)
	: AtlasWindowCommand(true, description),
	m_Object(object),
	m_PreData(object->FreezeData())
{
}

bool AtlasCommand_Begin::Do()
{
	if (m_PostData.defined())
		m_Object->ThawData(m_PostData);
	return true;
}

bool AtlasCommand_Begin::Undo()
{
	m_Object->ThawData(m_PreData);
	return true;
}



bool AtlasCommand_End::Merge(AtlasWindowCommand* command)
{
	AtlasCommand_Begin* previousCommand = wxDynamicCast(command, AtlasCommand_Begin);

	if (! previousCommand)
	{
		wxLogError(_("Internal error - invalid _end merge"));
		return false;
	}

	previousCommand->m_PostData = previousCommand->m_Object->FreezeData();
	return true;
}
