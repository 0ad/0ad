#include "precompiled.h"

#include "TriggerManager.h"
#include "scripting/ScriptingHost.h"
#include "ps/XML/Xeromyces.h"
#include "ps/XML/XeroXMB.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "Triggers"


CTrigger::CTrigger()
{
	m_timeLeft = m_timeDelay = m_maxRunCount = m_runCount = m_active = 0;
}
CTrigger::~CTrigger()
{

}
CTrigger::CTrigger(const CStrW& name, bool active, float delay, int maxRuns, const CStrW& condFunc, 
	const CStrW& effectFunc)
{
	m_name = name;
	m_active = active;
	m_timeLeft = m_timeDelay = delay;
	m_runCount = 0;
	m_maxRunCount = maxRuns;
	
	CStrW validName = name;
	validName.Replace(L" ", L"_");

	m_conditionFuncString = condFunc;
	m_effectFuncString = effectFunc;
	m_conditionFunction.Compile( L"TriggerCondition_" + validName, condFunc );
	m_effectFunction.Compile( L"TriggerEffect_" + validName, effectFunc );
}
CTrigger::CTrigger(const CStrW& name, bool active, float delay, int maxRuns,
					CScriptObject& condFunc, CScriptObject& effectFunc)
{
	m_name = name;
	m_active = active;
	m_timeLeft = m_timeDelay = delay;
	m_runCount = 0;
	m_maxRunCount = maxRuns;
	m_conditionFunction = condFunc;
	m_effectFunction = effectFunc;
}

JSBool CTrigger::Construct( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS_CPP(6);

	CScriptObject condFunc( JS_ValueToFunction(cx, argv[4]) );
	CScriptObject effectFunc( JS_ValueToFunction(cx, argv[5]) );
	CTrigger* newTrigger = new CTrigger( ToPrimitive<CStr>( argv[0] ),
										 ToPrimitive<bool>( argv[1] ),
										 ToPrimitive<float>( argv[2] ),
										 ToPrimitive<int>( argv[3]),
										 condFunc, effectFunc );

	g_TriggerManager.AddTrigger(newTrigger);
	*rval = OBJECT_TO_JSVAL( newTrigger->GetScript() );
	return JS_TRUE;
}

CTrigger& CTrigger::operator= (const CTrigger& trigger)
{
	m_name = trigger.m_name;
	m_active = trigger.m_active;
	m_timeLeft = trigger.m_timeLeft;
	m_timeDelay = trigger.m_timeDelay;
	m_maxRunCount = trigger.m_maxRunCount;
	m_conditionFunction = trigger.m_conditionFunction;
	m_effectFunction = trigger.m_effectFunction;

	return *this;
}

void CTrigger::ScriptingInit()
{
	AddProperty<int>(L"runCount", &CTrigger::m_runCount, true);
	AddProperty(L"name", &CTrigger::m_name, true);
	AddProperty<int>(L"maxRunCount", &CTrigger::m_maxRunCount);
	AddProperty<float>(L"timeDelay", &CTrigger::m_timeDelay);

	AddMethod<void, &CTrigger::Activate>( "activate", 0 );
	AddMethod<void, &CTrigger::Deactivate>( "deactivate", 0 );

	CJSObject<CTrigger>::ScriptingInit("Trigger", CTrigger::Construct, 6);
}

bool CTrigger::ShouldFire()
{
	if ( !m_active )
		return false;
	return m_conditionFunction.Run( this->GetScript() );
}
bool CTrigger::Fire()
{
	m_effectFunction.Run( this->GetScript() );
	++m_runCount;
	m_timeLeft = m_timeDelay;
	return (m_runCount < m_maxRunCount);
}

void CTrigger::Activate(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	m_active = true;
}
void CTrigger::Deactivate(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	m_active = false;
}


