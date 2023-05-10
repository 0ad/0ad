/**
 * Used to create player entities prior to reading the rest of a map,
 * all other initialization must be done after loading map (terrain/entities).
 * Be VERY careful in using other components here, as they may not be properly initialised yet.
 * settings is the object containing settings for this map.
 * newPlayers if true will remove old player entities or add new ones until
 * the new number of player entities is obtained
 * (used when loading a map or when Atlas changes the number of players).
 */
function LoadPlayerSettings(settings, newPlayers)
{
	const playerDefaults = Engine.ReadJSONFile("simulation/data/settings/player_defaults.json").PlayerData;
	const playerData = settings.PlayerData;
	if (!playerData)
		warn("Player.js: Setup has no player data - using defaults.");

	const getPlayerSetting = (idx, property) => {
		if (playerData && playerData[idx] && (property in playerData[idx]))
			return playerData[idx][property];

		if (playerDefaults && playerDefaults[idx] && (property in playerDefaults[idx]))
			return playerDefaults[idx][property];

		return undefined;
	};

	// Add gaia to simplify iteration
	// (if gaia is not already the first civ such as when called from Atlas' ActorViewer)
	if (playerData && playerData[0] && (!playerData[0].Civ || playerData[0].Civ != "gaia"))
		playerData.unshift(null);

	if (playerData && !playerData.some(v => v && !!v.AI))
		Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface).Disable();

	const cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let numPlayers = cmpPlayerManager.GetNumPlayers();

	// Remove existing players or add new ones
	if (newPlayers)
	{
		const settingsNumPlayers = playerData?.length ?? playerDefaults.length;

		while (numPlayers < settingsNumPlayers)
			cmpPlayerManager.AddPlayer(GetPlayerTemplateName(getPlayerSetting(numPlayers++, "Civ")));

		for (; numPlayers > settingsNumPlayers; numPlayers--)
			cmpPlayerManager.RemoveLastPlayer();
	}

	// Even when no new player, we must check the template compatibility as player templates are civ dependent.
	const cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for (let i = 0; i < numPlayers; ++i)
	{
		const template = GetPlayerTemplateName(getPlayerSetting(i, "Civ"));
		const entID = cmpPlayerManager.GetPlayerByID(i);
		if (cmpTemplateManager.GetCurrentTemplateName(entID) !== template)
			cmpPlayerManager.ReplacePlayerTemplate(i, template);
	}

	for (let i = 0; i < numPlayers; ++i)
	{
		QueryPlayerIDInterface(i, IID_Identity).SetName(getPlayerSetting(i, "Name"));

		const color = getPlayerSetting(i, "Color");
		const cmpPlayer = QueryPlayerIDInterface(i);
		cmpPlayer.SetColor(color.r, color.g, color.b);

		// Special case for gaia
		if (i == 0)
			continue;

		// PopulationLimit
		{
			const maxPopulation =
				settings.PlayerData[i].PopulationLimit !== undefined ?
					settings.PlayerData[i].PopulationLimit :
				settings.PopulationCap !== undefined ?
					settings.PopulationCap :
				playerDefaults[i].PopulationLimit !== undefined ?
					playerDefaults[i].PopulationLimit :
					undefined;

			if (maxPopulation !== undefined)
				cmpPlayer.SetMaxPopulation(maxPopulation);
		}

		// StartingResources
		if (settings.PlayerData[i].Resources !== undefined)
			cmpPlayer.SetResourceCounts(settings.PlayerData[i].Resources);
		else if (settings.StartingResources)
		{
			let resourceCounts = cmpPlayer.GetResourceCounts();
			let newResourceCounts = {};
			for (let resources in resourceCounts)
				newResourceCounts[resources] = settings.StartingResources;
			cmpPlayer.SetResourceCounts(newResourceCounts);
		}
		else if (playerDefaults[i].Resources !== undefined)
			cmpPlayer.SetResourceCounts(playerDefaults[i].Resources);

		if (settings.DisableSpies)
		{
			cmpPlayer.AddDisabledTechnology("unlock_spies");
			cmpPlayer.AddDisabledTemplate("special/spy");
		}

		// If diplomacy explicitly defined, use that; otherwise use teams.
		const diplomacy = getPlayerSetting(i, "Diplomacy");
		if (diplomacy !== undefined)
			cmpPlayer.SetDiplomacy(diplomacy);
		else
			cmpPlayer.SetTeam(getPlayerSetting(i, "Team") ?? -1);

		const formations = getPlayerSetting(i, "Formations");
		if (formations)
			cmpPlayer.SetFormations(formations);

		const startCam = getPlayerSetting(i, "StartingCamera");
		if (startCam)
			cmpPlayer.SetStartingCamera(startCam.Position, startCam.Rotation);
	}

	// NOTE: We need to do the team locking here, as otherwise
	// SetTeam can't ally the players.
	if (settings.LockTeams)
		for (let i = 0; i < numPlayers; ++i)
			QueryPlayerIDInterface(i).SetLockTeams(true);
}

function GetPlayerTemplateName(civ)
{
	return "special/players/" + civ;
}

/**
 * @param id An entity's ID
 * @returns The entity ID of the owner player (not his player ID) or ent if ent is a player entity.
 */
