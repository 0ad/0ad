#ifndef __H_MATERIALREADER_H__
#define __H_MATERIALREADER_H__

#include "Singleton.h"
#include "Material.h"

#define g_MaterialReader CMaterialReader::GetSingleton()

class CMaterialReader : public Singleton<CMaterialReader>
{
public:
	CMaterialReader();
	~CMaterialReader();

	CMaterial *LoadMaterial(const char *file);
private:

};

#endif