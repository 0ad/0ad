/**
 * Provides attack and damage-related helpers.
 */
function AttackHelper() {}

const DirectEffectsSchema =
	"<element name='Damage'>" +
		"<oneOrMore>" +
			"<element a:help='One or more elements describing damage types'>" +
				"<anyName/>" +
				"<ref name='nonNegativeDecimal' />" +
			"</element>" +
		"</oneOrMore>" +
	"</element>" +
	"<element name='Capture' a:help='Capture points value'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

const StatusEffectsSchema =
	"<element name='ApplyStatus' a:help='Effects like poisoning or burning a unit.'>" +
		"<oneOrMore>" +
			"<element>" +
				"<anyName a:help='The name must have a matching JSON file in data/status_effects.'/>" +
				"<interleave>" +
					"<optional>" +
						"<element name='Duration' a:help='The duration of the status while the effect occurs.'><ref name='nonNegativeDecimal'/></element>" +
					"</optional>" +
					"<optional>" +
						"<interleave>" +
							"<element name='Interval' a:help='Interval between the occurrences of the effect.'><ref name='nonNegativeDecimal'/></element>" +
							"<oneOrMore>" +
								"<choice>" +
									DirectEffectsSchema +
								"</choice>" +
							"</oneOrMore>" +
						"</interleave>" +
					"</optional>" +
					"<optional>" +
						ModificationsSchema +
					"</optional>" +
					"<element name='Stackability' a:help='Defines how this status effect stacks, i.e. how subsequent status effects of the same kind are handled. Choices are: &#x201C;Ignore&#x201D;, which means a new one is ignored, &#x201C;Extend&#x201D;, which means the duration of a new one is added to the already active status effect, &#x201C;Replace&#x201D;, which means the currently active status effect is removed and the new one is put in place and &#x201C;Stack&#x201D;, which means that the status effect can be added multiple times.'>" +
						"<choice>" +
							"<value>Ignore</value>" +
							"<value>Extend</value>" +
							"<value>Replace</value>" +
							"<value>Stack</value>" +
						"</choice>" +
					"</element>" +
				"</interleave>" +
			"</element>" +
		"</oneOrMore>" +
	"</element>";

/**
 * Builds a RelaxRNG schema of possible attack effects.
 * See globalscripts/AttackEffects.js for possible elements.
 * Attacks may also have a "Bonuses" element.
 *
 * @return {string} - RelaxNG schema string.
 */
AttackHelper.prototype.BuildAttackEffectsSchema = function()
{
	return "" +
	"<oneOrMore>" +
		"<choice>" +
			DirectEffectsSchema +
			StatusEffectsSchema +
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
AttackHelper.prototype.GetAttackEffectsData = function(valueModifRoot, template, entity)
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

	if (template.ApplyStatus)
		ret.ApplyStatus = this.GetStatusEffectsData(valueModifRoot, template.ApplyStatus, entity);

	if (template.Bonuses)
		ret.Bonuses = template.Bonuses;

	return ret;
};

AttackHelper.prototype.GetStatusEffectsData = function(valueModifRoot, template, entity)
{
	let result = {};
	for (let effect in template)
	{
		let statusTemplate = template[effect];
		result[effect] = {
			"Duration": ApplyValueModificationsToEntity(valueModifRoot + "/ApplyStatus/" + effect + "/Duration", +(statusTemplate.Duration || 0), entity),
			"Interval": ApplyValueModificationsToEntity(valueModifRoot + "/ApplyStatus/" + effect + "/Interval", +(statusTemplate.Interval || 0), entity),
			"Stackability": statusTemplate.Stackability
		};
		Object.assign(result[effect], this.GetAttackEffectsData(valueModifRoot + "/ApplyStatus" + effect, statusTemplate, entity));
		if (statusTemplate.Modifiers)
			result[effect].Modifiers = this.GetStatusEffectsModifications(valueModifRoot, statusTemplate.Modifiers, entity, effect);
	}
	return result;
};

