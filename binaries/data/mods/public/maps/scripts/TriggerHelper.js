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

TriggerHelper.GetOwner = function(ent)
{
	let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
		return cmpOwnership.GetOwner();

	return -1;
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
 * If it's not a last man standing game, then allies will win too.
 */
TriggerHelper.SetPlayerWon = function(playerID)
{
	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	cmpEndGameManager.MarkPlayerAsWon(playerID);
};

/**
 * Defeats a player
 */
TriggerHelper.DefeatPlayer = function(playerID)
{
	let cmpPlayer = QueryPlayerIDInterface(playerID);
	if (cmpPlayer)
		cmpPlayer.SetState("defeated");
};

/**
 * Returns the number of current players
 */
TriggerHelper.GetNumberOfPlayers = function()
{
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	return cmpPlayerManager.GetNumPlayers();
};

/**
 * A function to determine if an entity has a specific class
 * @param entity ID of the entity that we want to check for classes
 * @param classname The name of the class we are checking if the entity has
 */
TriggerHelper.EntityHasClass = function(entity, classname)
{
	let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	if (!cmpIdentity)
		return false;

	let classes = cmpIdentity.GetClassesList();
	return classes && classes.indexOf(classname) != -1;
};

Engine.RegisterGlobal("TriggerHelper", TriggerHelper);
