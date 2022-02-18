function PlayerManager() {}

PlayerManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

PlayerManager.prototype.Init = function()
{
	// List of player entity IDs.
	this.playerEntities = [];

	// Maximum world population (if applicable will be distributed amongst living players).
	this.maxWorldPopulation = undefined;
};

/**
 * @param {string} templateName - The template name of the player to add.
 * @return {number} - The player's ID (player number).
 */
PlayerManager.prototype.AddPlayer = function(templateName)
{
	const ent = Engine.AddEntity(templateName);
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

	Engine.BroadcastMessage(MT_PlayerEntityChanged, {
		"player": id,
		"from": INVALID_ENTITY,
		"to": ent
	});

	return id;
};

/**
 * To avoid possible problems,
 * we first remove all entities from this player, and add them back after the replacement.
 * Note: This should only be called during setup/init and not during the game
 * @param {number} id - The player number to replace.
 * @param {string} newTemplateName - The new template name for the player.
 */
PlayerManager.prototype.ReplacePlayerTemplate = function(id, newTemplateName)
{
	const ent = Engine.AddEntity(newTemplateName);
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let entities = cmpRangeManager.GetEntitiesByPlayer(id);
	for (let e of entities)
	{
		let cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership)
			cmpOwnership.SetOwner(INVALID_PLAYER);
	}

	let oldent = this.playerEntities[id];
	let oldCmpPlayer = Engine.QueryInterface(oldent, IID_Player);
	let diplo = oldCmpPlayer.GetDiplomacy();
	let color = oldCmpPlayer.GetColor();

	let newCmpPlayer = Engine.QueryInterface(ent, IID_Player);
	newCmpPlayer.SetPlayerID(id);
	this.playerEntities[id] = ent;
	newCmpPlayer.SetColor(color);
	newCmpPlayer.SetDiplomacy(diplo);

	Engine.BroadcastMessage(MT_PlayerEntityChanged, {
		"player": id,
		"from": oldent,
		"to": ent
	});

	for (let e of entities)
	{
		let cmpOwnership = Engine.QueryInterface(e, IID_Ownership);
		if (cmpOwnership)
			cmpOwnership.SetOwner(id);
	}

	Engine.DestroyEntity(oldent);
	Engine.FlushDestroyedEntities();
};

/**
 * Returns the player entity ID for the given player ID.
 * The player ID must be valid (else there will be an error message).
 */
PlayerManager.prototype.GetPlayerByID = function(id)
{
	if (id in this.playerEntities)
		return this.playerEntities[id];

	// Observers don't have player data.
	if (id == INVALID_PLAYER)
		return INVALID_ENTITY;

	let stack = new Error().stack.trimRight().replace(/^/mg, '  '); // indent each line
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
	for (let player in this.playerEntities)
	{
		Engine.BroadcastMessage(MT_PlayerEntityChanged, {
			"player": player,
			"from": this.playerEntities[player],
			"to": INVALID_ENTITY
		});
		Engine.DestroyEntity(this.playerEntities[player]);
	}

	this.playerEntities = [];
};

PlayerManager.prototype.RemoveLastPlayer = function()
{
	if (this.playerEntities.length == 0)
		return;

	var lastId = this.playerEntities.pop();
	Engine.BroadcastMessage(MT_PlayerEntityChanged, {
		"player": this.playerEntities.length + 1,
		"from": lastId,
		"to": INVALID_ENTITY
	});
	Engine.DestroyEntity(lastId);
};

PlayerManager.prototype.SetMaxWorldPopulation = function(max)
{
	this.maxWorldPopulation = max;
	this.RedistributeWorldPopulation();
};

PlayerManager.prototype.GetMaxWorldPopulation = function()
{
	return this.maxWorldPopulation;
};

PlayerManager.prototype.RedistributeWorldPopulation = function()
{
	let worldPopulation = this.GetMaxWorldPopulation();
	if (!worldPopulation)
		return;

	let activePlayers = this.GetActivePlayers();
	if (!activePlayers.length)
		return;

	let newMaxPopulation = worldPopulation / activePlayers.length;
	for (let playerID of activePlayers)
		QueryPlayerIDInterface(playerID).SetMaxPopulation(newMaxPopulation);
};

PlayerManager.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.RedistributeWorldPopulation();
};

Engine.RegisterSystemComponentType(IID_PlayerManager, "PlayerManager", PlayerManager);
