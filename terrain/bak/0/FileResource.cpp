//***********************************************************
//
// Name:		FileResource.Cpp
// Author:		Poya Manouchehri
//
// Description: A File resource is directly loaded from a data
//				file, like a BMP or WAV file.
//
//***********************************************************

#include "FileResource.H"

void CFileResource::SetupParams (char *path, RESOURCETYPE type)
{
	m_Type = type;
	char Extention[10];

	strcpy (m_Path, path);

	_splitpath (m_Path, NULL, NULL, m_Name, Extention);
	strcat (m_Name, Extention);
}