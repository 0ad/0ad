// Contains standardized functions suitable for using in trigger scripts.
// Do not use them in any other simulation script.

var TriggerHelper = {};

TriggerHelper.GetPlayerIDFromEntity = function(ent)
{
	let cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	if (cmpPlayer)
		return cmpPlayer.GetPlayerID();

	return -1;
};

TriggerHelper.IsInWorld = function(ent)
{
	let cmpPosition = Engine.QueryInterface(ent, IID_Position);
	return cmpPosition && cmpPosition.IsInWorld();
};

TriggerHelper.GetEntityPosition2D = function(ent)
{
	let cmpPosition = Engine.QueryInterface(ent, IID_Position);

	if (!cmpPosition || !cmpPosition.IsInWorld())
		return undefined;

	return cmpPosition.GetPosition2D();
};

TriggerHelper.GetOwner = function(ent)
{
	let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
		return cmpOwnership.GetOwner();

	return -1;
};

/**
 * This returns the mapsize in number of tiles, the value corresponding to map_sizes.json, also used by random map scripts.
 */
TriggerHelper.GetMapSizeTiles = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetTilesPerSide();
};

/**
 * This returns the mapsize in the the coordinate system used in the simulation/, especially cmpPosition.
 */
TriggerHelper.GetMapSizeTerrain = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetMapSize();
};

/**
 * Returns the elapsed ingame time in milliseconds since the gamestart.
 */
TriggerHelper.GetTime = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime();
};

 /**
  * Returns the elpased ingame time in minutes since the gamestart. Useful for balancing.
  */
TriggerHelper.GetMinutes = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;
};

TriggerHelper.GetEntitiesByPlayer = function(playerID)
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(playerID);
};

TriggerHelper.GetAllPlayersEntities = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetNonGaiaEntities();
};

TriggerHelper.SetUnitStance = function(ent, stance)
{
	let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.SwitchToStance(stance);
};

TriggerHelper.SetUnitFormation = function(playerID, entities, formation)
{
	ProcessCommand(playerID, {
		"type": "formation",
		"entities": entities,
		"name": formation
	});
};

/**
 * Can be used to "force" a building/unit to spawn a group of entities.
 *
 * @param source Entity id of the point where they will be spawned from
 * @param template Name of the template
 * @param count Number of units to spawn
 * @param owner Player id of the owner of the new units. By default, the owner
 * of the source entity.
 */
TriggerHelper.SpawnUnits = function(source, template, count, owner)
{
	let entities = [];
	let cmpFootprint = Engine.QueryInterface(source, IID_Footprint);
	let cmpPosition = Engine.QueryInterface(source, IID_Position);

	if (!cmpPosition || !cmpPosition.IsInWorld())
	{
		error("tried to create entity from a source without position");
		return entities;
	}

	if (owner == null)
		owner = TriggerHelper.GetOwner(source);

	for (let i = 0; i < count; ++i)
	{
		let ent = Engine.AddEntity(template);
		let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpEntPosition)
		{
			Engine.DestroyEntity(ent);
			error("tried to create entity without position");
			continue;
		}

		let cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpEntOwnership)
			cmpEntOwnership.SetOwner(owner);

		entities.push(ent);

		let pos;
		if (cmpFootprint)
			pos = cmpFootprint.PickSpawnPoint(ent);

		// TODO this can happen if the player build on the place
		// where our trigger point is
		// We should probably warn the trigger maker in some way,
		// but not interrupt the game for the player
		if (!pos || pos.y < 0)
			pos = cmpPosition.GetPosition();

		cmpEntPosition.JumpTo(pos.x, pos.z);
	}

	return entities;
};

/**
 * Can be used to spawn garrisoned units inside a building/ship.
 *
 * @param entity Entity id of the garrison holder in which units will be garrisoned
 * @param template Name of the template
 * @param count Number of units to spawn
 * @param owner Player id of the owner of the new units. By default, the owner
 * of the garrisonholder entity.
 */
