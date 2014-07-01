// Contains standardized functions suitable for using in trigger scripts.
// Do not use them in any other simulation script.

var TriggerHelper = {};

TriggerHelper.GetPlayerIDFromEntity = function(ent)
{
	var cmpPlayer = Engine.QueryInterface(ent, IID_Player);
	if (cmpPlayer)
		return cmpPlayer.GetPlayerID();
	return -1;
};

TriggerHelper.GetOwner = function(ent)
{
	var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
		return cmpOwnership.GetOwner();
	return -1;
};

/**
 * Can be used to "force" a building to spawn a group of entities.
 * Only works for buildings that can already train units.
 * @param source Entity id of the point where they will be spawned from
 * @param template Name of the template
 * @param count Number of units to spawn
 * @param owner Player id of the owner of the new units. By default, the owner
 * of the source entity.
 */
TriggerHelper.SpawnUnits = function(source, template, count, owner)
{
	var r = []; // array of entities to return;
	var cmpFootprint = Engine.QueryInterface(source, IID_Footprint);
	var cmpPosition = Engine.QueryInterface(source, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
	{
		error("tried to create entity from a source without position");
		return r;
	}
	if (owner == null)
		owner = TriggerHelper.GetOwner(source);

	for (var i = 0; i < count; i++)
	{
		var ent = Engine.AddEntity(template);
		var cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
		if (!cmpEntPosition)
		{
			error("tried to create entity without position");
			continue;
		}
		var cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
		if (cmpEntOwnership)
			cmpEntOwnership.SetOwner(owner);
		r.push(ent);
		var pos;
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
	return r;
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
	var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	var triggerPoints = cmpTrigger.GetTriggerPoints(ref);
	var r = {};
	for (var point of triggerPoints)
		r[point] = TriggerHelper.SpawnUnits(point, template, count, owner);
	return r;
};

/**
 * Returs a function that can be used to filter an array of entities by player
 */
TriggerHelper.GetPlayerFilter = function(playerID)
{
	return function(entity) {
		var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
		return cmpOwnership && cmpOwnership.GetOwner() == playerID;
	}
};

/**
 * Returns the resource type that can be gathered from an entity
 */
TriggerHelper.GetResourceType = function(entity)
{
	var cmpResourceSupply = Engine.QueryInterface(entity, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return undefined;
	return cmpResourceSupply.GetType();
}; 

/**
 * Wins the game for a player
 */
TriggerHelper.SetPlayerWon = function(playerID)
{
	var cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	cmpEndGameManager.MarkPlayerAsWon(playerID);
};

/**
 * Defeats a player
 */
TriggerHelper.DefeatPlayer = function(playerID)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var playerEnt = cmpPlayerMan.GetPlayerByID(playerID);
	Engine.PostMessage(playerEnt, MT_PlayerDefeated, { "playerId": playerID } );
};

/**
 * Returns the number of current players
 */
TriggerHelper.GetNumberOfPlayers = function()
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	return cmpPlayerManager.GetNumPlayers();
};

/**
 * Returns the player component. For more information on its functions, see simulation/components/Player.js
 */
TriggerHelper.GetPlayerComponent = function(playerID)
{
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	return Engine.QueryInterface(cmpPlayerMan.GetPlayerByID(playerID), IID_Player);
};

/**
 * A function to determine if an entity has a specific class
 * @param entity ID of the entity that we want to check for classes
 * @param classname The name of the class we are checking if the entity has
 */
TriggerHelper.EntityHasClass = function(entity, classname)
{
	var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	if (!cmpIdentity)
		return false;
	var classes = cmpIdentity.GetClassesList();;
	return (classes && classes.indexOf(classname) != -1);
};

Engine.RegisterGlobal("TriggerHelper", TriggerHelper);