AttackHelper.prototype.GetStatusEffectsModifications = function(valueModifRoot, template, entity, effect)
{
	let modifiers = {};
	for (let modifier in template)
	{
		let modifierTemplate = template[modifier];
		modifiers[modifier] = {
			"Paths": modifierTemplate.Paths,
			"Affects": modifierTemplate.Affects
		};
		if (modifierTemplate.Add !== undefined)
			modifiers[modifier].Add = ApplyValueModificationsToEntity(valueModifRoot + "/ApplyStatus/" + effect + "/Modifiers/" + modifier + "/Add", +modifierTemplate.Add, entity);
		if (modifierTemplate.Multiply !== undefined)
			modifiers[modifier].Multiply = ApplyValueModificationsToEntity(valueModifRoot + "/ApplyStatus/" + effect + "/Modifiers/" + modifier + "/Multiply", +modifierTemplate.Multiply, entity);
		if (modifierTemplate.Replace !== undefined)
			modifiers[modifier].Replace = modifierTemplate.Replace;
	}
	return modifiers;
};

/**
 * Calculate the total effect taking bonus and resistance into account.
 *
 * @param {number} target - The target of the attack.
 * @param {Object} effectData - The effects calculate the effect for.
 * @param {string} effectType - The type of effect to apply (e.g. Damage, Capture or ApplyStatus).
 * @param {number} bonusMultiplier - The factor to multiply the total effect with.
 * @param {Object} cmpResistance - Optionally the resistance component of the target.
 *
 * @return {number} - The total value of the effect.
 */
AttackHelper.prototype.GetTotalAttackEffects = function(target, effectData, effectType, bonusMultiplier, cmpResistance)
{
	let total = 0;
	if (!cmpResistance)
		cmpResistance = Engine.QueryInterface(target, IID_Resistance);

	let resistanceStrengths = cmpResistance ? cmpResistance.GetEffectiveResistanceAgainst(effectType) : {};

	if (effectType == "Damage")
		for (let type in effectData.Damage)
			total += effectData.Damage[type] * Math.pow(0.9, resistanceStrengths.Damage ? resistanceStrengths.Damage[type] || 0 : 0);
	else if (effectType == "Capture")
	{
		total = effectData.Capture * Math.pow(0.9, resistanceStrengths.Capture || 0);

		// If Health is lower we are more susceptible to capture attacks.
		let cmpHealth = Engine.QueryInterface(target, IID_Health);
		if (cmpHealth)
			total /= 0.1 + 0.9 * cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints();
	}
	if (effectType != "ApplyStatus")
		return total * bonusMultiplier;

	if (!resistanceStrengths.ApplyStatus)
		return effectData[effectType];

	let result = {};
	for (let statusEffect in effectData[effectType])
	{
		if (!resistanceStrengths.ApplyStatus[statusEffect])
		{
			result[statusEffect] = effectData[effectType][statusEffect];
			continue;
		}

		if (randBool(resistanceStrengths.ApplyStatus[statusEffect].blockChance))
			continue;

		result[statusEffect] = effectData[effectType][statusEffect];

		if (effectData[effectType][statusEffect].Duration)
			result[statusEffect].Duration = effectData[effectType][statusEffect].Duration *
				resistanceStrengths.ApplyStatus[statusEffect].duration;
	}
	return result;

};

/**
 * Get the list of players affected by the damage.
 * @param {number}  attackerOwner - The player id of the attacker.
 * @param {boolean} friendlyFire - A flag indicating if allied entities are also damaged.
 * @return {number[]} The ids of players need to be damaged.
 */