TriggerHelper.SpawnGarrisonedUnits = function(entity, template, count, owner)
{
	let entities = [];

	let cmpGarrisonHolder = Engine.QueryInterface(entity, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
	{
		error("tried to create garrisoned entities inside a non-garrisonholder");
		return entities;
	}

	if (owner == null)
		owner = TriggerHelper.GetOwner(entity);

	for (let i = 0; i < count; ++i)
	{
		let ent = Engine.AddEntity(template);

		let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpOwnership)
			cmpOwnership.SetOwner(owner);

		if (cmpGarrisonHolder.Garrison(ent))
		{
			let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			if (cmpUnitAI)
				cmpUnitAI.Autogarrison(entity);

			entities.push(ent);
		}
		else
			error("failed to garrison entity " + ent + " (" + template + ") inside " + entity);
	}

	return entities;
};

/**
 * Spawn units from all trigger points with this reference
 * If player is defined, only spaw units from the trigger points
 * that belong to that player
 * @param ref Trigger point reference name to spawn units from
 * @param template Template name
 * @param count Number of spawned entities per Trigger point
 * @param owner Owner of the spawned units. Default: the owner of the origins
 * @return A list of new entities per origin like
 * {originId1: [entId1, entId2], originId2: [entId3, entId4], ...}
 */
TriggerHelper.SpawnUnitsFromTriggerPoints = function(ref, template, count, owner = null)
{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	let triggerPoints = cmpTrigger.GetTriggerPoints(ref);

	let entities = {};
	for (let point of triggerPoints)
		entities[point] = TriggerHelper.SpawnUnits(point, template, count, owner);

	return entities;
};

/**
 * Returns the resource type that can be gathered from an entity
 */
TriggerHelper.GetResourceType = function(entity)
{
	let cmpResourceSupply = Engine.QueryInterface(entity, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return undefined;

	return cmpResourceSupply.GetType();
};

/**
 * The given player will win the game.
 * If it's not a last man standing game, then allies will win too and others will be defeated.
 *
 * @param {number} playerID - The player who will win.
 * @param {function} victoryReason - Function that maps from number to plural string, for example
 *     n => markForPluralTranslation(
 *         "%(lastPlayer)s has won (game mode).",
 *         "%(players)s and %(lastPlayer)s have won (game mode).",
 *         n));
 * It's a function since we don't know in advance how many players will have won.
 */
TriggerHelper.SetPlayerWon = function(playerID, victoryReason, defeatReason)
{
	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	cmpEndGameManager.MarkPlayerAndAlliesAsWon(playerID, victoryReason, defeatReason);
};

/**
 * Defeats a single player.
 *
 * @param {number} - ID of that player.
 * @param {string} - String to be shown in chat, for example
 *                   markForTranslation("%(player)s has been defeated (objective).")
 */
TriggerHelper.DefeatPlayer = function(playerID, defeatReason)
{
	let cmpPlayer = QueryPlayerIDInterface(playerID);
	if (cmpPlayer)
		cmpPlayer.SetState("defeated", defeatReason);
};

/**
 * Returns the number of current players
 */
TriggerHelper.GetNumberOfPlayers = function()
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
};

/**
 * A function to determine if an entity matches specific classes.
 * See globalscripts/Templates.js for details of MatchesClassList.
 *
 * @param entity - ID of the entity that we want to check for classes.
 * @param classes - List of the classes we are checking if the entity matches.
 */
TriggerHelper.EntityMatchesClassList = function(entity, classes)
{
	let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), classes);
};

TriggerHelper.MatchEntitiesByClass = function(entities, classes)
{
	return entities.filter(ent => TriggerHelper.EntityMatchesClassList(ent, classes));
};

TriggerHelper.GetPlayerEntitiesByClass = function(playerID, classes)
{
	return TriggerHelper.MatchEntitiesByClass(TriggerHelper.GetEntitiesByPlayer(playerID), classes);
};

TriggerHelper.GetAllPlayersEntitiesByClass = function(classes)
{
	return TriggerHelper.MatchEntitiesByClass(TriggerHelper.GetAllPlayersEntities(), classes);
};

/**
 * Return valid gaia-owned spawn points on land in neutral territory.
 * If there are none, use those available in player-owned territory.
 */
