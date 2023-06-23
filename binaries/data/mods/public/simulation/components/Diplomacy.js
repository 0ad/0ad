function Diplomacy() {}

Diplomacy.prototype.Schema =
	"<element name='SharedLosTech' a:help='Allies will share los when this technology is researched. Leave empty to never share LOS.'>" +
		"<text/>" +
	"</element>" +
	"<element name='SharedDropsitesTech' a:help='Allies will share dropsites when this technology is researched. Leave empty to never share dropsites.'>" +
		"<text/>" +
	"</element>";

Diplomacy.prototype.SerializableAttributes = [
	"team",
	"teamLocked",
	"diplomacy",
	"sharedDropsites",
];

Diplomacy.prototype.Serialize = function()
{
	const state = {};
	for (const key in this.SerializableAttributes)
		if (this.hasOwnProperty(key))
			state[key] = this[key];

	return state;
};

Diplomacy.prototype.Deserialize = function(state)
{
	for (const prop in state)
		this[prop] = state[prop];
};

Diplomacy.prototype.Init = function()
{
	// Team number of the player, players on the same team will always have ally diplomatic status. Also this is useful for team emblems, scoring, etc.
	this.team = -1;

	// Array of diplomatic stances for this player with respect to other players (including gaia and self).
	this.diplomacy = [];
};

/**
 * @param {Object} color - r, g, b values of the diplomacy colour.
 */
Diplomacy.prototype.SetDiplomacyColor = function(color)
{
	this.diplomacyColor = { "r": color.r / 255, "g": color.g / 255, "b": color.b / 255, "a": 1 };
};

/**
 * @return {Object} -
 */
Diplomacy.prototype.GetColor = function()
{
	return this.diplomacyColor;
};

/**
 * @return {number} -
 */
Diplomacy.prototype.GetTeam = function()
{
	return this.team;
};

/**
 * @param {number} team - The new team number, -1 for no team.
 */
Diplomacy.prototype.ChangeTeam = function(team)
{
	if (this.teamLocked || this.team === team)
		return;

	const playerID = Engine.QueryInterface(this.entity, IID_Player)?.GetPlayerID();
	if (!playerID)
		return;

	// ToDo: Fix this.
	if (this.team !== -1)
		warn("A change in teams is requested while the player already had a team, previous alliances are maintained.");

	this.team = team;

	if (this.team !== -1)
	{
		const numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		for (let i = 0; i < numPlayers; ++i)
		{
			const cmpDiplomacy = QueryPlayerIDInterface(i, IID_Diplomacy);
			if (this.team !== cmpDiplomacy.GetTeam())
				continue;

			this.Ally(i);
			cmpDiplomacy.Ally(playerID);
		}
	}

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": playerID,
		"otherPlayer": null
	});
};

Diplomacy.prototype.LockTeam = function()
{
	this.teamLocked = true;
};

Diplomacy.prototype.UnLockTeam = function()
{
	delete this.teamLocked;
};

/**
 * @return {boolean} -
 */
Diplomacy.prototype.IsTeamLocked = function()
{
	return !!this.teamLocked;
};

/**
 * @return {number[]} - Current diplomatic stances.
 */
Diplomacy.prototype.GetDiplomacy = function()
{
	return this.diplomacy.slice();
};

/**
 * @param {number[]} dipl - The diplomacy array to set.
 */
Diplomacy.prototype.SetDiplomacy = function(dipl)
{
	const playerID = Engine.QueryInterface(this.entity, IID_Player)?.GetPlayerID();
	if (playerID === undefined)
		return

	this.diplomacy = dipl.slice();

	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": playerID,
		"otherPlayer": null
	});
};

/**
 * Helper function for allying etc.
 * @param {number} idx - The player number.
 * @param {number} value - The diplomacy value.
 */
Diplomacy.prototype.SetDiplomacyIndex = function(idx, value)
{
	if (!QueryPlayerIDInterface(idx)?.IsActive())
		return;

	const cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer?.IsActive())
		return;

	this.diplomacy[idx] = value;

	const playerID = cmpPlayer.GetPlayerID();
	Engine.BroadcastMessage(MT_DiplomacyChanged, {
		"player": playerID,
		"otherPlayer": idx,
		"value": value
	});
};