void TriggerParameter::SetWindowData(const CStrW& _windowType, CStrW& windowPosition, CStrW& windowSize)
{
	windowPosition.Remove( CStrW(L" ") );
	windowSize.Remove( CStrW(L" ") );

	xPos = windowPosition.BeforeFirst( CStrW(L",") ).ToInt();
	yPos = windowPosition.AfterFirst( CStrW(L",") ).ToInt();
	xSize = windowSize.BeforeFirst( CStrW(L",") ).ToInt();
	ySize = windowSize.AfterFirst( CStrW(L",") ).ToInt();
	windowType = _windowType;
}

bool TriggerParameter::operator< ( const TriggerParameter& rhs ) const
{
	if ( row < rhs.row )
		return true;
	else if ( row == rhs.row)
		return ( column < rhs.column );

	return false;
}
//===========Trigger Manager===========

CTriggerManager::CTriggerManager() : m_UpdateRate(.20f), m_UpdateTime(.20f)
{
}

CTriggerManager::~CTriggerManager()
{
	DestroyEngineTriggers();
}

void CTriggerManager::DestroyEngineTriggers()
{
	for ( TriggerIter it = m_TriggerMap.begin(); it != m_TriggerMap.end(); ++it )
	{
		SAFE_DELETE(it->second);
	}
	m_TriggerMap.clear();
}

std::vector<std::wstring> CTriggerManager::GetTriggerChoices(const std::wstring& name)
{
	return m_TriggerChoices[name];
}
std::vector<std::wstring> CTriggerManager::GetTriggerTranslations(const std::wstring& name)
{
	return m_TriggerTranslations[name];
}

void CTriggerManager::Update(float delta_ms)
{
	float delta = delta_ms / 1000.f;
	m_UpdateTime -= delta;
	if ( m_UpdateTime < 0 )
		m_UpdateTime += m_UpdateRate;
	else
		return;
	
	std::list<TriggerIter> expired;
	for ( TriggerIter it = m_TriggerMap.begin(); it != m_TriggerMap.end(); ++it )
	{
		if ( it->second->ShouldFire() )
		{
			it->second->m_timeLeft -= m_UpdateRate;
			if ( it->second->m_timeLeft < 0 )
			{
				if ( !it->second->Fire() )
					expired.push_back(it);
			}
		}
	}

	//Remove all expired triggers
	for ( std::list<TriggerIter>::iterator it = expired.begin(); it != expired.end(); ++it )
	{
		delete (*it)->second;
		m_TriggerMap.erase(*it);
	}
}

void CTriggerManager::AddTrigger(CTrigger* trigger)
{
	m_TriggerMap[trigger->GetName()] = trigger;
}
void CTriggerManager::AddGroup(const MapTriggerGroup& group)
{
	m_GroupList.push_back(group);
}
void CTriggerManager::SetAllGroups(const std::list<MapTriggerGroup>& groups)
{
	m_GroupList = groups;
}

