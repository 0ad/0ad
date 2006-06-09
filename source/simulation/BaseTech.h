//Andrew aka pyrolink
//ajdecker1022@msn.com
//Holds template information for technologies and research

#ifndef BASETECH_INCLUDED
#define BASETECH_INCLUDED

#include <vector>
#include "scripting/ScriptableObject.h"

class CStr;
class XMBElement;
class CXeromyces;

class CBaseTech : public CJSObject<CBaseTech>
{
	friend class CBaseTechCollection;
	
	typedef struct Modifier
	{
		CStr attribute;
		float value;
		Modifier() { value = 0; }
	};
	static STL_HASH_SET<CStr, CStr_hash_compare> m_scriptsLoaded;

public:
	CBaseTech();
	~CBaseTech() {} 
	
	//JS functions
	static void ScriptingInit();
	jsval ApplyEffects( JSContext* cx, uintN argc, jsval* argv );
	jsval IsValid( JSContext* cx, uintN argc, jsval* argv );
	jsval IsResearched( JSContext* cx, uintN argc, jsval* argv );
	jsval IsJSFirst( JSContext* cx, uintN argc, jsval* argv );
	jsval IsExcluded( JSContext* cx, uintN argc, jsval* argv );
	inline jsval GetPlayerID( JSContext* cx, uintN argc, jsval* argv );
	
	
	bool isTechValid();
	inline bool isResearched() { return m_researched; }

	void setPlayer( CPlayer* player ) { m_player=player; }
	void setExclusion( bool exclude ) { m_excluded=exclude; }

	bool loadXML( CStr filename );
	bool loadELID( XMBElement ID, CXeromyces& XeroFile );
	bool loadELReq( XMBElement Req, CXeromyces& XeroFile );
	bool loadELEffect( XMBElement Effect, CXeromyces& XeroFile, CStr& filename );

private:
	CStr m_Generic;
	CStr m_Specific;

	CStr m_Icon;
	int m_IconCell;
	CStr m_Classes;
	CStr m_History;

	float m_ReqTime;
	float m_ReqWood;
	float m_ReqFood;
	float m_ReqStone;
	float m_ReqOre;

	std::vector<CStr> m_ReqEntities;
	std::vector<CStr> m_ReqTechs;
	std::vector<CStr> m_Pairs;
	std::vector<CStr> m_Targets;
	std::vector<Modifier> m_Modifiers;
	std::vector<Modifier> m_Sets;
	
	CPlayer* m_player;	//Which player this tech belongs to
	CScriptObject* m_effectFunction;
	
	bool m_JSFirst;	//Should JS effect function run before C++
	bool m_excluded;
	bool m_researched;

	bool hasReqEntities();
	bool hasReqTechs();
};

#endif