/**
 * Helper function for getting allies etc.
 * @param {string} func - Name of the function to test.
 * @return {number[]} - Player IDs matching the function.
 */
Diplomacy.prototype.GetPlayersByDiplomacy = function(func)
{
	const players = [];
	for (let i = 0; i < this.diplomacy.length; ++i)
		if (this[func](i))
			players.push(i);
	return players;
};

/**
 * @param {number} - id
 */
Diplomacy.prototype.Ally = function(id)
{
	this.SetDiplomacyIndex(id, 1);
};

/**
 * Check if given player is our ally.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsAlly = function(id)
{
	return this.diplomacy[id] > 0;
};

/**
 * @return {number[]} -
 */
Diplomacy.prototype.GetAllies = function()
{
	return this.GetPlayersByDiplomacy("IsAlly");
};

/**
 * Check if given player is our ally excluding ourself.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsExclusiveAlly = function(id)
{
	return Engine.QueryInterface(this.entity, IID_Player)?.GetPlayerID() !== id && this.IsAlly(id);
};

/**
 * Check if given player is our ally, and we are its ally.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsMutualAlly = function(id)
{
	const playerID = Engine.QueryInterface(this.entity, IID_Player)?.GetPlayerID();
	return playerID !== undefined && this.IsAlly(id) && QueryPlayerIDInterface(id, IID_Diplomacy)?.IsAlly(playerID);
};

/**
 * @return {number[]} -
 */
Diplomacy.prototype.GetMutualAllies = function()
{
	return this.GetPlayersByDiplomacy("IsMutualAlly");
};

/**
 * Check if given player is our ally, and we are its ally, excluding ourself.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsExclusiveMutualAlly = function(id)
{
	const playerID = Engine.QueryInterface(this.entity, IID_Player)?.GetPlayerID();
	return playerID !== id && this.IsMutualAlly(id);
};

/**
 * @param {number} id -
 */
Diplomacy.prototype.SetEnemy = function(id)
{
	this.SetDiplomacyIndex(id, -1);
};

/**
 * Check if given player is our enemy.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsEnemy = function(id)
{
	return this.diplomacy[id] < 0;
};

/**
 * @return {number[]} -
 */
Diplomacy.prototype.GetEnemies = function()
{
	return this.GetPlayersByDiplomacy("IsEnemy");
};

/**
 * @param {number} id -
 */
Diplomacy.prototype.SetNeutral = function(id)
{
	this.SetDiplomacyIndex(id, 0);
};

/**
 * Check if given player is neutral.
 * @param {number} id -
 * @return {boolean} -
 */
Diplomacy.prototype.IsNeutral = function(id)
{
	return this.diplomacy[id] === 0;
};

/**
 * @return {boolean} -
 */
Diplomacy.prototype.HasSharedDropsites = function()
{
	return this.sharedDropsites;
};

/**
 * @return {boolean} -
 */
Diplomacy.prototype.HasSharedLos = function()
{
	const cmpTechnologyManager = Engine.QueryInterface(this.entity, IID_TechnologyManager);
	return cmpTechnologyManager && cmpTechnologyManager.IsTechnologyResearched(this.template.SharedLosTech);
};

Diplomacy.prototype.UpdateSharedLos = function()
{
	const playerID = Engine.QueryInterface(this.entity, IID_Player).GetPlayerID();
	if (playerID === undefined)
		return;

	Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager)?.
		SetSharedLos(playerID, this.HasSharedLos() ? this.GetMutualAllies() : [playerID]);
};

Diplomacy.prototype.OnResearchFinished = function(msg)
{
	if (msg.tech === this.template.SharedLosTech)
		this.UpdateSharedLos();
	else if (msg.tech === this.template.SharedDropsitesTech)
		this.sharedDropsites = true;
};

Diplomacy.prototype.OnDiplomacyChanged = function(msg)
{
	this.UpdateSharedLos();

	if (msg.otherPlayer === null)
		return;

	const cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer || cmpPlayer.GetPlayerID() != msg.otherPlayer)
		return;

	// Mutual worsening of relations.
	if (this.diplomacy[msg.player] > msg.value)
		this.SetDiplomacyIndex(msg.player, msg.value);
};

Engine.RegisterComponentType(IID_Diplomacy, "Diplomacy", Diplomacy);
