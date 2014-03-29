
/**
 * Used to create player entities prior to reading the rest of a map,
 * all other initialization must be done after loading map (terrain/entities).
 * DO NOT use other components here, as they may fail unpredictably.
 * settings is the object containing settings for this map.
 * newPlayers if true will remove any old player entities and add new ones
 *	(used when loading a map or when Atlas changes the number of players).
 */
function LoadPlayerSettings(settings, newPlayers)
{
	// Default settings
	if (!settings)
		settings = {};

	// Get default player data
	var rawData = Engine.ReadJSONFile("player_defaults.json");
	if (!(rawData && rawData.PlayerData))
		throw("Player.js: Error reading player_defaults.json");

	var playerDefaults = rawData.PlayerData;

	// default number of players
	var numPlayers = 8;

	// Get player manager
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	// Remove existing players and add new ones
	if (newPlayers)
	{
		cmpPlayerManager.RemoveAllPlayers();

		if (settings.PlayerData)
		{
			// Get number of players including gaia
			numPlayers = settings.PlayerData.length + 1;
		}
		else
		{
			warn("Player.js: Setup has no player data - using defaults");
		}

		for (var i = 0; i < numPlayers; ++i)
		{
			// Add player entity to engine
			// TODO: Get player template name from civ data
			var entID = Engine.AddEntity("special/player");
			var cmpPlayer = Engine.QueryInterface(entID, IID_Player);
			if (!cmpPlayer)
				throw("Player.js: Error creating player entity "+i);

			cmpPlayer.SetPlayerID(i);

			// Add player to player manager
			cmpPlayerManager.AddPlayer(entID);

			// Properly autoresearch techs on init.
			var cmpTechManager = Engine.QueryInterface(entID, IID_TechnologyManager);
			if (cmpTechManager !== undefined)
				cmpTechManager.UpdateAutoResearch();
		}
	}

	numPlayers = cmpPlayerManager.GetNumPlayers();

	// Initialize the player data
	for (var i = 0; i < numPlayers; ++i)
	{
		var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
		var pDefs = playerDefaults ? playerDefaults[i] : {};

		// Skip gaia
		if (i > 0)
		{
			var pData = settings.PlayerData ? settings.PlayerData[i-1] : {};

			cmpPlayer.SetName(getSetting(pData, pDefs, "Name"));
			cmpPlayer.SetCiv(getSetting(pData, pDefs, "Civ"));
			var colour = getSetting(pData, pDefs, "Colour");
			cmpPlayer.SetColour(colour.r, colour.g, colour.b);

			// Note: this is not yet implemented but I leave it commented to highlight it's easy
			// If anyone ever adds handicap.
			//if (getSetting(pData, pDefs, "GatherRateMultiplier") !== undefined)
			//	cmpPlayer.SetGatherRateMultiplier(getSetting(pData, pDefs, "GatherRateMultiplier"));

			if (getSetting(pData, pDefs, "PopulationLimit") !== undefined)
				cmpPlayer.SetMaxPopulation(getSetting(pData, pDefs, "PopulationLimit"));

			if (getSetting(pData, pDefs, "Resources") !== undefined)
				cmpPlayer.SetResourceCounts(getSetting(pData, pDefs, "Resources"));

			// If diplomacy explicitly defined, use that; otherwise use teams
			if (getSetting(pData, pDefs, "Diplomacy") !== undefined)
				cmpPlayer.SetDiplomacy(getSetting(pData, pDefs, "Diplomacy"));
			else
			{
				// Init diplomacy
				var myTeam = getSetting(pData, pDefs, "Team");

				// Set all but self as enemies as SetTeam takes care of allies
				for (var j = 0; j < numPlayers; ++j)
				{
					if (i == j)
						cmpPlayer.SetAlly(j);
					else
						cmpPlayer.SetEnemy(j);
				}
				cmpPlayer.SetTeam((myTeam !== undefined) ? myTeam : -1);
			}

			// If formations explicitly defined, use that; otherwise use civ defaults
			var formations = getSetting(pData, pDefs, "Formations");
			if (formations !== undefined)
				cmpPlayer.SetFormations(formations);
			else
			{
				var rawFormations = Engine.ReadCivJSONFile(cmpPlayer.GetCiv()+".json");
				if (!(rawFormations && rawFormations.Formations))
					throw("Player.js: Error reading "+cmpPlayer.GetCiv()+".json");

				cmpPlayer.SetFormations(rawFormations.Formations);
			}

			var startCam = getSetting(pData, pDefs, "StartingCamera");
			if (startCam !== undefined)
				cmpPlayer.SetStartingCamera(startCam.Position, startCam.Rotation);
		}
		else
		{
			// Copy gaia data from defaults
			cmpPlayer.SetName(pDefs.Name);
			cmpPlayer.SetCiv(pDefs.Civ);
			cmpPlayer.SetColour(pDefs.Colour.r, pDefs.Colour.g, pDefs.Colour.b);

			// Gaia should be its own ally.
			cmpPlayer.SetAlly(0);

			// Gaia is everyone's enemy
			for (var j = 1; j < numPlayers; ++j)
				cmpPlayer.SetEnemy(j);
		}
	}

	// NOTE: We need to do the team locking here, as otherwise
	// SetTeam can't ally the players.
	if (settings.LockTeams)
		for (var i = 0; i < numPlayers; ++i)
		{
			var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
			cmpPlayer.SetLockTeams(true);
		}
}

