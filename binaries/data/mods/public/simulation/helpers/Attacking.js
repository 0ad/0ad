/**
 * Provides attack and damage-related helpers under the Attacking umbrella (to avoid name ambiguity with the component).
 */
function Attacking() {}

/**
 * Builds a RelaxRNG schema of possible attack effects.
 * See globalscripts/AttackEffects.js for possible elements.
 * Attacks may also have a "Bonuses" element.
 *
 * @return {string}	- RelaxNG schema string
 */
const DamageSchema = "" +
	"<oneOrMore>" +
		"<element a:help='One or more elements describing damage types'>" +
			"<anyName>" +
				// Armour requires Foundation to not be a damage type.
				"<except><name>Foundation</name></except>" +
			"</anyName>" +
			"<ref name='nonNegativeDecimal' />" +
		"</element>" +
	"</oneOrMore>";

Attacking.prototype.BuildAttackEffectsSchema = function()
{
	return "" +
	"<oneOrMore>" +
		"<choice>" +
			"<element name='Damage'>" +
				DamageSchema +
			"</element>" +
			"<element name='Capture' a:help='Capture points value'>" +
				"<ref name='nonNegativeDecimal'/>" +
			"</element>" +
			"<element name='GiveStatus' a:help='Effects like poisoning or burning a unit.'>" +
				"<oneOrMore>" +
					"<element>" +
						"<anyName/>" +
						"<interleave>" +
								"<optional>" +
									"<element name='Icon' a:help='Icon for the status effect'><text/></element>" +
								"</optional>" +
								"<element name='Duration' a:help='The duration of the status while the effect occurs.'><ref name='nonNegativeDecimal'/></element>" +
								"<element name='Interval' a:help='Interval between the occurances of the effect.'><ref name='nonNegativeDecimal'/></element>" +
								"<element name='Damage' a:help='Damage caused by the effect.'>" + DamageSchema + "</element>" +
						"</interleave>" +
					"</element>" +
				"</oneOrMore>" +
			"</element>" +
		"</choice>" +
	"</oneOrMore>" +
	"<optional>" +
		"<element name='Bonuses'>" +
			"<zeroOrMore>" +
				"<element>" +
					"<anyName/>" +
					"<interleave>" +
						"<optional>" +
							"<element name='Civ' a:help='If an entity has this civ then the bonus is applied'><text/></element>" +
						"</optional>" +
						"<element name='Classes' a:help='If an entity has all these classes then the bonus is applied'><text/></element>" +
						"<element name='Multiplier' a:help='The effect strength is multiplied by this'><ref name='nonNegativeDecimal'/></element>" +
					"</interleave>" +
				"</element>" +
			"</zeroOrMore>" +
		"</element>" +
	"</optional>";
};

/**
 * Returns a template-like object of attack effects.
 */
Attacking.prototype.GetAttackEffectsData = function(valueModifRoot, template, entity)
{
	let ret = {};

	if (template.Damage)
	{
		ret.Damage = {};
		let applyMods = damageType =>
			ApplyValueModificationsToEntity(valueModifRoot + "/Damage/" + damageType, +(template.Damage[damageType] || 0), entity);
		for (let damageType in template.Damage)
			ret.Damage[damageType] = applyMods(damageType);
	}
	if (template.Capture)
		ret.Capture = ApplyValueModificationsToEntity(valueModifRoot + "/Capture", +(template.Capture || 0), entity);

	if (template.GiveStatus)
		ret.GiveStatus = template.GiveStatus;

	if (template.Bonuses)
		ret.Bonuses = template.Bonuses;

	return ret;
};

Attacking.prototype.GetTotalAttackEffects = function(effectData, effectType, cmpResistance)
{
	let total = 0;
	let armourStrengths = cmpResistance ? cmpResistance.GetArmourStrengths(effectType) : {};

	for (let type in effectData)
		total += effectData[type] * Math.pow(0.9, armourStrengths[type] || 0);

	return total;
};

/**
 * Gives the position of the given entity, taking the lateness into account.
 * @param {number} ent - Entity id of the entity we are finding the location for.
 * @param {number} lateness - The time passed since the expected time to fire the function.
 * @return {Vector3D} The location of the entity.
 */
