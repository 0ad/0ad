// EntityTemplateCollection.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Keeps tabs on the various types of entity that roam the world.
//
// General note: Template, Base Entity, and Entity Class are used more-or-less interchangably.
// 
// Usage: g_EntityTemplateCollection.LoadTemplates(): loads all templates
//        g_EntityTemplateCollection.GetTemplate(name): get a template by name
//
// EntityTemplateCollection will look at all subdirectiroes of entities/, but each template's
// name will only be its filename; thus, no two templates should have the same filename,
// but subdirectories can be created in entities/ to organize the files nicely.

#ifndef ENTITY_TEMPLATE_COLLECTION_INCLUDED
#define ENTITY_TEMPLATE_COLLECTION_INCLUDED

#include <vector>
#include <map>
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "ps/Game.h"

#define g_EntityTemplateCollection CEntityTemplateCollection::GetSingleton()

class CPlayer;
class CEntityTemplate;

class CEntityTemplateCollection : public Singleton<CEntityTemplateCollection>
{
	// TODO: PS_MAX_PLAYERS doesn't seem to be an upper limit -
	//  "This may be overridden by system.cfg ("max_players")"
	// - so we shouldn't use it here
	static const uint NULL_PLAYER = (PS_MAX_PLAYERS+1);

	typedef STL_HASH_MAP<CStrW, CEntityTemplate*, CStrW_hash_compare> TemplateMap;
	typedef STL_HASH_MAP<CStrW, CStr, CStrW_hash_compare> TemplateFilenameMap;
	
	TemplateMap m_templates[PS_MAX_PLAYERS + 2];
	TemplateFilenameMap m_templateFilenames;
public:
	~CEntityTemplateCollection();
	CEntityTemplate* GetTemplate( const CStrW& entityType, CPlayer* player = 0 );

	// Load list of template filenames
	int LoadTemplates();
	void LoadFile( const char* path );

	// Create a list of the names of all base entities, excluding template_*,
	// for display in ScEd's entity-selection box.
	void GetEntityTemplateNames( std::vector<CStrW>& names );

	// Get all the templates owned by a specific player, which is useful for techs
	void GetPlayerTemplates( CPlayer* player, std::vector<CEntityTemplate*>& dest );
};

#endif
