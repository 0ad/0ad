function Attack() {}

var bonusesSchema = 
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
						"<element name='Multiplier' a:help='The attackers attack strength is multiplied by this'><ref name='nonNegativeDecimal'/></element>" +
					"</interleave>" +
				"</element>" +
			"</zeroOrMore>" +
		"</element>" +
	"</optional>";

Attack.prototype.Schema =
	"<a:help>Controls the attack abilities and strengths of the unit.</a:help>" +
	"<a:example>" +
		"<Melee>" +
			"<Hack>10.0</Hack>" +
			"<Pierce>0.0</Pierce>" +
			"<Crush>5.0</Crush>" +
			"<MaxRange>4.0</MaxRange>" +
			"<RepeatTime>1000</RepeatTime>" +
			"<Bonuses>" +
				"<Bonus1>" +
					"<Civ>pers</Civ>" +
					"<Classes>Infantry</Classes>" +
					"<Multiplier>1.5</Multiplier>" +
				"</Bonus1>" +
				"<BonusCavMelee>" +
					"<Classes>Cavalry Melee</Classes>" +
					"<Multiplier>1.5</Multiplier>" +
				"</BonusCavMelee>" +
			"</Bonuses>" +
		"</Melee>" +
		"<Ranged>" +
			"<Hack>0.0</Hack>" +
			"<Pierce>10.0</Pierce>" +
			"<Crush>0.0</Crush>" +
			"<MaxRange>44.0</MaxRange>" +
			"<MinRange>20.0</MinRange>" +
			"<PrepareTime>800</PrepareTime>" +
			"<RepeatTime>1600</RepeatTime>" +
			"<ProjectileSpeed>50.0</ProjectileSpeed>" +
			"<Bonuses>" +
				"<Bonus1>" +
					"<Classes>Cavalry</Classes>" +
					"<Multiplier>2</Multiplier>" +
				"</Bonus1>" +
			"</Bonuses>" +
		"</Ranged>" +
		"<Charge>" +
			"<Hack>10.0</Hack>" +
			"<Pierce>0.0</Pierce>" +
			"<Crush>50.0</Crush>" +
			"<MaxRange>24.0</MaxRange>" +
			"<MinRange>20.0</MinRange>" +
		"</Charge>" +
	"</a:example>" +
	"<optional>" +
		"<element name='Melee'>" +
			"<interleave>" +
				"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MaxRange' a:help='Maximum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" + // TODO: it shouldn't be stretched
					"<data type='positiveInteger'/>" +
				"</element>" +
				bonusesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Ranged'>" +
			"<interleave>" +
				"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MaxRange' a:help='Maximum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MinRange' a:help='Minimum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='PrepareTime' a:help='Time from the start of the attack command until the attack actually occurs (in milliseconds). This value relative to RepeatTime should closely match the \"event\" point in the actor&apos;s attack animation'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" +
					"<data type='positiveInteger'/>" +
				"</element>" +
				"<element name='ProjectileSpeed' a:help='Speed of projectiles (in metres per second). If unspecified, then it is a melee attack instead'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
				bonusesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Charge'>" +
			"<interleave>" +
				"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MaxRange'><ref name='nonNegativeDecimal'/></element>" + // TODO: how do these work?
				"<element name='MinRange'><ref name='nonNegativeDecimal'/></element>" +
				bonusesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>";

Attack.prototype.Init = function()
{
};

Attack.prototype.Serialize = null; // we have no dynamic state to save

/**
 * Return the type of the best attack.
 * TODO: this should probably depend on range, target, etc,
 * so we can automatically switch between ranged and melee
 */
Attack.prototype.GetBestAttack = function()
{
	if (this.template.Ranged)
		return "Ranged";
	else if (this.template.Melee)
		return "Melee";
	else if (this.template.Charge)
		return "Charge";
	else
		return undefined;
};

