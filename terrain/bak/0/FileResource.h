//***********************************************************
//
// Name:		FileResource.H
// Author:		Poya Manouchehri
//
// Description: A File resource is directly loaded from a data
//				file, like a BMP or WAV file.
//
//***********************************************************

#ifndef FILERESOURCE_H
#define FILERESOURCE_H

#include "Types.H"
#include "Resource.H"


#define MAX_PATH_LENGTH		(100)


class CFileResource : public CResource
{
	public:
		char *GetPath() { return m_Path; }

	protected:
		void SetupParams (char *path, RESOURCETYPE type);
		char m_Path[MAX_PATH_LENGTH];
};

#endif