void CTriggerManager::AddTrigger(MapTriggerGroup& group, const MapTrigger& trigger)
{
	CStrW conditionBody;
	CStrW linkLogic[] = { CStrW(L""), CStrW(L" && "), CStrW(L" || ") };
	size_t i=0;
	bool allParameters = true;

	if(trigger.conditions.size() == 0) {
		conditionBody = CStrW(L"return ( true );");
	}
	else {
		conditionBody = CStrW(L"return ( ");

		for ( std::list<MapTriggerCondition>::const_iterator it = trigger.conditions.begin();
													it != trigger.conditions.end(); ++it, ++i )
		{
			//Opening parenthesis here?
			std::set<MapTriggerLogicBlock>::const_iterator blockIt;
			if ( ( blockIt = trigger.logicBlocks.find(MapTriggerLogicBlock(i)) ) != trigger.logicBlocks.end() )
			{
				if ( blockIt->negated )
					conditionBody += CStrW(L"!");
				conditionBody += CStrW(L" (");
			}
			
			if ( it->negated )
				conditionBody += CStrW(L"!");
			conditionBody += it->functionName;
			conditionBody += CStrW(L"(");
			
			for ( std::list<CStrW>::const_iterator it2 = it->parameters.begin(); it2 != 
														it->parameters.end(); ++it2 )
			{
				size_t params = (size_t)std::find(m_ConditionSpecs.begin(), m_ConditionSpecs.end(), it->displayName)->funcParameters;
				size_t distance = std::distance(it->parameters.begin(), it2);
				
				//Parameters end here, additional "parameters" are used directly as script
				if ( distance == params )
				{
					conditionBody += CStrW(L") ");
					allParameters = false;
				}

				//Take display parameter and translate into JS usable code...evilness
				CTriggerSpec spec = *std::find( m_ConditionSpecs.begin(), m_ConditionSpecs.end(), it->displayName );
				const std::set<TriggerParameter>& specParameters = spec.GetParameters();
				
				//Don't use specialized find, since we're searching for a different member
				std::set<TriggerParameter>::const_iterator specParam = std::find( 
								specParameters.begin(), specParameters.end(), (int)distance);
				std::wstring combined = std::wstring( it->functionName + specParam->name );
				size_t translatedIndex = std::distance( m_TriggerChoices[combined].begin(), 
					std::find(m_TriggerChoices[combined].begin(), m_TriggerChoices[combined].end(), std::wstring(*it2)) );

				if ( m_TriggerTranslations[combined].empty() )
					conditionBody += *it2;
				else
					conditionBody += m_TriggerTranslations[combined][translatedIndex];	

				if ( distance + 1 < params )
					conditionBody += CStrW(L", ");
			}
			
			if ( allParameters )	//Otherwise, closed inside loop
				conditionBody += CStrW(L")");
			if ( trigger.logicBlockEnds.find(i) != trigger.logicBlockEnds.end() )
				conditionBody += CStrW(L" )");

			if ( std::distance(it, trigger.conditions.end()) != 1 )
				conditionBody += linkLogic[it->linkLogic];
		}

		conditionBody += CStrW(L" );");
	}

	CStrW effectBody;
	
	for ( std::list<MapTriggerEffect>::const_iterator it = trigger.effects.begin();
													it != trigger.effects.end(); ++it )
	{
		effectBody += it->functionName;
		effectBody += CStrW(L"(");
		for ( std::list<CStrW>::const_iterator it2 = it->parameters.begin(); it2 != 
													it->parameters.end(); ++it2 )
		{
			size_t distance = std::distance(it->parameters.begin(), it2);
			//Take display parameter and translate into JS usable code...evilness
			CTriggerSpec spec = *std::find( m_EffectSpecs.begin(), m_EffectSpecs.end(), it->displayName );
			const std::set<TriggerParameter>& specParameters = spec.GetParameters();
			
			//Don't use specialized find, since we're searching for a different member
			std::set<TriggerParameter>::const_iterator specParam = std::find( 
							specParameters.begin(), specParameters.end(), (int)distance);
			std::wstring combined = std::wstring( it->functionName + specParam->name );
			size_t translatedIndex = std::distance( m_TriggerChoices[combined].begin(), 
				std::find(m_TriggerChoices[combined].begin(), m_TriggerChoices[combined].end(), std::wstring(*it2)) );

			if ( m_TriggerTranslations[combined].empty() )
				effectBody += *it2;
			else
				effectBody += m_TriggerTranslations[combined][translatedIndex];	
			std::list<CStrW>::const_iterator endIt = it->parameters.end();
			if ( std::distance(it2, endIt) != 1 )
				effectBody += CStrW(L", ");
		}

		effectBody += CStrW(L");");
	}

	group.triggers.push_back(trigger);
	CTrigger* newTrigger = new CTrigger(trigger.name, trigger.active, trigger.timeValue,
									trigger.maxRunCount, conditionBody, effectBody);
	AddTrigger(newTrigger);
}


//XML stuff