function QueryOwnerEntityID(ent)
{
	let cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	if (cmpPlayer)
		return ent;

	let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (!cmpOwnership)
		return null;

	let owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER)
		return null;

	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager)
		return null;

	return cmpPlayerManager.GetPlayerByID(owner);
}

/**
 * Similar to Engine.QueryInterface but applies to the player entity
 * that owns the given entity.
 * iid is typically IID_Player.
 */
function QueryOwnerInterface(ent, iid = IID_Player)
{
	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (!cmpOwnership)
		return null;

	var owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER)
		return null;

	return QueryPlayerIDInterface(owner, iid);
}

/**
 * Similar to Engine.QueryInterface but applies to the player entity
 * with the given ID number.
 * iid is typically IID_Player.
 */
function QueryPlayerIDInterface(id, iid = IID_Player)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var playerEnt = cmpPlayerManager.GetPlayerByID(id);
	if (!playerEnt)
		return null;

	return Engine.QueryInterface(playerEnt, iid);
}

/**
 * Similar to Engine.QueryInterface but first checks if the entity
 * mirages the interface.
 */
function QueryMiragedInterface(ent, iid)
{
	let cmpMirage = Engine.QueryInterface(ent, IID_Mirage);
	if (cmpMirage && !cmpMirage.Mirages(iid))
		return null;
	else if (!cmpMirage)
		return Engine.QueryInterface(ent, iid);

	return cmpMirage.Get(iid);
}

/**
 * Similar to Engine.QueryInterface, but checks for all interfaces
 * implementing a builder list (currently Foundation and Repairable)
 * TODO Foundation and Repairable could both implement a BuilderList component
 */
function QueryBuilderListInterface(ent)
{
	return Engine.QueryInterface(ent, IID_Foundation) || Engine.QueryInterface(ent, IID_Repairable);
}

/**
 * Returns true if the entity 'target' is owned by an ally of
 * the owner of 'entity'.
 */
function IsOwnedByAllyOfEntity(entity, target)
{
	return IsOwnedByEntityHelper(entity, target, "IsAlly");
}

function IsOwnedByMutualAllyOfEntity(entity, target)
{
	return IsOwnedByEntityHelper(entity, target, "IsMutualAlly");
}

function IsOwnedByEntityHelper(entity, target, check)
{
	// Figure out which player controls us
	let owner = 0;
	let cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
	if (cmpOwnership)
		owner = cmpOwnership.GetOwner();

	// Figure out which player controls the target entity
	let targetOwner = 0;
	let cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	let cmpPlayer = QueryPlayerIDInterface(owner);

	return cmpPlayer && cmpPlayer[check](targetOwner);
}

/**
 * Returns true if the entity 'target' is owned by player
 */
function IsOwnedByPlayer(player, target)
{
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	return cmpOwnershipTarget && player == cmpOwnershipTarget.GetOwner();
}

function IsOwnedByGaia(target)
{
	return IsOwnedByPlayer(0, target);
}

/**
 * Returns true if the entity 'target' is owned by an ally of player
 */
function IsOwnedByAllyOfPlayer(player, target)
{
	return IsOwnedByHelper(player, target, "IsAlly");
}

function IsOwnedByMutualAllyOfPlayer(player, target)
{
	return IsOwnedByHelper(player, target, "IsMutualAlly");
}

function IsOwnedByNeutralOfPlayer(player, target)
{
	return IsOwnedByHelper(player, target, "IsNeutral");
}

function IsOwnedByEnemyOfPlayer(player, target)
{
	return IsOwnedByHelper(player, target, "IsEnemy");
}

function IsOwnedByHelper(player, target, check)
{
	let targetOwner = 0;
	let cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	let cmpPlayer = QueryPlayerIDInterface(player);

	return cmpPlayer && cmpPlayer[check](targetOwner);
}

Engine.RegisterGlobal("LoadPlayerSettings", LoadPlayerSettings);
Engine.RegisterGlobal("QueryOwnerEntityID", QueryOwnerEntityID);
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);
Engine.RegisterGlobal("QueryPlayerIDInterface", QueryPlayerIDInterface);
Engine.RegisterGlobal("QueryMiragedInterface", QueryMiragedInterface);
Engine.RegisterGlobal("QueryBuilderListInterface", QueryBuilderListInterface);
Engine.RegisterGlobal("IsOwnedByAllyOfEntity", IsOwnedByAllyOfEntity);
Engine.RegisterGlobal("IsOwnedByMutualAllyOfEntity", IsOwnedByMutualAllyOfEntity);
Engine.RegisterGlobal("IsOwnedByPlayer", IsOwnedByPlayer);
Engine.RegisterGlobal("IsOwnedByGaia", IsOwnedByGaia);
Engine.RegisterGlobal("IsOwnedByAllyOfPlayer", IsOwnedByAllyOfPlayer);
Engine.RegisterGlobal("IsOwnedByMutualAllyOfPlayer", IsOwnedByMutualAllyOfPlayer);
Engine.RegisterGlobal("IsOwnedByNeutralOfPlayer", IsOwnedByNeutralOfPlayer);
Engine.RegisterGlobal("IsOwnedByEnemyOfPlayer", IsOwnedByEnemyOfPlayer);
