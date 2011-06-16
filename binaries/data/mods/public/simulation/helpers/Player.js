
/**
 * Used to create player entities prior to reading the rest of a map,
 * all other initialization must be done after loading map (terrain/entities).
 * DO NOT use other components here, as they may fail unpredictably.
 */
function LoadPlayerSettings(settings)
{
	// Default settings
	if (!settings)
	{
		settings = {};
	}

	// Get default player data
	var rawData = Engine.ReadJSONFile("player_defaults.json");
	if (!(rawData && rawData.PlayerData))
	{
		throw("Player.js: Error reading player default data (player_defaults.json)");
	}

	var playerDefaults = rawData.PlayerData;
	
	// default number of players
	var numPlayers = 8;
	
	if (settings.PlayerData)
	{	//Get number of players including gaia
		numPlayers = settings.PlayerData.length + 1;
	}
	else
	{
		warn("Player.js: Setup has no player data - using defaults");
	}
	
	// Get player manager
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	
	var teams = [];
	var diplomacy = [];
	
	// Build team + diplomacy data
	for (var i = 0; i < (numPlayers - 1); ++i)
	{
		diplomacy[i] = cmpPlayerMan.Diplomacy.ENEMY;
		
		var pData = settings.PlayerData ? settings.PlayerData[i] : {};
		var pDefs = playerDefaults ? playerDefaults[i+1] : {};
		var team = getSetting(pData, pDefs, "Team");
		
		// If team defined, add player to the team
		if (team !== undefined && team != -1)
		{
			if (!teams[team])
			{
				teams[team] = [i];
			}
			else
			{
				teams[team].push(i);
			}
		}
	}

	for (var i = 0; i < numPlayers; ++i)
	{
		// Add player entity to engine
		var entID = Engine.AddEntity("special/player");
		
		// Retrieve entity
		var player = Engine.QueryInterface(entID, IID_Player);
		if (!player)
		{
			throw("Player.js: Error creating player entity "+i);
		}
		
		player.SetPlayerID(i);
		
		var pDefs = playerDefaults ? playerDefaults[i] : {};
		
		// Use real player data if available
		if (i > 0)
		{
			var pData = settings.PlayerData ? settings.PlayerData[i-1] : {};
			
			// Copy player data if not gaia
			player.SetName(getSetting(pData, pDefs, "Name"));
			player.SetCiv(getSetting(pData, pDefs, "Civ"));
			
			var colour = getSetting(pData, pDefs, "Colour");
			player.SetColour(colour.r, colour.g, colour.b);
			
			if (getSetting(pData, pDefs, "PopulationLimit") !== undefined)
			{
				player.SetPopulationLimit(getSetting(pData, pDefs, "PopulationLimit"));
			}
			
			if (getSetting(pData, pDefs, "Resources") !== undefined)
			{
				player.SetResourceCounts(getSetting(pData, pDefs, "Resources"));
			}
			
			var team = getSetting(pData, pDefs, "Team");
			
			//If diplomacy array exists use that, otherwise use team data or default diplomacy
			if (getSetting(pData, pDefs, "Diplomacy") !== undefined)
			{
				player.SetDiplomacy(getSetting(pData, pDefs, "Diplomacy"));
			}
			else if (team !== undefined && team != -1)
			{
				//Team exists, copy default diplomacy
				var teamDiplomacy = [];
				for (var p in diplomacy)
				{
					teamDiplomacy[p] = diplomacy[p];
				}
				// Set teammates to allies
				var myTeam = teams[team];
				for (var n in myTeam)
				{
					teamDiplomacy[myTeam[n]] = cmpPlayerMan.Diplomacy.ALLY; //Set ally
				}
				
				player.SetDiplomacy(teamDiplomacy);
			}
			else
			{	//Set default
				player.SetDiplomacy(diplomacy);
			}
			
			var startCam = getSetting(pData, pDefs, "StartingCamera");
			if (startCam !== undefined)
			{
				player.SetStartingCamera(startCam.Position, startCam.Rotation);
			}
		}
		else
		{	// Copy gaia data from defaults
			player.SetName(pDefs.Name);
			player.SetCiv(pDefs.Civ);
			player.SetColour(pDefs.Colour.r, pDefs.Colour.g, pDefs.Colour.b);
			player.SetDiplomacy(diplomacy);
		}
		
		// Add player to player manager
		cmpPlayerMan.AddPlayer(entID);
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
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (!cmpOwnership)
		return null;

	var playerEnt = cmpPlayerMan.GetPlayerByID(cmpOwnership.GetOwner());
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
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	var playerEnt = cmpPlayerMan.GetPlayerByID(id);
	if (!playerEnt)
		return null;
	
	return Engine.QueryInterface(playerEnt, iid);
}

/**
 * Returns true if the entity 'target' is owned by an ally of
 * the owner of 'entity'.
 */
function IsOwnedByAlly(entity, target)
{
	// Figure out which player controls us
	var owner = 0;
	var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
	if (cmpOwnership)
		owner = cmpOwnership.GetOwner();

	// Figure out which player controls the foundation being built
	var targetOwner = 0;
	var cmpOwnershipTarget = Engine.QueryInterface(target, IID_Ownership);
	if (cmpOwnershipTarget)
		targetOwner = cmpOwnershipTarget.GetOwner();

	// Players are always implicitly their own ally
	if (owner == targetOwner)
		return true;

	// Get our diplomacy array
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var player = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
	var diplomacy = player.GetDiplomacy();

	// Check for allied diplomacy status
	if (diplomacy[targetOwner - 1] > 0)
		return true;

	return false;
}

Engine.RegisterGlobal("LoadPlayerSettings", LoadPlayerSettings);
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);
Engine.RegisterGlobal("QueryPlayerIDInterface", QueryPlayerIDInterface);
Engine.RegisterGlobal("IsOwnedByAlly", IsOwnedByAlly);
