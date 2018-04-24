function PlayerManager() {}

PlayerManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

PlayerManager.prototype.Init = function()
{
	this.playerEntities = []; // list of player entity IDs
};

PlayerManager.prototype.AddPlayer = function(ent)
{
	var id = this.playerEntities.length;
	var cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	cmpPlayer.SetPlayerID(id);
	this.playerEntities.push(ent);
	// initialize / update the diplomacy arrays
	var newDiplo = [];
	for (var i = 0; i < id; i++)
	{
		var cmpOtherPlayer = Engine.QueryInterface(this.GetPlayerByID(i), IID_Player);
		cmpOtherPlayer.diplomacy[id] = -1;
		newDiplo[i] = -1;
	}
	newDiplo[id] = 1;
	cmpPlayer.SetDiplomacy(newDiplo);

	return id;
};

/**
 * To avoid possible problems with cached quantities (as in TechnologyManager),
 * we first remove all entities from this player, and add them back after the replacement.
 * Note: This should only be called during setup/init and not during the game
 */
PlayerManager.prototype.ReplacePlayer = function(id, ent)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var entities = cmpRangeManager.GetEntitiesByPlayer(id);
	for (var e of entities)
	{
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership)
			cmpOwnership.SetOwner(INVALID_PLAYER);
	}

	var oldent = this.playerEntities[id];
	var cmpPlayer = Engine.QueryInterface(oldent, IID_Player);
	var diplo = cmpPlayer.GetDiplomacy();
	var color = cmpPlayer.GetColor();

	var cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	cmpPlayer.SetPlayerID(id);
	this.playerEntities[id] = ent;
	cmpPlayer.SetColor(color);
	cmpPlayer.SetDiplomacy(diplo);

	Engine.DestroyEntity(oldent);
	Engine.FlushDestroyedEntities();

	for (var e of entities)
	{
		var cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership)
			cmpOwnership.SetOwner(id);
	}
};

/**
 * Returns the player entity ID for the given player ID.
 * The player ID must be valid (else there will be an error message).
 */
PlayerManager.prototype.GetPlayerByID = function(id)
{
	if (id in this.playerEntities)
		return this.playerEntities[id];

	// All players at or below ID 0 get gaia-level data. (Observers for example)
	if (id <= 0)
		return this.playerEntities[0];

	var stack = new Error().stack.trimRight().replace(/^/mg, '  '); // indent each line
	warn("GetPlayerByID: no player defined for id '"+id+"'\n"+stack);

	return INVALID_ENTITY;
};

/**
 * Returns the number of players including gaia.
 */
PlayerManager.prototype.GetNumPlayers = function()
{
	return this.playerEntities.length;
};

/**
 * Returns IDs of all players including gaia.
 */
PlayerManager.prototype.GetAllPlayers = function()
{
	let players = [];
	for (let i = 0; i < this.playerEntities.length; ++i)
		players.push(i);
	return players;
};

/**
 * Returns IDs of all players excluding gaia.
 */
PlayerManager.prototype.GetNonGaiaPlayers = function()
{
	let players = [];
	for (let i = 1; i < this.playerEntities.length; ++i)
		players.push(i);
	return players;
};

/**
 * Returns IDs of all players excluding gaia that are not defeated nor have won.
 */
PlayerManager.prototype.GetActivePlayers = function()
{
	return this.GetNonGaiaPlayers().filter(playerID => QueryPlayerIDInterface(playerID).GetState() == "active");
};

PlayerManager.prototype.RemoveAllPlayers = function()
{
	// Destroy existing player entities
	for (var id of this.playerEntities)
		Engine.DestroyEntity(id);

	this.playerEntities = [];
};

PlayerManager.prototype.RemoveLastPlayer = function()
{
	if (this.playerEntities.length == 0)
		return;

	var lastId = this.playerEntities.pop();
	Engine.DestroyEntity(lastId);
};

Engine.RegisterSystemComponentType(IID_PlayerManager, "PlayerManager", PlayerManager);
