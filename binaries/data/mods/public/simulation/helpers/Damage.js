// Create global Damage object.
var Damage = {};

/**
 * Damages units around a given origin.
 * data.attacker = <entity id>
 * data.origin = <Vector2D>
 * data.radius = <int>
 * data.shape = <string>
 * data.strengths = {'hack':<float>, 'pierce':<float>, 'crush':<float>}
 * data.type = <string>
 * ***Optional Variables***
 * data.direction = <unit vector>
 * data.playersToDamage = <array of player ids>
 */
Damage.CauseSplashDamage = function(data)
{
	// Get nearby entities and define variables
	var nearEnts = Damage.EntitiesNearPoint(data.origin, data.radius, data.playersToDamage);
	var damageMultiplier = 1;
	// Cycle through all the nearby entities and damage it appropriately based on its distance from the origin.
	for each (var entity in nearEnts)
	{
		var entityPosition = Engine.QueryInterface(entity, IID_Position).GetPosition2D();
		if(data.shape == 'Circular') // circular effect with quadratic falloff in every direction
		{
			var squaredDistanceFromOrigin = data.origin.distanceToSquared(entityPosition);
			damageMultiplier = 1 - squaredDistanceFromOrigin / (data.radius * data.radius);
		}
		else if(data.shape == 'Linear') // linear effect with quadratic falloff in two directions (only used for certain missiles)
		{
			// Get position of entity relative to splash origin.
			var relativePos = entityPosition.sub(data.origin);

			// The width of linear splash is one fifth of the normal splash radius.
			var width = data.radius/5;

			// Effectivly rotate the axis to align with the missile direction.
			var parallelDist = relativePos.dot(data.direction); // z axis
			var perpDist = Math.abs(relativePos.cross(data.direction)); // y axis

			// Check that the unit is within the distance at which it will get damaged.
			if (parallelDist > -width && perpDist < width) // If in radius, quadratic falloff in both directions
				damageMultiplier = (data.radius * data.radius - parallelDist * parallelDist) / (data.radius * data.radius)
								 * (width * width - perpDist * perpDist) / (width * width);
			else
				damageMultiplier = 0;
		}
		else // In case someone calls this function with an invalid shape.
		{
			warn("The " + data.shape + " splash damage shape is not implemented!");
		}
		// Call CauseDamage which reduces the hitpoints, posts network command, plays sounds....
		Damage.CauseDamage({"strengths":data.strengths, "target":entity, "attacker":data.attacker, "multiplier":damageMultiplier, "type":data.type + ".Splash"})
	}
};

/**
 * Causes damage on a given unit
 * data.strengths = {'hack':<float>, 'pierce':<float>, 'crush':<float>}
 * data.target = <entity id>
 * data.attacker = <entity id>
 * data.multiplier = <float between 1 and 0>
 * data.type = <string>
 */
Damage.CauseDamage = function(data)
{
	// Check the target can be damaged otherwise don't do anything.
	var cmpDamageReceiver = Engine.QueryInterface(data.target, IID_DamageReceiver);
	var cmpHealth = Engine.QueryInterface(data.target, IID_Health);
	if (!cmpDamageReceiver || !cmpHealth)
		return;

	// Damage the target
	var targetState = cmpDamageReceiver.TakeDamage(data.strengths.hack * data.multiplier, data.strengths.pierce * data.multiplier, data.strengths.crush * data.multiplier, data.attacker);

	// If the target was killed run some cleanup
	if (targetState.killed)
		Damage.TargetKilled(data.attacker, data.target);

	// Post the network command (make it work in multiplayer)
	Engine.PostMessage(data.target, MT_Attacked, {"attacker":data.attacker, "target":data.target, "type":data.type, "damage":-targetState.change});

	// Play attacking sounds
	PlaySound("attack_impact", data.attacker);
};

/**
 * Gets entities near a give point for given players.
 * origin = <Vector2D>
 * radius = <int>
 * players = <array>
 * If players is not included, entities from all players are used.
 */
Damage.EntitiesNearPoint = function(origin, radius, players)
{
	// If there is insufficient data return an empty array.
	if (!origin || !radius)
		return [];

	// If the players parameter is not specified use all players.
	if (!players)
	{
		var playerEntities = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayerEntities();
		players = [];
		for each (var entity in playerEntities)
			players.push(Engine.QueryInterface(entity, IID_Player).GetPlayerID());
	}

	// Call RangeManager with dummy entity and return the result.
	var rangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var rangeQuery = rangeManager.ExecuteQueryAroundPos(origin, 0, radius, players, IID_DamageReceiver);
	return rangeQuery;
};

/**
 * Called when some units kills something (another unit, building, animal etc)
 * killerEntity = <entity id>
 * targetEntity = <entity id>
 */
Damage.TargetKilled = function(killerEntity, targetEntity)
{
	// Add to killer statistics.
	var cmpKillerPlayerStatisticsTracker = QueryOwnerInterface(killerEntity, IID_StatisticsTracker);
	if (cmpKillerPlayerStatisticsTracker)
		cmpKillerPlayerStatisticsTracker.KilledEntity(targetEntity);
	// Add to loser statistics.
	var cmpTargetPlayerStatisticsTracker = QueryOwnerInterface(targetEntity, IID_StatisticsTracker);
	if (cmpTargetPlayerStatisticsTracker)
		cmpTargetPlayerStatisticsTracker.LostEntity(targetEntity);

	// If killer can collect loot, let's try to collect it.
	var cmpLooter = Engine.QueryInterface(killerEntity, IID_Looter);
	if (cmpLooter)
		cmpLooter.Collect(targetEntity);
};

Engine.RegisterGlobal("Damage", Damage);