// Get a setting if it exists or return default
function getSetting(settings, defaults, property)
{
	if (settings && (property in settings))
		return settings[property];

	// Use defaults
	if (defaults && (property in defaults))
		return defaults[property];

	return undefined;
}

/**
 * Similar to Engine.QueryInterface but applies to the player entity
 * that owns the given entity.
 * iid is typically IID_Player.
 */
function QueryOwnerInterface(ent, iid)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (!cmpOwnership)
		return null;

	var owner = cmpOwnership.GetOwner();
	if (owner == -1)
		return null;

	var playerEnt = cmpPlayerManager.GetPlayerByID(owner);
	if (!playerEnt)
		return null;

	return Engine.QueryInterface(playerEnt, iid);
}

/**
 * Similar to Engine.QueryInterface but applies to the player entity
 * with the given ID number.
 * iid is typically IID_Player.
 */
function QueryPlayerIDInterface(id, iid)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var playerEnt = cmpPlayerManager.GetPlayerByID(id);
	if (!playerEnt)
		return null;

	return Engine.QueryInterface(playerEnt, iid);
}

/**
 * Returns true if the entity 'target' is owned by an ally of
 * the owner of 'entity'.
 */
function IsOwnedByAllyOfEntity(entity, target)
{
	// Figure out which player controls us
	var owner = 0;
	var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
	if (cmpOwnership)
		owner = cmpOwnership.GetOwner();

	// Figure out which player controls the target entity
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);

	// Check for allied diplomacy status
	if (cmpPlayer.IsAlly(targetOwner))
		return true;

	return false;
}

/**
 * Returns true if the entity 'target' is owned by a mutual ally of
 * the owner of 'entity'.
 */
function IsOwnedByMutualAllyOfEntity(entity, target)
{
	// Figure out which player controls us
	var owner = 0;
	var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
	if (cmpOwnership)
		owner = cmpOwnership.GetOwner();

	// Figure out which player controls the target entity
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);

	// Check for mutual allied diplomacy status
	if (cmpPlayer.IsMutualAlly(targetOwner))
		return true;

	return false;
}

/**
 * Returns true if the entity 'target' is owned by player
 */
function IsOwnedByPlayer(player, target)
{
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	return (cmpOwnershipTarget && player == cmpOwnershipTarget.GetOwner());
}

/**
 * Returns true if the entity 'target' is owned by gaia (player 0)
 */
function IsOwnedByGaia(target)
{
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	return (cmpOwnershipTarget && cmpOwnershipTarget.GetOwner() == 0);
}

/**
 * Returns true if the entity 'target' is owned by an ally of player
 */
function IsOwnedByAllyOfPlayer(player, target)
{
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(player), IID_Player);

	// Check for allied diplomacy status
	if (cmpPlayer.IsAlly(targetOwner))
		return true;

	return false;
}

/**
 * Returns true if the entity 'target' is owned by a mutual ally of player
 */
function IsOwnedByMutualAllyOfPlayer(player, target)
{
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(player), IID_Player);

	// Check for mutual allied diplomacy status
	if (cmpPlayer.IsMutualAlly(targetOwner))
		return true;

	return false;
}

/**
 * Returns true if the entity 'target' is owned by someone neutral to player
 */
function IsOwnedByNeutralOfPlayer(player,target)
{
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(player), IID_Player);

	// Check for neutral diplomacy status
	if (cmpPlayer.IsNeutral(targetOwner))
		return true;

	return false;
}

/**
 * Returns true if the entity 'target' is owned by an enemy of player
 */
function IsOwnedByEnemyOfPlayer(player, target)
{
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(player), IID_Player);

	// Check for enemy diplomacy status
	if (cmpPlayer.IsEnemy(targetOwner))
		return true;

	return false;
}

Engine.RegisterGlobal("LoadPlayerSettings", LoadPlayerSettings);
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);
Engine.RegisterGlobal("QueryPlayerIDInterface", QueryPlayerIDInterface);
Engine.RegisterGlobal("IsOwnedByAllyOfEntity", IsOwnedByAllyOfEntity);
Engine.RegisterGlobal("IsOwnedByMutualAllyOfEntity", IsOwnedByMutualAllyOfEntity);
Engine.RegisterGlobal("IsOwnedByPlayer", IsOwnedByPlayer);
Engine.RegisterGlobal("IsOwnedByGaia", IsOwnedByGaia);
Engine.RegisterGlobal("IsOwnedByAllyOfPlayer", IsOwnedByAllyOfPlayer);
Engine.RegisterGlobal("IsOwnedByMutualAllyOfPlayer", IsOwnedByMutualAllyOfPlayer);
Engine.RegisterGlobal("IsOwnedByNeutralOfPlayer", IsOwnedByNeutralOfPlayer);
Engine.RegisterGlobal("IsOwnedByEnemyOfPlayer", IsOwnedByEnemyOfPlayer);
