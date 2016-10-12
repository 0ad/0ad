function Attack() {}

Attack.prototype.bonusesSchema =
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

Attack.prototype.preferredClassesSchema =
	"<optional>" +
		"<element name='PreferredClasses' a:help='Space delimited list of classes preferred for attacking. If an entity has any of theses classes, it is preferred. The classes are in decending order of preference'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>";

Attack.prototype.restrictedClassesSchema =
	"<optional>" +
		"<element name='RestrictedClasses' a:help='Space delimited list of classes that cannot be attacked by this entity. If target entity has any of these classes, it cannot be attacked'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
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
			"<RestrictedClasses datatype=\"tokens\">Champion</RestrictedClasses>" +
			"<PreferredClasses datatype=\"tokens\">Cavalry Infantry</PreferredClasses>" +
		"</Melee>" +
		"<Ranged>" +
			"<Hack>0.0</Hack>" +
			"<Pierce>10.0</Pierce>" +
			"<Crush>0.0</Crush>" +
			"<MaxRange>44.0</MaxRange>" +
			"<MinRange>20.0</MinRange>" +
			"<ElevationBonus>15.0</ElevationBonus>" +
			"<PrepareTime>800</PrepareTime>" +
			"<RepeatTime>1600</RepeatTime>" +
			"<ProjectileSpeed>50.0</ProjectileSpeed>" +
			"<Spread>2.5</Spread>" +
			"<Bonuses>" +
				"<Bonus1>" +
					"<Classes>Cavalry</Classes>" +
					"<Multiplier>2</Multiplier>" +
				"</Bonus1>" +
			"</Bonuses>" +
			"<RestrictedClasses datatype=\"tokens\">Champion</RestrictedClasses>" +
			"<Splash>" +
				"<Shape>Circular</Shape>" +
				"<Range>20</Range>" +
				"<FriendlyFire>false</FriendlyFire>" +
				"<Hack>0.0</Hack>" +
				"<Pierce>10.0</Pierce>" +
				"<Crush>0.0</Crush>" +
			"</Splash>" +
		"</Ranged>" +
		"<Slaughter>" +
			"<Hack>1000.0</Hack>" +
			"<Pierce>0.0</Pierce>" +
			"<Crush>0.0</Crush>" +
			"<MaxRange>4.0</MaxRange>" +
		"</Slaughter>" +
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
				Attack.prototype.bonusesSchema +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
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
				"<optional>"+
					"<element name='ElevationBonus' a:help='give an elevation advantage (in meters)'><ref name='nonNegativeDecimal'/></element>" +
				"</optional>" +
				"<element name='PrepareTime' a:help='Time from the start of the attack command until the attack actually occurs (in milliseconds). This value relative to RepeatTime should closely match the \"event\" point in the actor&apos;s attack animation'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" +
					"<data type='positiveInteger'/>" +
				"</element>" +
				"<element name='ProjectileSpeed' a:help='Speed of projectiles (in metres per second). If unspecified, then it is a melee attack instead'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
				"<element name='Spread' a:help='Radius over which missiles will tend to land (when shooting to the maximum range). Roughly 2/3 will land inside this radius (in metres). Spread is linearly diminished as the target gets closer.'><ref name='nonNegativeDecimal'/></element>" +
				Attack.prototype.bonusesSchema +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
				"<optional>" +
					"<element name='Splash'>" +
						"<interleave>" +
							"<element name='Shape' a:help='Shape of the splash damage, can be circular or linear'><text/></element>" +
							"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
							"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
							Attack.prototype.bonusesSchema +
						"</interleave>" +
					"</element>" +
				"</optional>" +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Capture'>" +
			"<interleave>" +
				"<element name='Value' a:help='Capture points value'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MaxRange' a:help='Maximum attack range (in meters)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" + // TODO: it shouldn't be stretched
					"<data type='positiveInteger'/>" +
				"</element>" +
				Attack.prototype.bonusesSchema +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Slaughter' a:help='A special attack to kill domestic animals'>" +
			"<interleave>" +
				"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MaxRange'><ref name='nonNegativeDecimal'/></element>" + // TODO: how do these work?
				Attack.prototype.bonusesSchema +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>";

Attack.prototype.Init = function()
{
};

Attack.prototype.Serialize = null; // we have no dynamic state to save

Attack.prototype.GetAttackTypes = function()
{
	return ["Melee", "Ranged", "Capture"].filter(type => !!this.template[type]);
};

Attack.prototype.GetPreferredClasses = function(type)
{
	if (this.template[type] && this.template[type].PreferredClasses &&
	    this.template[type].PreferredClasses._string)
		return this.template[type].PreferredClasses._string.split(/\s+/);

	return [];
};

