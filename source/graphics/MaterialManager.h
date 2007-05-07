#ifndef INCLUDED_MATERIALMANAGER
#define INCLUDED_MATERIALMANAGER

#include <map>
#include "ps/Singleton.h"
#include "Material.h"

#define g_MaterialManager CMaterialManager::GetSingleton()

class CMaterialManager : public Singleton<CMaterialManager>
{
public:
	CMaterialManager();
	~CMaterialManager();

	CMaterial &LoadMaterial(const char *file);
private:
    std::map<std::string, CMaterial *> m_Materials;
};

#endif
