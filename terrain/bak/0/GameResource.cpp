#include "GameResource.H"

FRESULT CGameResource::LoadResource (char *filename, RESOURCETYPE type)
{
	m_Type = type;

	strcpy (m_Path, filename);

	_splitpath (m_Path, NULL, NULL, m_Name, NULL);

	return R_OK;
}