bool CTriggerManager::LoadXml( const CStr& filename )
{
	CXeromyces XeroFile;

	if ( XeroFile.Load( filename.c_str() ) != PSRETURN_OK )
		return false;

	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)

	EL(condition);
	EL(effect);
	EL(definitions);

	#undef EL
	#undef AT
	
	//Return false on any error to be extra sure the point gets across (FIX IT)
	XMBElement root = XeroFile.GetRoot();
	
	if ( root.GetNodeName() != el_definitions )
		return false;

	XERO_ITER_EL(root, rootChild)
	{
		int name = rootChild.GetNodeName();
		if ( name == el_condition )
		{
			if ( !LoadTriggerSpec(rootChild, XeroFile, true) )
			{
				LOG(CLogger::Error, LOG_CATEGORY, "Error detected in Trigger XML <condition> tag. File: %s", filename.c_str());
				return false;
			}
		}
		else if ( name == el_effect )
		{
			if ( !LoadTriggerSpec(rootChild, XeroFile, false) )
			{
				LOG(CLogger::Error, LOG_CATEGORY, "Error detected in Trigger XML <effect> tag. File: %s", filename.c_str());
				return false;
			}
		}
		else 
		{
			LOG(CLogger::Error, LOG_CATEGORY, "Invalid tag in trigger XML. File: %s", filename.c_str());
			return false;
		}
	}
	return true;
}

bool CTriggerManager::LoadTriggerSpec( XMBElement condition, CXeromyces& XeroFile, bool isCondition )
{
	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)

	EL(parameter);
	EL(window);
	EL(inputtype);
	EL(parameterorder);
	EL(windowrow);
	EL(choices);
	EL(choicetranslation);

	AT(type);
	AT(position);
	AT(name);
	AT(size);
	AT(function);
	AT(funcParameters);

	#undef EL
	#undef AT
	
	CTriggerSpec specStore;
	specStore.functionName = CStrW( condition.GetAttributes().GetNamedItem(at_function) );
	specStore.displayName = CStrW( condition.GetAttributes().GetNamedItem(at_name) );
	specStore.funcParameters = CStr( condition.GetAttributes().GetNamedItem(at_funcParameters) ).ToInt();
	int row = -1, column = -1;

	XERO_ITER_EL(condition, child)
	{
		int childID = child.GetNodeName();
		
		if ( childID == el_windowrow )
		{
			++row;
			column = -1;
			
			XERO_ITER_EL(child, windowChild)
			{
				++column;
				TriggerParameter specParam(row, column);
				specParam.name = windowChild.GetAttributes().GetNamedItem(at_name);
				childID = windowChild.GetNodeName();
				
				if ( childID != el_parameter)
					return false;

				XERO_ITER_EL(windowChild, parameterChild)
				{
					childID = parameterChild.GetNodeName();
					
					if ( childID == el_window )
					{
						CStrW type( parameterChild.GetAttributes().GetNamedItem(at_type) );
						CStrW pos( parameterChild.GetAttributes().GetNamedItem(at_position) );
						CStrW size( parameterChild.GetAttributes().GetNamedItem(at_size) );
						specParam.SetWindowData(type, pos, size);
					}
					
					else if ( childID == el_inputtype )
						specParam.inputType = CStrW( parameterChild.GetText() );
					else if ( childID == el_parameterorder )
						specParam.parameterOrder = CStrW( parameterChild.GetText() ).ToInt();
					else if ( childID == el_choices )
					{
						std::vector<std::wstring> choices;
						CStrW comma(L","), input(parameterChild.GetText());
						CStrW substr;
						while ( (substr = input.BeforeFirst(comma)) != input )
						{
							choices.push_back(substr);
							input = input.AfterFirst(comma);
						}

						choices.push_back(substr);	//Last element has no comma
						m_TriggerChoices[specStore.functionName + specParam.name] = choices;
					}
					else if ( childID == el_choicetranslation )
					{
						std::vector<std::wstring> choices;
						CStrW comma(L","), input(parameterChild.GetText());
						CStrW substr;
						while ( (substr = input.BeforeFirst(comma)) != input )
						{
							choices.push_back(substr);
							input = input.AfterFirst(comma);
						}

						choices.push_back(substr);	//Last element has no comma
						m_TriggerTranslations[specStore.functionName + specParam.name] = choices;
					}
					else
						return false;
				}
				specStore.AddParameter(specParam);
			}
		}
		else
			return false;
	}

	if ( isCondition )
		m_ConditionSpecs.push_back(specStore);
	else
		m_EffectSpecs.push_back(specStore);

	return true;
}