AttackHelper.prototype.GetPlayersToDamage = function(attackerOwner, friendlyFire)
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
AttackHelper.prototype.CauseDamageOverArea = function(data)
{
	let nearEnts = PositionHelper.EntitiesNearPoint(data.origin, data.radius,
		this.GetPlayersToDamage(data.attackerOwner, data.friendlyFire));
	let damageMultiplier = 1;

	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);

	// Cycle through all the nearby entities and damage it appropriately based on its distance from the origin.
	for (let ent of nearEnts)
	{
		// Correct somewhat for the entity's obstruction radius.
		// TODO: linear falloff should arguably use something cleverer.
		let distance = cmpObstructionManager.DistanceToPoint(ent, data.origin.x, data.origin.y);

		if (data.shape == 'Circular') // circular effect with quadratic falloff in every direction
			damageMultiplier = 1 - distance * distance / (data.radius * data.radius);
		else if (data.shape == 'Linear') // linear effect with quadratic falloff in two directions (only used for certain missiles)
		{
			// The entity has a position here since it was returned by the range manager.
			let entityPosition = Engine.QueryInterface(ent, IID_Position).GetPosition2D();
			let relativePos = entityPosition.sub(data.origin).normalize().mult(distance);

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
		// The RangeManager can return units that are too far away (due to approximations there)
		// so the multiplier can end up below 0.
		damageMultiplier = Math.max(0, damageMultiplier);

		data.type += ".Splash";
		this.HandleAttackEffects(ent, data, damageMultiplier);
	}
};
/**
 * Handle an attack peformed on an entity.
 *
 * @param {number} target - The targetted entityID.
 * @param {Object} data - The data of the attack.
 * @param {string} data.type - The type of attack that was performed (e.g. "Melee" or "Capture").
 * @param {Object} data.effectData - The effects use.
 * @param {number} data.attacker - The entityID that attacked us.
 * @param {number} data.attackerOwner - The playerID that owned the attacker when the attack was performed.
 * @param {number} bonusMultiplier - The factor to multiply the total effect with, defaults to 1.
 *
 * @return {boolean} - Whether we handled the attack.
 */
AttackHelper.prototype.HandleAttackEffects = function(target, data, bonusMultiplier = 1)
{
	let cmpResistance = Engine.QueryInterface(target, IID_Resistance);
	if (cmpResistance && cmpResistance.IsInvulnerable())
		return false;

	bonusMultiplier *= !data.attackData.Bonuses ? 1 : this.GetAttackBonus(data.attacker, target, data.type, data.attackData.Bonuses);

	let targetState = {};
	for (let receiver of g_AttackEffects.Receivers())
	{
		if (!data.attackData[receiver.type])
			continue;

		let cmpReceiver = Engine.QueryInterface(target, global[receiver.IID]);
		if (!cmpReceiver)
			continue;

		Object.assign(targetState, cmpReceiver[receiver.method](this.GetTotalAttackEffects(target, data.attackData, receiver.type, bonusMultiplier, cmpResistance), data.attacker, data.attackerOwner));
	}

	if (!Object.keys(targetState).length)
		return false;

	Engine.PostMessage(target, MT_Attacked, {
		"type": data.type,
		"target": target,
		"attacker": data.attacker,
		"attackerOwner": data.attackerOwner,
		"damage": -(targetState.healthChange || 0),
		"capture": targetState.captureChange || 0,
		"statusEffects": targetState.inflictedStatuses || [],
		"fromStatusEffect": !!data.attackData.StatusEffect,
	});

	// We do not want an entity to get XP from active Status Effects.
	if (!!data.attackData.StatusEffect)
		return true;

	let cmpPromotion = Engine.QueryInterface(data.attacker, IID_Promotion);
	if (cmpPromotion && targetState.xp)
		cmpPromotion.IncreaseXp(targetState.xp);

	return true;
};

/**
 * Calculates the attack damage multiplier against a target.
 * @param {number} source - The source entity's id.
 * @param {number} target - The target entity's id.
 * @param {string} type - The type of attack.
 * @param {Object} template - The bonus' template.
 * @return {number} - The source entity's attack bonus against the specified target.
 */
AttackHelper.prototype.GetAttackBonus = function(source, target, type, template)
{
	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return 1;

	let attackBonus = 1;
	let targetClasses = cmpIdentity.GetClassesList();
	let targetCiv = cmpIdentity.GetCiv();

	// Multiply the bonuses for all matching classes.
	for (let key in template)
	{
		let bonus = template[key];
		if (bonus.Civ && bonus.Civ !== targetCiv)
			continue;
		if (!bonus.Classes || MatchesClassList(targetClasses, bonus.Classes))
			attackBonus *= ApplyValueModificationsToEntity("Attack/" + type + "/Bonuses/" + key + "/Multiplier", +bonus.Multiplier, source);
	}

	return attackBonus;
};

Engine.RegisterGlobal("AttackHelper", new AttackHelper());
Engine.RegisterGlobal("g_AttackEffects", new AttackEffects());