Attack.prototype.GetRestrictedClasses = function(type)
{
	if (this.template[type] && this.template[type].RestrictedClasses &&
	    this.template[type].RestrictedClasses._string)
		return this.template[type].RestrictedClasses._string.split(/\s+/);

	return [];
};

Attack.prototype.CanAttack = function(target)
{
	let cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		return true;

	let cmpThisPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpThisPosition || !cmpTargetPosition || !cmpThisPosition.IsInWorld() || !cmpTargetPosition.IsInWorld())
		return false;

	// Check if the relative height difference is larger than the attack range
	// If the relative height is bigger, it means they will never be able to
	// reach each other, no matter how close they come.
	let heightDiff = Math.abs(cmpThisPosition.GetHeightOffset() - cmpTargetPosition.GetHeightOffset());

	const cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	const targetClasses = cmpIdentity.GetClassesList();

	for (let type of this.GetAttackTypes())
	{
		if (type == "Capture" && !QueryMiragedInterface(target, IID_Capturable))
			continue;

		if (heightDiff > this.GetRange(type).max)
			continue;

		let restrictedClasses = this.GetRestrictedClasses(type);
		if (!restrictedClasses.length)
			return true;

		if (!MatchesClassList(targetClasses, restrictedClasses))
			return true;
	}

	return false;
};

/**
 * Returns null if we have no preference or the lowest index of a preferred class.
 */
Attack.prototype.GetPreference = function(target)
{
	const cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	const targetClasses = cmpIdentity.GetClassesList();

	let minPref = null;
	for (let type of this.GetAttackTypes())
	{
		let preferredClasses = this.GetPreferredClasses(type);
		for (let targetClass of targetClasses)
		{
			let pref = preferredClasses.indexOf(targetClass);
			if (pref === 0)
				return pref;
			if (pref != -1 && (minPref === null || minPref > pref))
				minPref = pref;
		}
	}
	return minPref;
};

/**
 * Get the full range of attack using all available attack types.
 */
Attack.prototype.GetFullAttackRange = function()
{
	let ret = { "min": Infinity, "max": 0 };
	for (let type of this.GetAttackTypes())
	{
		let range = this.GetRange(type);
		ret.min = Math.min(ret.min, range.min);
		ret.max = Math.max(ret.max, range.max);
	}
	return ret;
};

Attack.prototype.GetBestAttackAgainst = function(target, allowCapture)
{
	let cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
	{
		// TODO: Formation against formation needs review
		let types = this.GetAttackTypes();
		return ["Ranged", "Melee", "Capture"].find(attack => types.indexOf(attack) != -1);
	}

	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	let targetClasses = cmpIdentity.GetClassesList();
	let isTargetClass = className => targetClasses.indexOf(className) != -1;

	// Always slaughter domestic animals instead of using a normal attack
	if (isTargetClass("Domestic") && this.template.Slaughter)
		return "Slaughter";

	let attack = this;
	let types = this.GetAttackTypes().filter(type => !attack.GetRestrictedClasses(type).some(isTargetClass));

	// check if the target is capturable
	let captureIndex = types.indexOf("Capture");
	if (captureIndex != -1)
	{
		let cmpCapturable = QueryMiragedInterface(target, IID_Capturable);

		let cmpPlayer = QueryOwnerInterface(this.entity);
		if (allowCapture && cmpPlayer && cmpCapturable && cmpCapturable.CanCapture(cmpPlayer.GetPlayerID()))
			return "Capture";
		// not captureable, so remove this attack
		types.splice(captureIndex, 1);
	}

	let isPreferred = className => attack.GetPreferredClasses(className).some(isTargetClass);

	return types.sort((a, b) =>
		(types.indexOf(a) + (isPreferred(a) ? types.length : 0)) -
		(types.indexOf(b) + (isPreferred(b) ? types.length : 0))).pop();
};

Attack.prototype.CompareEntitiesByPreference = function(a, b)
{
	let aPreference = this.GetPreference(a);
	let bPreference = this.GetPreference(b);

	if (aPreference === null && bPreference === null) return 0;
	if (aPreference === null) return 1;
	if (bPreference === null) return -1;
	return aPreference - bPreference;
};

Attack.prototype.GetTimers = function(type)
{
	let prepare = +(this.template[type].PrepareTime || 0);
	prepare = ApplyValueModificationsToEntity("Attack/" + type + "/PrepareTime", prepare, this.entity);

	let repeat = +(this.template[type].RepeatTime || 1000);
	repeat = ApplyValueModificationsToEntity("Attack/" + type + "/RepeatTime", repeat, this.entity);

	return { "prepare": prepare, "repeat": repeat };
};

