/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "../Shareable.h"

#include "ps/Game.h"
#include "graphics/GameView.h"
#include "graphics/CinemaManager.h"
#include "simulation2/Simulation2.h"

namespace AtlasMessage {

/*
sTriggerSpec TriggerSpecToAtlas(const CTriggerSpec& spec)
{
	sTriggerSpec atlasSpec;
	std::vector< sTriggerParameter > atlasParameters;
	const std::set<TriggerParameter>& parameters = spec.GetParameters();
	
	for ( std::set<TriggerParameter>::const_iterator it=parameters.begin(); it!=parameters.end(); ++it )
	{
		sTriggerParameter atlasParam;
		atlasParam.column = it->column;
		atlasParam.inputType = it->inputType;
		atlasParam.name = it->name;
		atlasParam.parameterOrder = it->parameterOrder;
		atlasParam.row = it->row;
		
		atlasParam.windowType = it->windowType;
		atlasParam.xPos = it->xPos;
		atlasParam.yPos = it->yPos;
		atlasParam.xSize = it->xSize;
		atlasParam.ySize = it->ySize;
		atlasParameters.push_back(atlasParam);
	}
	atlasSpec.functionName = spec.functionName;
	atlasSpec.displayName = spec.displayName;
	atlasSpec.parameters = atlasParameters;
	return atlasSpec;
}

sTrigger TriggerToAtlas(const MapTrigger& trigger)
{
	sTrigger atlasTrigger;
	atlasTrigger.active = trigger.active;
	atlasTrigger.timeValue = trigger.timeValue;
	atlasTrigger.maxRuns = trigger.maxRunCount;
	atlasTrigger.group = trigger.groupName;
	atlasTrigger.name = trigger.name;
	
	std::vector<bool> atlasNots;
	std::vector<int> atlasBlocks, atlasBlockEnds;
	
	for ( std::set<MapTriggerLogicBlock>::const_iterator it = trigger.logicBlocks.begin();
												it != trigger.logicBlocks.end(); ++it )
	{
		atlasBlocks.push_back( (int)it->index );
		atlasNots.push_back( it->negated );
	}
	for ( std::set<size_t>::const_iterator it = trigger.logicBlockEnds.begin();
										it != trigger.logicBlockEnds.end(); ++it )
	{
		atlasBlockEnds.push_back( (int)*it );
	}
	atlasTrigger.logicNots = atlasNots;
	atlasTrigger.logicBlocks = atlasBlocks;
	atlasTrigger.logicBlockEnds = atlasBlockEnds;
	
	std::vector<sTriggerCondition> atlasConditions;
	std::vector<sTriggerEffect> atlasEffects;
	
	for ( std::list<MapTriggerCondition>::const_iterator it = trigger.conditions.begin();
											it != trigger.conditions.end(); ++it )
	{
		sTriggerCondition atlasCondition;
		atlasCondition.linkLogic = it->linkLogic;
		atlasCondition.name = it->name;
		atlasCondition.functionName = it->functionName;
		atlasCondition.displayName = it->displayName;
		atlasCondition.negated = it->negated;
		std::vector<std::wstring> parameters;
		
		for ( std::list<CStrW>::const_iterator it2=it->parameters.begin();
													it2 != it->parameters.end(); ++it2 )
		{
			parameters.push_back( std::wstring(it2->c_str()) );
		}
		
		atlasCondition.parameters = parameters;
		atlasConditions.push_back(atlasCondition);
	}

	for ( std::list<MapTriggerEffect>::const_iterator it = trigger.effects.begin();
														it != trigger.effects.end(); ++it )
	{
		sTriggerEffect atlasEffect;
		std::vector<std::wstring> parameters;
		atlasEffect.name = it->name;
		atlasEffect.functionName = it->functionName;
		atlasEffect.displayName = it->displayName;

		for ( std::list<CStrW>::const_iterator it2 = it->parameters.begin();
													it2 != it->parameters.end(); ++it2 )
		{
			parameters.push_back( std::wstring(it2->c_str()) );
		}
		atlasEffect.parameters = parameters;
		atlasEffects.push_back( atlasEffect );
	}

	atlasTrigger.conditions = atlasConditions;
	atlasTrigger.effects = atlasEffects;
	return atlasTrigger;
}

sTriggerGroup GroupToAtlas(const MapTriggerGroup& group)
{
	sTriggerGroup atlasGroup;
	atlasGroup.parentName = group.parentName;
	atlasGroup.name = group.name;
	
	std::vector<sTrigger> atlasTriggers;
	std::vector<std::wstring> atlasChildren;
	for ( std::list<MapTrigger>::const_iterator it = group.triggers.begin();
											it != group.triggers.end(); ++it )
	{
		atlasTriggers.push_back( TriggerToAtlas(*it) );
	}
	for ( std::list<CStrW>::const_iterator it = group.childGroups.begin();
											it != group.childGroups.end(); ++it )
	{
		atlasChildren.push_back( std::wstring(it->c_str()) );
	}
	
	atlasGroup.triggers = atlasTriggers;
	atlasGroup.children = atlasChildren;
	return atlasGroup;
}

MapTrigger AtlasToTrigger(const sTrigger& trigger)
{
	MapTrigger engineTrigger;
	
	engineTrigger.active = trigger.active;
	engineTrigger.groupName = *trigger.group;

	std::vector<int> blockEnds = *trigger.logicBlockEnds, blocks = *trigger.logicBlocks;
	std::copy( blockEnds.begin(), blockEnds.end(), inserter(engineTrigger.logicBlockEnds, engineTrigger.logicBlockEnds.begin()) );
	std::copy( blocks.begin(), blocks.end(), inserter(engineTrigger.logicBlocks, engineTrigger.logicBlocks.begin()) );

	engineTrigger.maxRunCount = trigger.maxRuns;
	engineTrigger.name = *trigger.name;
	engineTrigger.timeValue = trigger.timeValue;
	
	std::vector<sTriggerCondition> conditions = *trigger.conditions;
	std::vector<sTriggerEffect> effects = *trigger.effects;

	for ( std::vector<sTriggerCondition>::const_iterator it = conditions.begin(); it != conditions.end(); ++it )
	{
		engineTrigger.conditions.push_back( MapTriggerCondition() );
		MapTriggerCondition* cond = &engineTrigger.conditions.back();
		
		cond->functionName = *it->functionName;
		cond->displayName = *it->displayName;
		cond->linkLogic = it->linkLogic;
		cond->name = *it->name;
		cond->negated = it->negated;
		
		std::vector<std::wstring> parameters = *it->parameters;
		for ( std::vector<std::wstring>::const_iterator it2 = parameters.begin(); it2 != parameters.end(); ++it2 )
		{
			cond->parameters.push_back( CStrW(*it2) );
		}
	}

	for ( std::vector<sTriggerEffect>::const_iterator it = effects.begin(); it != effects.end(); ++it )
	{
		engineTrigger.effects.push_back( MapTriggerEffect() );
		MapTriggerEffect* effect = &engineTrigger.effects.back();

		effect->functionName = *it->functionName;
		effect->displayName = *it->displayName;
		effect->name = *it->name;
		std::vector<std::wstring> parameters = *it->parameters;

		for ( std::vector<std::wstring>::const_iterator it2 = parameters.begin(); it2 != parameters.end(); ++it2 )
		{
			effect->parameters.push_back( CStrW(*it2) );
		}
	}

	return engineTrigger;
}


MapTriggerGroup AtlasToGroup(const sTriggerGroup& group)
{
	MapTriggerGroup engineGroup;
	engineGroup.parentName = *group.parentName;
	engineGroup.name = *group.name;
	
	std::list<MapTrigger> engineTriggers;
	std::vector<std::wstring> atlasChildren = *group.children;
	std::vector<sTrigger> atlasTriggers = *group.triggers;
	
	for ( std::vector<sTrigger>::const_iterator it = atlasTriggers.begin();
													it != atlasTriggers.end(); ++it )
	{
		engineTriggers.push_back( AtlasToTrigger(*it) );
	}
	for ( std::vector<std::wstring>::const_iterator it = atlasChildren.begin(); it != atlasChildren.end(); ++it )
		engineGroup.childGroups.push_back(*it);
	
	engineGroup.triggers = engineTriggers;
	return engineGroup;
}

std::vector<sTriggerGroup> GetCurrentTriggers()
{
	return std::vector<sTriggerGroup>();

	const std::list<MapTriggerGroup>& groups = g_TriggerManager.GetAllTriggerGroups();
	std::vector<sTriggerGroup> atlasGroups;

	for ( std::list<MapTriggerGroup>::const_iterator it = groups.begin(); it != groups.end(); ++it )
		atlasGroups.push_back( GroupToAtlas(*it) );
	return atlasGroups;
}

void SetCurrentTriggers(const std::vector<sTriggerGroup>& groups)
{
	std::list<MapTriggerGroup> engineGroups;
	for ( std::vector<sTriggerGroup>::const_iterator it = groups.begin(); it != groups.end(); ++it )
		engineGroups.push_back( AtlasToGroup(*it) );
	g_TriggerManager.SetAllGroups(engineGroups);
}
*/

QUERYHANDLER(GetTriggerData)
{
	UNUSED2(msg);
/*
	const std::list<CTriggerCondition>& conditions = g_TriggerManager.GetAllConditions();
	const std::list<CTriggerEffect>& effects = g_TriggerManager.GetAllEffects();
	std::vector<sTriggerSpec> atlasConditions;
	std::vector<sTriggerSpec> atlasEffects;

	for ( std::list<CTriggerCondition>::const_iterator it=conditions.begin(); it!=conditions.end(); ++it )
		atlasConditions.push_back( TriggerSpecToAtlas(*it) );
	for ( std::list<CTriggerEffect>::const_iterator it=effects.begin(); it!=effects.end(); ++it )
		atlasEffects.push_back( TriggerSpecToAtlas(*it) );
	
	msg->conditions = atlasConditions;
	msg->effects = atlasEffects;
	msg->groups = GetCurrentTriggers();
*/
}

QUERYHANDLER(GetTriggerChoices)
{
	UNUSED2(msg);
/*
	CStrW selectedName(*msg->name);
	std::vector<std::wstring> choices = g_TriggerManager.GetTriggerChoices(selectedName);
	std::vector<std::wstring> translations = g_TriggerManager.GetTriggerTranslations(selectedName);

	if ( choices.empty() )
		return;
	
	//If a special list (i.e. uses engine data)
	if ( choices.size() == 1 )
	{
		
		if ( choices[0] == std::wstring(L"ATLAS_CINEMA_LIST") )
		{
			choices.clear();
			const std::map<CStrW, CCinemaPath>& paths = g_Game->GetView()->GetCinema()->GetAllPaths();
			for ( std::map<CStrW, CCinemaPath>::const_iterator it = paths.begin(); it != paths.end(); ++it )
			{
				choices.push_back(it->first);
				translations.push_back( L"\"" + it->first + L"\"" );	//Strings need quotes in JS
			}
		}
		else if ( choices[0] == std::wstring(L"ATLAS_TRIGGER_LIST") )
		{
			choices.clear();
			const std::list<MapTriggerGroup>& groups = g_TriggerManager.GetAllTriggerGroups();
			for ( std::list<MapTriggerGroup>::const_iterator it = groups.begin();
															it != groups.end(); ++it )
			{
				for ( std::list<MapTrigger>::const_iterator it2 = it->triggers.begin();
													it2 != it->triggers.end(); ++it2 )
				{
					choices.push_back(it2->name);
					translations.push_back( L"\"" + it2->name + L"\"" );
				}
			}
		}
		else if ( choices[0] == std::wstring(L"ATLAS_TRIG_GROUP_LIST") )
		{
			choices.clear();
			const std::list<MapTriggerGroup>& groups = g_TriggerManager.GetAllTriggerGroups();
			for ( std::list<MapTriggerGroup>::const_iterator it = groups.begin();
															it != groups.end(); ++it )
			{
				choices.push_back(it->name);
				translations.push_back( L"\"" + it->name + L"\"" );
			}
		}
		else
			debug_warn(L"Invalid special choice list for trigger specification parameter");
	}
	msg->choices = choices;
	msg->translations = translations;
*/
}

QUERYHANDLER(GetWorldPosition)
{
	Position pos;
	pos.type1.x = msg->x;
	pos.type1.y = msg->y;
	CVector3D worldPos = pos.GetWorldSpace();
	Position ret(worldPos.X, worldPos.Y, worldPos.Z);
	msg->position = ret;
}
BEGIN_COMMAND(SetAllTriggers)
{
//	std::vector<sTriggerGroup> m_oldGroups, m_newGroups;

	void Do()
	{
//		m_oldGroups = GetCurrentTriggers();
//		m_newGroups = *msg->groups;
//		Redo();
	}
	void Redo()
	{
//		SetCurrentTriggers(m_newGroups);
	}
	void Undo()
	{
//		SetCurrentTriggers(m_oldGroups);
	}
};
END_COMMAND(SetAllTriggers)


MESSAGEHANDLER(TriggerToggleSelector)
{
	//TODO: Draw stuff
	UNUSED2(msg);
}

}