Attacking.prototype.InterpolatedLocation = function(ent, lateness)
{
	let cmpTargetPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld()) // TODO: handle dead target properly
		return undefined;
	let curPos = cmpTargetPosition.GetPosition();
	let prevPos = cmpTargetPosition.GetPreviousPosition();
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let turnLength = cmpTimer.GetLatestTurnLength();
	return new Vector3D(
	    (curPos.x * (turnLength - lateness) + prevPos.x * lateness) / turnLength,
	    0,
	    (curPos.z * (turnLength - lateness) + prevPos.z * lateness) / turnLength
	);
};

/**
 * Test if a point is inside of an entity's footprint.
 * @param {number}   ent - Id of the entity we are checking with.
 * @param {Vector3D} point - The point we are checking with.
 * @param {number}   lateness - The time passed since the expected time to fire the function.
 * @return {boolean} True if the point is inside of the entity's footprint.
 */
Attacking.prototype.TestCollision = function(ent, point, lateness)
{
	let targetPosition = this.InterpolatedLocation(ent, lateness);
	if (!targetPosition)
		return false;

	let cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
	if (!cmpFootprint)
		return false;

	let targetShape = cmpFootprint.GetShape();

	if (!targetShape)
		return false;

	if (targetShape.type == "circle")
		return targetPosition.horizDistanceToSquared(point) < targetShape.radius * targetShape.radius;

	if (targetShape.type == "square")
	{
		let angle = Engine.QueryInterface(ent, IID_Position).GetRotation().y;
		let distance = Vector2D.from3D(Vector3D.sub(point, targetPosition)).rotate(-angle);
		return Math.abs(distance.x) < targetShape.width / 2 && Math.abs(distance.y) < targetShape.depth / 2;
	}

	warn("TestCollision called with an invalid footprint shape");
	return false;
};

/**
 * Get the list of players affected by the damage.
 * @param {number}  attackerOwner - The player id of the attacker.
 * @param {boolean} friendlyFire - A flag indicating if allied entities are also damaged.
 * @return {number[]} The ids of players need to be damaged.
 */
Attacking.prototype.GetPlayersToDamage = function(attackerOwner, friendlyFire)
{
	if (!friendlyFire)
		return QueryPlayerIDInterface(attackerOwner).GetEnemies();

	return Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayers();
};

/**
 * Damages units around a given origin.
 * @param {Object}   data - The data sent by the caller.
 * @param {string}   data.type - The type of damage.
 * @param {Object}   data.attackData - The attack data.
 * @param {number}   data.attacker - The entity id of the attacker.
 * @param {number}   data.attackerOwner - The player id of the attacker.
 * @param {Vector2D} data.origin - The origin of the projectile hit.
 * @param {number}   data.radius - The radius of the splash damage.
 * @param {string}   data.shape - The shape of the radius.
 * @param {Vector3D} [data.direction] - The unit vector defining the direction. Needed for linear splash damage.
 * @param {boolean}  data.friendlyFire - A flag indicating if allied entities also ought to be damaged.
 */
Attacking.prototype.CauseDamageOverArea = function(data)
{
	let nearEnts = this.EntitiesNearPoint(data.origin, data.radius,
		this.GetPlayersToDamage(data.attackerOwner, data.friendlyFire));
	let damageMultiplier = 1;

	// Cycle through all the nearby entities and damage it appropriately based on its distance from the origin.
	for (let ent of nearEnts)
	{
		let entityPosition = Engine.QueryInterface(ent, IID_Position).GetPosition2D();
		if (data.shape == 'Circular') // circular effect with quadratic falloff in every direction
			damageMultiplier = 1 - data.origin.distanceToSquared(entityPosition) / (data.radius * data.radius);
		else if (data.shape == 'Linear') // linear effect with quadratic falloff in two directions (only used for certain missiles)
		{
			// Get position of entity relative to splash origin.
			let relativePos = entityPosition.sub(data.origin);

			// Get the position relative to the missile direction.
			let direction = Vector2D.from3D(data.direction);
			let parallelPos = relativePos.dot(direction);
			let perpPos = relativePos.cross(direction);

			// The width of linear splash is one fifth of the normal splash radius.
			let width = data.radius / 5;

			// Check that the unit is within the distance splash width of the line starting at the missile's
			// landing point which extends in the direction of the missile for length splash radius.
			if (parallelPos >= 0 && Math.abs(perpPos) < width) // If in radius, quadratic falloff in both directions
				damageMultiplier = (1 - parallelPos * parallelPos / (data.radius * data.radius)) *
					(1 - perpPos * perpPos / (width * width));
			else
				damageMultiplier = 0;
		}
		else // In case someone calls this function with an invalid shape.
		{
			warn("The " + data.shape + " splash damage shape is not implemented!");
		}

		this.HandleAttackEffects(data.type + ".Splash", data.attackData, ent, data.attacker, data.attackerOwner, damageMultiplier);
	}
};