TriggerHelper.GetLandSpawnPoints = function()
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
	let cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	let neutralSpawnPoints = [];
	let nonNeutralSpawnPoints = [];

	for (let ent of cmpRangeManager.GetEntitiesByPlayer(0))
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		let cmpPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpIdentity || !cmpPosition || !cmpPosition.IsInWorld())
			continue;

		let templateName = cmpTemplateManager.GetCurrentTemplateName(ent);
		if (!templateName)
			continue;

		let template = cmpTemplateManager.GetTemplate(templateName);
		if (!template || template.UnitMotionFlying)
			continue;

		let pos = cmpPosition.GetPosition();
		if (pos.y <= cmpWaterManager.GetWaterLevel(pos.x, pos.z))
			continue;

		if (cmpTerritoryManager.GetOwner(pos.x, pos.z) == 0)
			neutralSpawnPoints.push(ent);
		else
			nonNeutralSpawnPoints.push(ent);
	}

	return neutralSpawnPoints.length ? neutralSpawnPoints : nonNeutralSpawnPoints;
};

TriggerHelper.HasDealtWithTech = function(playerID, techName)
{
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let playerEnt = cmpPlayerManager.GetPlayerByID(playerID);
	let cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
	return cmpTechnologyManager && (cmpTechnologyManager.IsTechnologyQueued(techName) ||
	                                cmpTechnologyManager.IsTechnologyStarted(techName) ||
	                                cmpTechnologyManager.IsTechnologyResearched(techName));
};

/**
 * Returns all names of templates that match the given identity classes, constrainted to an optional civ.
 *
 * @param {String} classes - See MatchesClassList for the accepted formats, for example "Class1 Class2+!Class3".
 * @param [String] civ - Optionally only retrieve templates of the given civ. Can be left undefined.
 * @param [String] packedState - When retrieving siege engines filter for the "packed" or "unpacked" state
 * @param [String] rank - If given, only return templates that have no or the given rank. For example "Elite".
 * @param [Boolean] excludeBarracksVariants - Optionally exclude templates whose name ends with "_barracks"
 */
TriggerHelper.GetTemplateNamesByClasses = function(classes, civ, packedState, rank, excludeBarracksVariants)
{
	let templateNames = [];
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for (let templateName of cmpTemplateManager.FindAllTemplates(false))
	{
		if (templateName.startsWith("campaigns/army_"))
			continue;

		if (excludeBarracksVariants && templateName.endsWith("_barracks"))
			continue;

		let template = cmpTemplateManager.GetTemplate(templateName);

		if (civ && (!template.Identity || template.Identity.Civ != civ))
			continue;

		if (!MatchesClassList(GetIdentityClasses(template.Identity), classes))
			continue;

		if (rank && template.Identity.Rank && template.Identity.Rank != rank)
			continue;

		if (packedState && template.Pack && packedState != template.Pack.State)
			continue;

		templateNames.push(templateName);
	}

	return templateNames;
};
/**
 * Composes a random set of the given templates of the given total size.
 *
 * @param {String[]} templateNames - for example ["brit_infantry_javelinist_b", "brit_cavalry_swordsman_e"]
 * @param {Number} totalCount - total amount of templates, in this  example 12
 * @returns an object where the keys are template names and values are amounts,
 *          for example { "brit_infantry_javelinist_b": 4, "brit_cavalry_swordsman_e": 8 }
 */
TriggerHelper.RandomTemplateComposition = function(templateNames, totalCount)
{
	let frequencies = templateNames.map(() => randFloat(0, 1));
	let frequencySum = frequencies.reduce((sum, frequency) => sum + frequency, 0);

	let remainder = totalCount;
	let templateCounts = {};

	for (let i = 0; i < templateNames.length; ++i)
	{
		let count = i == templateNames.length - 1 ? remainder : Math.min(remainder, Math.round(frequencies[i] / frequencySum * totalCount));
		if (!count)
			continue;

		templateCounts[templateNames[i]] = count;
		remainder -= count;
	}

	return templateCounts;
};