Attack.prototype.GetTimers = function(type)
{
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	
	var prepare = cmpTechMan.ApplyModifications("Attack/" + type + "/PrepareTime", +(this.template[type].PrepareTime||0), this.entity);
	var repeat = cmpTechMan.ApplyModifications("Attack/" + type + "/RepeatTime", +(this.template[type].RepeatTime||1000), this.entity);
	
	return { "prepare": prepare, "repeat": repeat, "recharge": repeat - prepare };
};

Attack.prototype.GetAttackStrengths = function(type)
{
	// Work out the attack values with technology effects
	var self = this;
	
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	var applyTechs = function(damageType)
	{
		// All causes caching problems so disable it for now.
		//var allComponent = cmpTechMan.ApplyModifications("Attack/" + type + "/All", (+self.template[type][damageType] || 0), self.entity) - self.template[type][damageType];
		return cmpTechMan.ApplyModifications("Attack/" + type + "/" + damageType, (+self.template[type][damageType] || 0), self.entity);
	};
	
	return {
		hack: applyTechs("Hack"),
		pierce: applyTechs("Pierce"),
		crush: applyTechs("Crush")
	};
};

Attack.prototype.GetRange = function(type)
{
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	
	var max = cmpTechMan.ApplyModifications("Attack/" + type + "/MaxRange", +this.template[type].MaxRange, this.entity);
	var min = cmpTechMan.ApplyModifications("Attack/" + type + "/MinRange", +(this.template[type].MinRange || 0), this.entity);
	
	return { "max": max, "min": min };
};

// Calculate the attack damage multiplier against a target
Attack.prototype.GetAttackBonus = function(type, target)
{
	var attackBonus = 1;
	if (this.template[type].Bonuses)
	{
		var cmpIdentity = Engine.QueryInterface(target, IID_Identity);
		if (!cmpIdentity)
			return 1;
		
		// Multiply the bonuses for all matching classes
		for (var key in this.template[type].Bonuses)
		{
			var bonus = this.template[type].Bonuses[key];
			
			var hasClasses = true;
			if (bonus.Classes){
				var classes = bonus.Classes.split(/\s+/);
				for (var key in classes)
					hasClasses = hasClasses && cmpIdentity.HasClass(classes[key]);
			}
			
			if (hasClasses && (!bonus.Civ || bonus.Civ === cmpIdentity.GetCiv()))
				attackBonus *= bonus.Multiplier;
		}
	}
	
	return attackBonus;
};

/**
 * Attack the target entity. This should only be called after a successful range check,
 * and should only be called after GetTimers().repeat msec has passed since the last
 * call to PerformAttack.
 */