Attacking.prototype.HandleAttackEffects = function(attackType, attackData, target, attacker, attackerOwner, bonusMultiplier = 1)
{
	let targetState = {};
	for (let effectType of g_EffectTypes)
	{
		if (!attackData[effectType])
			continue;

		bonusMultiplier *= !attackData.Bonuses ? 1 : GetAttackBonus(attacker, target, attackType, attackData.Bonuses);

		let receiver = g_EffectReceiver[effectType];
		let cmpReceiver = Engine.QueryInterface(target, global[receiver.IID]);
		if (!cmpReceiver)
			continue;

		Object.assign(targetState, cmpReceiver[receiver.method](attackData[effectType], attacker, attackerOwner, bonusMultiplier));
	}

	let cmpPromotion = Engine.QueryInterface(attacker, IID_Promotion);
	if (cmpPromotion && targetState.xp)
		cmpPromotion.IncreaseXp(targetState.xp);

	if (targetState.killed)
		this.TargetKilled(attacker, target, attackerOwner);

	Engine.PostMessage(target, MT_Attacked, {
		"type": attackType,
		"target": target,
		"attacker": attacker,
		"attackerOwner": attackerOwner,
		"damage": -(targetState.HPchange || 0),
		"capture": targetState.captureChange || 0,
		"statusEffects": targetState.inflictedStatuses || [],
	});
};

/**
 * Gets entities near a give point for given players.
 * @param {Vector2D} origin - The point to check around.
 * @param {number}   radius - The radius around the point to check.
 * @param {number[]} players - The players of which we need to check entities.
 * @return {number[]} The id's of the entities in range of the given point.
 */
Attacking.prototype.EntitiesNearPoint = function(origin, radius, players)
{
	// If there is insufficient data return an empty array.
	if (!origin || !radius || !players)
		return [];

	return Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).ExecuteQueryAroundPos(origin, 0, radius, players, IID_Resistance);
};

/**
 * Called when a unit kills something (another unit, building, animal etc).
 * @param {number} attacker - The entity id of the killer.
 * @param {number} target - The entity id of the target.
 * @param {number} attackerOwner - The player id of the attacker.
 */
Attacking.prototype.TargetKilled = function(attacker, target, attackerOwner)
{
	let cmpAttackerOwnership = Engine.QueryInterface(attacker, IID_Ownership);
	let atkOwner = cmpAttackerOwnership && cmpAttackerOwnership.GetOwner() != INVALID_PLAYER ? cmpAttackerOwnership.GetOwner() : attackerOwner;

	// Add to killer statistics.
	let cmpKillerPlayerStatisticsTracker = QueryPlayerIDInterface(atkOwner, IID_StatisticsTracker);
	if (cmpKillerPlayerStatisticsTracker)
		cmpKillerPlayerStatisticsTracker.KilledEntity(target);
	// Add to loser statistics.
	let cmpTargetPlayerStatisticsTracker = QueryOwnerInterface(target, IID_StatisticsTracker);
	if (cmpTargetPlayerStatisticsTracker)
		cmpTargetPlayerStatisticsTracker.LostEntity(target);

	// If killer can collect loot, let's try to collect it.
	let cmpLooter = Engine.QueryInterface(attacker, IID_Looter);
	if (cmpLooter)
		cmpLooter.Collect(target);
};

var AttackingInstance = new Attacking();
Engine.RegisterGlobal("Attacking", AttackingInstance);