/**
 * Composes a random set of the given templates so that the sum of templates matches totalCount.
 * For each template array that has a count item, it choses exactly that number of templates at random.
 * The remaining template arrays are chosen depending on the given frequency.
 * If a unique_entities array is given, it will only select the template if none of the given entityIDs
 * already have that entity (useful to let heroes remain unique).
 *
 * @param {Object[]} templateBalancing - for example
 *     [
 *        { "templates": ["template1", "template2"], "frequency": 2 },
 *        { "templates": ["template3"], "frequency": 1 },
 *        { "templates": ["hero1", "hero2"], "unique_entities": [380, 495], "count": 1 }
 *     ]
 * @param {Number} totalCount - total amount of templates, for example 5.
 *
 * @returns an object where the keys are template names and values are amounts,
 *    for example { "template1": 1, "template2": 3, "template3": 2, "hero1": 1 }
 */
TriggerHelper.BalancedTemplateComposition = function(templateBalancing, totalCount)
{
	// Remove all unavailable unique templates (heroes) and empty template arrays
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let templateBalancingFiltered = [];
	for (let templateBalance of templateBalancing)
	{
		let templateBalanceNew = clone(templateBalance);

		if (templateBalanceNew.unique_entities)
			templateBalanceNew.templates = templateBalanceNew.templates.filter(templateName =>
				templateBalanceNew.unique_entities.every(ent => templateName != cmpTemplateManager.GetCurrentTemplateName(ent)));

		if (templateBalanceNew.templates.length)
			templateBalancingFiltered.push(templateBalanceNew);
	}

	// Helper function to add randomized templates to the result
	let remainder = totalCount;
	let results = {};
	let addTemplates = (templateNames, count) => {
		let templateCounts = TriggerHelper.RandomTemplateComposition(templateNames, count);
		for (let templateName in templateCounts)
		{
			if (!results[templateName])
				results[templateName] = 0;

			results[templateName] += templateCounts[templateName];
			remainder -= templateCounts[templateName];
		}
	};

	// Add template groups with fixed counts
	for (let templateBalance of templateBalancingFiltered)
		if (templateBalance.count)
			addTemplates(templateBalance.templates, Math.min(remainder, templateBalance.count));

	// Add template groups with frequency weights
	let templateBalancingFrequencies = templateBalancingFiltered.filter(templateBalance => !!templateBalance.frequency);
	let templateBalancingFrequencySum = templateBalancingFrequencies.reduce((sum, templateBalance) => sum + templateBalance.frequency, 0);
	for (let i = 0; i < templateBalancingFrequencies.length; ++i)
		addTemplates(
			templateBalancingFrequencies[i].templates,
			i == templateBalancingFrequencies.length - 1 ?
				remainder :
				Math.min(remainder, Math.round(templateBalancingFrequencies[i].frequency / templateBalancingFrequencySum * totalCount)));

	if (remainder != 0)
		warn("Could not chose as many templates as intended, remaining " + remainder + ", chosen: " + uneval(results));

	return results;
};

/**
 * This will spawn random compositions of entities of the given templates at all garrisonholders of the given targetClass of the given player.
 * The garrisonholder will be filled to capacityPercent.
 * Returns an object where keys are entityIDs of the affected garrisonholders and the properties are template compositions, see RandomTemplateComposition.
 */
TriggerHelper.SpawnAndGarrisonAtClasses = function(playerID, classes, templates, capacityPercent)
{
	let results = {};

	for (let entGarrisonHolder of Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(playerID))
	{
		let cmpIdentity = Engine.QueryInterface(entGarrisonHolder, IID_Identity);
		if (!cmpIdentity || !MatchesClassList(cmpIdentity.GetClassesList(), classes))
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(entGarrisonHolder, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		// TODO: account for already garrisoned entities
		results[entGarrisonHolder] = this.RandomTemplateComposition(templates, Math.floor(cmpGarrisonHolder.GetCapacity() * capacityPercent));

		for (let template in results[entGarrisonHolder])
			TriggerHelper.SpawnGarrisonedUnits(entGarrisonHolder, template, results[entGarrisonHolder][template], playerID);
	}

	return results;
};

Engine.RegisterGlobal("TriggerHelper", TriggerHelper);
