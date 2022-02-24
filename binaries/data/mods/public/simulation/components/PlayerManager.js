function PlayerManager() {}

PlayerManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

PlayerManager.prototype.Init = function()
{
	// List of player entity IDs.
	this.playerEntities = [];
};

/**
 * @param {string} templateName - The template name of the player to add.
 * @return {number} - The player's ID (player number).
 */
PlayerManager.prototype.AddPlayer = function(templateName)
{
	const ent = Engine.AddEntity(templateName);
	const id = this.playerEntities.length;
	const cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	cmpPlayer.SetPlayerID(id);
	this.playerEntities.push(ent);

	const newDiplo = [];
	for (let i = 0; i < id; i++)
	{
		Engine.QueryInterface(this.GetPlayerByID(i), IID_Player).diplomacy[id] = -1;
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
	const entities = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(id);
	for (const e of entities)
		Engine.QueryInterface(e, IID_Ownership)?.SetOwner(INVALID_PLAYER);

	const oldent = this.playerEntities[id];
	const oldCmpPlayer = Engine.QueryInterface(oldent, IID_Player);
	const newCmpPlayer = Engine.QueryInterface(ent, IID_Player);

	newCmpPlayer.SetPlayerID(id);
	this.playerEntities[id] = ent;

	newCmpPlayer.SetColor(oldCmpPlayer.GetColor());
	newCmpPlayer.SetDiplomacy(oldCmpPlayer.GetDiplomacy());

	Engine.BroadcastMessage(MT_PlayerEntityChanged, {
		"player": id,
		"from": oldent,
		"to": ent
	});

	for (const e of entities)
		Engine.QueryInterface(e, IID_Ownership)?.SetOwner(id);

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

	const stack = new Error().stack.trimRight().replace(/^/mg, '  '); // indent each line
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
	const players = [];
	for (let i = 0; i < this.playerEntities.length; ++i)
		players.push(i);
	return players;
};

/**
 * Returns IDs of all players excluding gaia.
 */
PlayerManager.prototype.GetNonGaiaPlayers = function()
{
	const players = [];
	for (let i = 1; i < this.playerEntities.length; ++i)
		players.push(i);
	return players;
};

/**
 * Returns IDs of all players excluding gaia that are not defeated nor have won.
 */
PlayerManager.prototype.GetActivePlayers = function()
{
	return this.GetNonGaiaPlayers().filter(playerID =>
		Engine.QueryInterface(this.GetPlayerByID(playerID), IID_Player).IsActive()
	);
};

/**
 * Note: This should only be called during setup/init and not during a match
 * since it doesn't change the owned entities.
 */
PlayerManager.prototype.RemoveLastPlayer = function()
{
	if (!this.playerEntities.length)
		return;

	const lastId = this.playerEntities.pop();
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
	const worldPopulation = this.GetMaxWorldPopulation();
	if (!worldPopulation)
		return;

	const activePlayers = this.GetActivePlayers();
	if (!activePlayers.length)
		return;

	const newMaxPopulation = worldPopulation / activePlayers.length;
	for (const playerID of activePlayers)
		Engine.QueryInterface(this.GetPlayerByID(playerID), IID_Player).SetMaxPopulation(newMaxPopulation);
};

PlayerManager.prototype.OnGlobalPlayerDefeated = function(msg)
{
	this.RedistributeWorldPopulation();
};

Engine.RegisterSystemComponentType(IID_PlayerManager, "PlayerManager", PlayerManager);