Attack.prototype.GetAttackStrengths = function(type)
{
	// Work out the attack values with technology effects
	let template = this.template[type];
	let splash = "";
	if (!template)
	{
		template = this.template[type.split(".")[0]].Splash;
		splash = "/Splash";
	}

	let applyMods = damageType =>
		ApplyValueModificationsToEntity("Attack/" + type + splash + "/" + damageType, +(template[damageType] || 0), this.entity);

	if (type == "Capture")
		return { "value": applyMods("Value") };

	return {
		"hack": applyMods("Hack"),
		"pierce": applyMods("Pierce"),
		"crush": applyMods("Crush")
	};
};

Attack.prototype.GetSplashDamage = function(type)
{
	if (!this.template[type].Splash)
		return false;

	let splash = this.GetAttackStrengths(type + ".Splash");
	splash.friendlyFire = this.template[type].Splash.FriendlyFire != "false";
	return splash;
};

Attack.prototype.GetRange = function(type)
{
	let max = +this.template[type].MaxRange;
	max = ApplyValueModificationsToEntity("Attack/" + type + "/MaxRange", max, this.entity);

	let min = +(this.template[type].MinRange || 0);
	min = ApplyValueModificationsToEntity("Attack/" + type + "/MinRange", min, this.entity);

	let elevationBonus = +(this.template[type].ElevationBonus || 0);
	elevationBonus = ApplyValueModificationsToEntity("Attack/" + type + "/ElevationBonus", elevationBonus, this.entity);

	return { "max": max, "min": min, "elevationBonus": elevationBonus };
};

// Calculate the attack damage multiplier against a target
Attack.prototype.GetAttackBonus = function(type, target)
{
	let attackBonus = 1;
	let template = this.template[type];
	if (!template)
		template = this.template[type.split(".")[0]].Splash;

	if (template.Bonuses)
	{
		let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
		if (!cmpIdentity)
			return 1;

		// Multiply the bonuses for all matching classes
		for (let key in template.Bonuses)
		{
			let bonus = template.Bonuses[key];

			let hasClasses = true;
			if (bonus.Classes){
				let classes = bonus.Classes.split(/\s+/);
				for (let key in classes)
					hasClasses = hasClasses && cmpIdentity.HasClass(classes[key]);
			}

			if (hasClasses && (!bonus.Civ || bonus.Civ === cmpIdentity.GetCiv()))
				attackBonus *= bonus.Multiplier;
		}
	}

	return attackBonus;
};

// Returns a 2d random distribution scaled for a spread of scale 1.
// The current implementation is a 2d gaussian with sigma = 1
Attack.prototype.GetNormalDistribution = function(){

	// Use the Box-Muller transform to get a gaussian distribution
	let a = Math.random();
	let b = Math.random();

	let c = Math.sqrt(-2*Math.log(a)) * Math.cos(2*Math.PI*b);
	let d = Math.sqrt(-2*Math.log(a)) * Math.sin(2*Math.PI*b);

	return [c, d];
};

/**
 * Attack the target entity. This should only be called after a successful range check,
 * and should only be called after GetTimers().repeat msec has passed since the last
 * call to PerformAttack.
 */