Attack.prototype.PerformAttack = function(type, target)
{
	// If this is a ranged attack, then launch a projectile
	if (type == "Ranged")
	{
		// To implement (in)accuracy, for arrows and javelins, we want to do the following:
		//  * Compute an accuracy rating, based on the entity's characteristics and the distance to the target
		//  * Pick a random point 'close' to the target (based on the accuracy) which is the real target point
		//  * Pick a real target unit, based on their footprint's proximity to the real target point
		//  * If there is none, then harmlessly shoot to the real target point instead
		//  * If the real target unit moves after being targeted, the projectile will follow it and hit it anyway
		//
		// In the future this should be extended:
		//  * If the target unit moves too far, the projectile should 'detach' and not hit it, so that
		//    players can dodge projectiles. (Or it should pick a new target after detaching, so it can still
		//    hit somebody.)
		//  * Obstacles like trees could reduce the probability of the target being hit
		//  * Obstacles like walls should block projectiles entirely
		//  * There should be more control over the probabilities of hitting enemy units vs friendly units vs missing,
		//    for gameplay balance tweaks
		//  * Larger, slower projectiles (catapults etc) shouldn't pick targets first, they should just
		//    hurt anybody near their landing point

		// Get some data about the entity
		var horizSpeed = +this.template[type].ProjectileSpeed;
		var gravity = 9.81; // this affects the shape of the curve; assume it's constant for now
		var accuracy = 6; // TODO: get from entity template

		//horizSpeed /= 8; gravity /= 8; // slow it down for testing

		// Find the distance to the target
		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			return;
		var selfPosition = cmpPosition.GetPosition();
		var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
		if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
			return;
		var targetPosition = cmpTargetPosition.GetPosition();
		var horizDistance = Math.sqrt(Math.pow(targetPosition.x - selfPosition.x, 2) + Math.pow(targetPosition.z - selfPosition.z, 2));

		// Compute the real target point (based on accuracy)
		var angle = Math.random() * 2*Math.PI;
		var r = 1 - Math.sqrt(Math.random()); // triangular distribution [0,1] (cluster around the center)
		var offset = r * accuracy; // TODO: should be affected by range
		var offsetX = offset * Math.sin(angle);
		var offsetZ = offset * Math.cos(angle);

		var realTargetPosition = { "x": targetPosition.x + offsetX, "y": targetPosition.y, "z": targetPosition.z + offsetZ };

		// TODO: what we should really do here is select the unit whose footprint is closest to the realTargetPosition
		// (and harmlessly hit the ground if there's none), but as a simplification let's just randomly decide whether to
		// hit the original target or not.
		var realTargetUnit = undefined;
		if (Math.random() < 0.5) // TODO: this is yucky and hardcoded
		{
			// Hit the original target
			realTargetUnit = target;
			realTargetPosition = targetPosition;
		}
		else
		{
			// Hit the ground
			// TODO: ought to make sure Y is on the ground
		}

		// Hurt the target after the appropriate time
		if (realTargetUnit)
		{
			var realHorizDistance = Math.sqrt(Math.pow(realTargetPosition.x - selfPosition.x, 2) + Math.pow(realTargetPosition.z - selfPosition.z, 2));
			var timeToTarget = realHorizDistance / horizSpeed;
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.SetTimeout(this.entity, IID_Attack, "CauseDamage", timeToTarget*1000, {"type": type, "target": target});
		}

		// Launch the graphical projectile
		var cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
		if (realTargetUnit)
			cmpProjectileManager.LaunchProjectileAtEntity(this.entity, realTargetUnit, horizSpeed, gravity);
		else
			cmpProjectileManager.LaunchProjectileAtPoint(this.entity, realTargetPosition, horizSpeed, gravity);
	}
	else
	{
		// Melee attack - hurt the target immediately
		this.CauseDamage({"type": type, "target": target});
	}
	// TODO: charge attacks (need to design how they work)
};

/**
 * Called when some units kills something (another unit, building, animal etc)
 */
Attack.prototype.TargetKilled = function(killerEntity, targetEntity)
{
	var cmpKillerPlayerStatisticsTracker = QueryOwnerInterface(killerEntity, IID_StatisticsTracker);
	if (cmpKillerPlayerStatisticsTracker) cmpKillerPlayerStatisticsTracker.KilledEntity(targetEntity);
	var cmpTargetPlayerStatisticsTracker = QueryOwnerInterface(targetEntity, IID_StatisticsTracker);
	if (cmpTargetPlayerStatisticsTracker) cmpTargetPlayerStatisticsTracker.LostEntity(targetEntity);

	// if unit can collect loot, lets try to collect it
	var cmpLooter = Engine.QueryInterface(killerEntity, IID_Looter);
	if (cmpLooter)
	{
		cmpLooter.Collect(targetEntity);
	}
};

/**
 * Inflict damage on the target
 */
Attack.prototype.CauseDamage = function(data)
{
	var strengths = this.GetAttackStrengths(data.type);
	
	var attackBonus = this.GetAttackBonus(data.type, data.target);
	
	var cmpDamageReceiver = Engine.QueryInterface(data.target, IID_DamageReceiver);
	if (!cmpDamageReceiver)
		return;
	var targetState = cmpDamageReceiver.TakeDamage(strengths.hack * attackBonus, strengths.pierce * attackBonus, strengths.crush * attackBonus);
	// if target killed pick up loot and credit experience
	if (targetState.killed == true)
	{
		this.TargetKilled(this.entity, data.target);
	}

	Engine.PostMessage(data.target, MT_Attacked,
		{ "attacker": this.entity, "target": data.target, "type": data.type });

	PlaySound("attack_impact", this.entity);
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
