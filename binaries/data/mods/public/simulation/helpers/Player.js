/**
 * Used to create player entities prior to reading the rest of a map,
 * all other initialization must be done after loading map (terrain/entities).
 * DO NOT use other components here, as they may fail unpredictably.
 * settings is the object containing settings for this map.
 * newPlayers if true will remove old player entities or add new ones until
 * the new number of player entities is obtained
 * (used when loading a map or when Atlas changes the number of players).
 */
function LoadPlayerSettings(settings, newPlayers)
{
	var playerDefaults = Engine.ReadJSONFile("simulation/data/settings/player_defaults.json").PlayerData;

	// Default settings
	if (!settings)
		settings = {};

	// Add gaia to simplify iteration
	// (if gaia is not already the first civ such as when called from Atlas' ActorViewer)
	if (settings.PlayerData && settings.PlayerData[0] &&
		(!settings.PlayerData[0].Civ || settings.PlayerData[0].Civ != "gaia"))
		settings.PlayerData.unshift(null);

	var playerData = settings.PlayerData;

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	// Remove existing players or add new ones
	if (newPlayers)
	{
		var settingsNumPlayers = 9; // default 8 players + gaia

		if (playerData)
			settingsNumPlayers = playerData.length; // includes gaia (see above)
		else
			warn("Player.js: Setup has no player data - using defaults");

		while (settingsNumPlayers > numPlayers)
		{
			// Add player entity to engine
			var entID = Engine.AddEntity(GetPlayerTemplateName(getSetting(playerData, playerDefaults, numPlayers, "Civ")));
			var cmpPlayer = Engine.QueryInterface(entID, IID_Player);
			if (!cmpPlayer)
				throw new Error("Player.js: Error creating player entity " + numPlayers);

			cmpPlayerManager.AddPlayer(entID);
			++numPlayers;
		}

		while (settingsNumPlayers < numPlayers)
		{
			cmpPlayerManager.RemoveLastPlayer();
			--numPlayers;
		}
	}

	// Even when no new player, we must check the template compatibility as player template may be civ dependent
	for (var i = 0; i < numPlayers; ++i)
	{
		var template = GetPlayerTemplateName(getSetting(playerData, playerDefaults, i, "Civ"));
		var entID = cmpPlayerManager.GetPlayerByID(i);
		if (cmpTemplateManager.GetCurrentTemplateName(entID) === template)
			continue;
		// We need to recreate this player to have the right template
		entID = Engine.AddEntity(template);
		cmpPlayerManager.ReplacePlayer(i, entID);
	}

	// Initialize the player data
	for (var i = 0; i < numPlayers; ++i)
	{
		let cmpPlayer = QueryPlayerIDInterface(i);
		cmpPlayer.SetName(getSetting(playerData, playerDefaults, i, "Name"));
		cmpPlayer.SetCiv(getSetting(playerData, playerDefaults, i, "Civ"));

		var color = getSetting(playerData, playerDefaults, i, "Color");
		cmpPlayer.SetColor(color.r, color.g, color.b);

		// Special case for gaia
		if (i == 0)
		{
			// Gaia should be its own ally.
			cmpPlayer.SetAlly(0);

			// Gaia is everyone's enemy
			for (var j = 1; j < numPlayers; ++j)
				cmpPlayer.SetEnemy(j);

			continue;
		}

		// Note: this is not yet implemented but I leave it commented to highlight it's easy
		// If anyone ever adds handicap.
		//if (getSetting(playerData, playerDefaults, i, "GatherRateMultiplier") !== undefined)
		//	cmpPlayer.SetGatherRateMultiplier(getSetting(playerData, playerDefaults, i, "GatherRateMultiplier"));

		if (getSetting(playerData, playerDefaults, i, "PopulationLimit") !== undefined)
			cmpPlayer.SetMaxPopulation(getSetting(playerData, playerDefaults, i, "PopulationLimit"));

		if (getSetting(playerData, playerDefaults, i, "Resources") !== undefined)
			cmpPlayer.SetResourceCounts(getSetting(playerData, playerDefaults, i, "Resources"));

		if (getSetting(playerData, playerDefaults, i, "StartingTechnologies") !== undefined)
			cmpPlayer.SetStartingTechnologies(getSetting(playerData, playerDefaults, i, "StartingTechnologies"));

		if (getSetting(playerData, playerDefaults, i, "DisabledTechnologies") !== undefined)
			cmpPlayer.SetDisabledTechnologies(getSetting(playerData, playerDefaults, i, "DisabledTechnologies"));

		let disabledTemplates = [];
		if (settings.DisabledTemplates !== undefined)
			disabledTemplates = settings.DisabledTemplates;
		if (getSetting(playerData, playerDefaults, i, "DisabledTemplates") !== undefined)
			disabledTemplates = disabledTemplates.concat(getSetting(playerData, playerDefaults, i, "DisabledTemplates"));
		if (disabledTemplates.length)
			cmpPlayer.SetDisabledTemplates(disabledTemplates);

		if (settings.DisableSpies)
		{
			cmpPlayer.AddDisabledTechnology("unlock_spies");
			cmpPlayer.AddDisabledTemplate("special/spy");
		}

		// If diplomacy explicitly defined, use that; otherwise use teams
		if (getSetting(playerData, playerDefaults, i, "Diplomacy") !== undefined)
			cmpPlayer.SetDiplomacy(getSetting(playerData, playerDefaults, i, "Diplomacy"));
		else
		{
			// Init diplomacy
			var myTeam = getSetting(playerData, playerDefaults, i, "Team");

			// Set all but self as enemies as SetTeam takes care of allies
			for (var j = 0; j < numPlayers; ++j)
			{
				if (i == j)
					cmpPlayer.SetAlly(j);
				else
					cmpPlayer.SetEnemy(j);
			}
			cmpPlayer.SetTeam(myTeam === undefined ? -1 : myTeam);
		}

		cmpPlayer.SetFormations(
			getSetting(playerData, playerDefaults, i, "Formations") ||
			Engine.ReadJSONFile("simulation/data/civs/" + cmpPlayer.GetCiv() + ".json").Formations);

		var startCam = getSetting(playerData, playerDefaults, i, "StartingCamera");
		if (startCam !== undefined)
			cmpPlayer.SetStartingCamera(startCam.Position, startCam.Rotation);
	}

	// NOTE: We need to do the team locking here, as otherwise
	// SetTeam can't ally the players.
	if (settings.LockTeams)
		for (let i = 0; i < numPlayers; ++i)
			QueryPlayerIDInterface(i).SetLockTeams(true);

	// Disable the AIIinterface when no AI players are present
	if (playerData && !playerData.some(v => v && !!v.AI))
		Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface).Disable();
}

// Get a setting if it exists or return default
function getSetting(settings, defaults, idx, property)
{
	if (settings && settings[idx] && (property in settings[idx]))
		return settings[idx][property];

	// Use defaults
	if (defaults && defaults[idx] && (property in defaults[idx]))
		return defaults[idx][property];

	return undefined;
}

function GetPlayerTemplateName(civ)
{
	let path = "special/player/player";

	if (Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).TemplateExists(path + "_" + civ))
		return path + "_" + civ;

	return path;
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
	var cmp = Engine.QueryInterface(ent, IID_Mirage);
	if (cmp && !cmp.Mirages(iid))
		return null;
	else if (!cmp)
		cmp = Engine.QueryInterface(ent, iid);

	return cmp;
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

function IsOwnedByNeutralOfPlayer(player,target)
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