Attack.prototype.PerformAttack = function(type, target)
{
	let attackerOwner = Engine.QueryInterface(this.entity, IID_Ownership).GetOwner();
 	let cmpDamage = Engine.QueryInterface(SYSTEM_ENTITY, IID_Damage);
 	
	// If this is a ranged attack, then launch a projectile
	if (type == "Ranged")
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		let turnLength = cmpTimer.GetLatestTurnLength()/1000;
		// In the future this could be extended:
		//  * Obstacles like trees could reduce the probability of the target being hit
		//  * Obstacles like walls should block projectiles entirely

		// Get some data about the entity
		let horizSpeed = +this.template[type].ProjectileSpeed;
		let gravity = 9.81; // this affects the shape of the curve; assume it's constant for now

		let spread = +this.template.Ranged.Spread;
		spread = ApplyValueModificationsToEntity("Attack/Ranged/Spread", spread, this.entity);

		//horizSpeed /= 2; gravity /= 2; // slow it down for testing

		let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			return;
		let selfPosition = cmpPosition.GetPosition();
		let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
		if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
			return;
		let targetPosition = cmpTargetPosition.GetPosition();

		let relativePosition = Vector3D.sub(targetPosition, selfPosition);
		let previousTargetPosition = Engine.QueryInterface(target, IID_Position).GetPreviousPosition();

		let targetVelocity = Vector3D.sub(targetPosition, previousTargetPosition).div(turnLength);
		// The component of the targets velocity radially away from the archer
		let radialSpeed = relativePosition.dot(targetVelocity) / relativePosition.length();

		let horizDistance = targetPosition.horizDistanceTo(selfPosition);

		// This is an approximation of the time ot the target, it assumes that the target has a constant radial
		// velocity, but since units move in straight lines this is not true.  The exact value would be more
		// difficult to calculate and I think this is sufficiently accurate.  (I tested and for cavalry it was
		// about 5% of the units radius out in the worst case)
		let timeToTarget = horizDistance / (horizSpeed - radialSpeed);

		// Predict where the unit is when the missile lands.
		let predictedPosition = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);

		// Compute the real target point (based on spread and target speed)
		let range = this.GetRange(type);
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		let elevationAdaptedMaxRange = cmpRangeManager.GetElevationAdaptedRange(selfPosition, cmpPosition.GetRotation(), range.max, range.elevationBonus, 0);
		let distanceModifiedSpread = spread * horizDistance/elevationAdaptedMaxRange;

		let randNorm = this.GetNormalDistribution();
		let offsetX = randNorm[0] * distanceModifiedSpread * (1 + targetVelocity.length() / 20);
		let offsetZ = randNorm[1] * distanceModifiedSpread * (1 + targetVelocity.length() / 20);

		let realTargetPosition = new Vector3D(predictedPosition.x + offsetX, targetPosition.y, predictedPosition.z + offsetZ);

		// Calculate when the missile will hit the target position
		let realHorizDistance = realTargetPosition.horizDistanceTo(selfPosition);
		timeToTarget = realHorizDistance / horizSpeed;

		let missileDirection = Vector3D.sub(realTargetPosition, selfPosition).div(realHorizDistance);

		// Launch the graphical projectile
		let cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
		let id = cmpProjectileManager.LaunchProjectileAtPoint(this.entity, realTargetPosition, horizSpeed, gravity);

		cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		let data = {
			"type": type,
			"attacker": this.entity,
			"target": target,
			"strengths": this.GetAttackStrengths(type),
			"position": realTargetPosition,
			"direction": missileDirection,
			"projectileId": id,
			"multiplier": this.GetAttackBonus(type, target),
			"isSplash": false,
			"attackerOwner": attackerOwner
		};
		if (this.template.Ranged.Splash)
		{
			data.friendlyFire = this.template.Ranged.Splash.FriendlyFire != "false";
			data.radius = +this.template.Ranged.Splash.Range;
			data.shape = this.template.Ranged.Splash.Shape;
			data.isSplash = true;
			data.splashStrengths = this.GetAttackStrengths(type+".Splash");
		}
		cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Damage, "MissileHit", timeToTarget * 1000, data);
	}
	else if (type == "Capture")
	{
		if (attackerOwner == -1)
			return;
		
		let multiplier = this.GetAttackBonus(type, target);
		let cmpHealth = Engine.QueryInterface(target, IID_Health);
		if (!cmpHealth || cmpHealth.GetHitpoints() == 0)
			return;
		multiplier *= cmpHealth.GetMaxHitpoints() / (0.1 * cmpHealth.GetMaxHitpoints() + 0.9 * cmpHealth.GetHitpoints());

		let cmpCapturable = Engine.QueryInterface(target, IID_Capturable);
		if (!cmpCapturable || !cmpCapturable.CanCapture(attackerOwner))
			return;

		let strength = this.GetAttackStrengths("Capture").value * multiplier;
		if (cmpCapturable.Reduce(strength, attackerOwner))
			Engine.PostMessage(target, MT_Attacked, {
				"attacker": this.entity,
				"target": target,
				"type": type,
				"damage": strength,
				"attackerOwner": attackerOwner
			});
	}
	else
	{
		// Melee attack - hurt the target immediately
		cmpDamage.CauseDamage({
			"strengths": this.GetAttackStrengths(type),
			"target": target,
			"attacker": this.entity,
			"multiplier": this.GetAttackBonus(type, target),
			"type": type,
			"attackerOwner": attackerOwner
		});
	}
};

Attack.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Attack")
		return;

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (!cmpUnitAI)
		return;

	if (this.GetAttackTypes().some(type =>
	      msg.valueNames.indexOf("Attack/" + type + "/MaxRange") != -1))
		cmpUnitAI.UpdateRangeQueries();
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
