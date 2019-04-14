function Attack() {}

var g_AttackTypes = ["Melee", "Ranged", "Capture"];

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
			"<Delay>1000</Delay>" +
			"<Bonuses>" +
				"<Bonus1>" +
					"<Classes>Cavalry</Classes>" +
					"<Multiplier>2</Multiplier>" +
				"</Bonus1>" +
			"</Bonuses>" +
			"<Projectile>" +
				"<Speed>50.0</Speed>" +
				"<Spread>2.5</Spread>" +
				"<ActorName>props/units/weapons/rock_flaming.xml</ActorName>" +
				"<ImpactActorName>props/units/weapons/rock_explosion.xml</ImpactActorName>" +
				"<ImpactAnimationLifetime>0.1</ImpactAnimationLifetime>" +
			"</Projectile>" +
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
				DamageTypes.BuildSchema("damage strength") +
				"<element name='MaxRange' a:help='Maximum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='PrepareTime' a:help='Time from the start of the attack command until the attack actually occurs (in milliseconds). This value relative to RepeatTime should closely match the \"event\" point in the actor&apos;s attack animation'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
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
				DamageTypes.BuildSchema("damage strength") +
				"<element name='MaxRange' a:help='Maximum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<element name='MinRange' a:help='Minimum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<optional>"+
					"<element name='ElevationBonus' a:help='give an elevation advantage (in meters)'><ref name='nonNegativeDecimal'/></element>" +
				"</optional>" +
				"<optional>" +
					"<element name='RangeOverlay'>" +
						"<interleave>" +
							"<element name='LineTexture'><text/></element>" +
							"<element name='LineTextureMask'><text/></element>" +
							"<element name='LineThickness'><ref name='nonNegativeDecimal'/></element>" +
						"</interleave>" +
					"</element>" +
				"</optional>" +
				"<element name='PrepareTime' a:help='Time from the start of the attack command until the attack actually occurs (in milliseconds). This value relative to RepeatTime should closely match the \"event\" point in the actor&apos;s attack animation'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" +
					"<data type='positiveInteger'/>" +
				"</element>" +
				"<element name='Delay' a:help='Delay of the damage in milliseconds'><ref name='nonNegativeDecimal'/></element>" +
				"<optional>" +
					"<element name='Splash'>" +
						"<interleave>" +
							"<element name='Shape' a:help='Shape of the splash damage, can be circular or linear'><text/></element>" +
							"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
							DamageTypes.BuildSchema("damage strength") +
							Attack.prototype.bonusesSchema +
						"</interleave>" +
					"</element>" +
				"</optional>" +
				"<element name='Projectile'>" +
					"<interleave>" +
						"<element name='Speed' a:help='Speed of projectiles (in meters per second).'>" +
							"<ref name='positiveDecimal'/>" +
						"</element>" +
						"<element name='Spread' a:help='Standard deviation of the bivariate normal distribution of hits at 100 meters. A disk at 100 meters from the attacker with this radius (2x this radius, 3x this radius) is expected to include the landing points of 39.3% (86.5%, 98.9%) of the rounds.'><ref name='nonNegativeDecimal'/></element>" +
						"<element name='Gravity' a:help='The gravity affecting the projectile. This affects the shape of the flight curve.'>" +
							"<ref name='nonNegativeDecimal'/>" +
						"</element>" +
						"<optional>" +
							"<element name='LaunchPoint' a:help='Delta from the unit position where to launch the projectile.'>" +
								"<attribute name='y'>" +
									"<data type='decimal'/>" +
								"</attribute>" +
							"</element>" +
						"</optional>" +
						"<optional>" +
							"<element name='ActorName' a:help='actor of the projectile animation.'>" +
								"<text/>" +
							"</element>" +
						"</optional>" +
						"<optional>" +
							"<element name='ImpactActorName' a:help='actor of the projectile impact animation'>" +
								"<text/>" +
							"</element>" +
							"<element name='ImpactAnimationLifetime' a:help='length of the projectile impact animation.'>" +
								"<ref name='positiveDecimal'/>" +
							"</element>" +
						"</optional>" +
					"</interleave>" +
				"</element>" +
				Attack.prototype.bonusesSchema +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
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
				DamageTypes.BuildSchema("damage strength") +
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

Attack.prototype.GetAttackTypes = function(wantedTypes)
{
	let types = g_AttackTypes.filter(type => !!this.template[type]);
	if (!wantedTypes)
		return types;

	let wantedTypesReal = wantedTypes.filter(wtype => wtype.indexOf("!") != 0);
	return types.filter(type => wantedTypes.indexOf("!" + type) == -1 &&
	      (!wantedTypesReal || !wantedTypesReal.length || wantedTypesReal.indexOf(type) != -1));
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

Attack.prototype.CanAttack = function(target, wantedTypes)
{
	let cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		return true;

	let cmpThisPosition = Engine.QueryInterface(this.entity, IID_Position);
	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpThisPosition || !cmpTargetPosition || !cmpThisPosition.IsInWorld() || !cmpTargetPosition.IsInWorld())
		return false;

	let cmpIdentity = QueryMiragedInterface(target, IID_Identity);
	if (!cmpIdentity)
		return false;

	let cmpHealth = QueryMiragedInterface(target, IID_Health);
	let targetClasses = cmpIdentity.GetClassesList();
	if (targetClasses.indexOf("Domestic") != -1 && this.template.Slaughter && cmpHealth && cmpHealth.GetHitpoints() &&
	   (!wantedTypes || !wantedTypes.filter(wType => wType.indexOf("!") != 0).length))
		return true;

	let cmpEntityPlayer = QueryOwnerInterface(this.entity);
	let cmpTargetPlayer = QueryOwnerInterface(target);
	if (!cmpTargetPlayer || !cmpEntityPlayer)
		return false;

	let types = this.GetAttackTypes(wantedTypes);
	let entityOwner = cmpEntityPlayer.GetPlayerID();
	let targetOwner = cmpTargetPlayer.GetPlayerID();
	let cmpCapturable = QueryMiragedInterface(target, IID_Capturable);

	// Check if the relative height difference is larger than the attack range
	// If the relative height is bigger, it means they will never be able to
	// reach each other, no matter how close they come.
	let heightDiff = Math.abs(cmpThisPosition.GetHeightOffset() - cmpTargetPosition.GetHeightOffset());

	for (let type of types)
	{
		if (type != "Capture" && (!cmpEntityPlayer.IsEnemy(targetOwner) || !cmpHealth || !cmpHealth.GetHitpoints()))
			continue;

		if (type == "Capture" && (!cmpCapturable || !cmpCapturable.CanCapture(entityOwner)))
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
	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	let targetClasses = cmpIdentity.GetClassesList();

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
		return g_AttackTypes.find(attack => types.indexOf(attack) != -1);
	}

	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	let targetClasses = cmpIdentity.GetClassesList();
	let isTargetClass = className => targetClasses.indexOf(className) != -1;

	// Always slaughter domestic animals instead of using a normal attack
	if (isTargetClass("Domestic") && this.template.Slaughter)
		return "Slaughter";

	let types = this.GetAttackTypes().filter(type => !this.GetRestrictedClasses(type).some(isTargetClass));

	// check if the target is capturable
	let captureIndex = types.indexOf("Capture");
	if (captureIndex != -1)
	{
		let cmpCapturable = QueryMiragedInterface(target, IID_Capturable);

		let cmpPlayer = QueryOwnerInterface(this.entity);
		if (allowCapture && cmpPlayer && cmpCapturable && cmpCapturable.CanCapture(cmpPlayer.GetPlayerID()))
			return "Capture";
		// not capturable, so remove this attack
		types.splice(captureIndex, 1);
	}

	let isPreferred = className => this.GetPreferredClasses(className).some(isTargetClass);

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

	let ret = {};
	for (let damageType of DamageTypes.GetTypes())
		ret[damageType] = applyMods(damageType);

	return ret;
};

Attack.prototype.GetSplashDamage = function(type)
{
	if (!this.template[type].Splash)
		return false;

	let splash = this.GetAttackStrengths(type + ".Splash");
	splash.friendlyFire = this.template[type].Splash.FriendlyFire != "false";
	splash.shape = this.template[type].Splash.Shape;
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

Attack.prototype.GetBonusTemplate = function(type)
{
	let template = this.template[type];
	if (!template)
		template = this.template[type.split(".")[0]].Splash;

	return template.Bonuses || null;
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

		let horizSpeed = +this.template[type].Projectile.Speed;
		let gravity = +this.template[type].Projectile.Gravity;
		//horizSpeed /= 2; gravity /= 2; // slow it down for testing

		let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			return;
		let selfPosition = cmpPosition.GetPosition();
		let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
		if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
			return;
		let targetPosition = cmpTargetPosition.GetPosition();

		let previousTargetPosition = Engine.QueryInterface(target, IID_Position).GetPreviousPosition();
		let targetVelocity = Vector3D.sub(targetPosition, previousTargetPosition).div(turnLength);

		let timeToTarget = this.PredictTimeToTarget(selfPosition, horizSpeed, targetPosition, targetVelocity);
		let predictedPosition = (timeToTarget !== false) ? Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition) : targetPosition;

		// Add inaccuracy based on spread.
		let distanceModifiedSpread = ApplyValueModificationsToEntity("Attack/Ranged/Spread", +this.template[type].Projectile.Spread, this.entity) *
			predictedPosition.horizDistanceTo(selfPosition) / 100;

		let randNorm = randomNormal2D();
		let offsetX = randNorm[0] * distanceModifiedSpread;
		let offsetZ = randNorm[1] * distanceModifiedSpread;

		let realTargetPosition = new Vector3D(predictedPosition.x + offsetX, targetPosition.y, predictedPosition.z + offsetZ);

		// Recalculate when the missile will hit the target position.
		let realHorizDistance = realTargetPosition.horizDistanceTo(selfPosition);
		timeToTarget = realHorizDistance / horizSpeed;

		let missileDirection = Vector3D.sub(realTargetPosition, selfPosition).div(realHorizDistance);

		// Launch the graphical projectile.
		let cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);

		let actorName = "";
		let impactActorName = "";
		let impactAnimationLifetime = 0;

		actorName = this.template[type].Projectile.ActorName || "";
		impactActorName = this.template[type].Projectile.ImpactActorName || "";
		impactAnimationLifetime = this.template[type].Projectile.ImpactAnimationLifetime || 0;

		// TODO: Use unit rotation to implement x/z offsets.
		let deltaLaunchPoint = new Vector3D(0, this.template[type].Projectile.LaunchPoint["@y"], 0.0);
		let launchPoint = Vector3D.add(selfPosition, deltaLaunchPoint);
		
		let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
		if (cmpVisual)
		{
			// if the projectile definition is missing from the template
			// then fallback to the projectile name and launchpoint in the visual actor
			if (!actorName)
				actorName = cmpVisual.GetProjectileActor();

			let visualActorLaunchPoint = cmpVisual.GetProjectileLaunchPoint();
			if (visualActorLaunchPoint.length() > 0)
				launchPoint = visualActorLaunchPoint;
		}

		let id = cmpProjectileManager.LaunchProjectileAtPoint(launchPoint, realTargetPosition, horizSpeed, gravity, actorName, impactActorName, impactAnimationLifetime);

		let attackImpactSound = "";
		let cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
		if (cmpSound)
			attackImpactSound = cmpSound.GetSoundGroup("attack_impact_" + type.toLowerCase());

		let data = {
			"type": type,
			"attacker": this.entity,
			"target": target,
			"strengths": this.GetAttackStrengths(type),
			"position": realTargetPosition,
			"direction": missileDirection,
			"projectileId": id,
			"bonus": this.GetBonusTemplate(type),
			"isSplash": false,
			"attackerOwner": attackerOwner,
			"attackImpactSound": attackImpactSound
		};
		if (this.template[type].Splash)
		{
			data.friendlyFire = this.template[type].Splash.FriendlyFire != "false";
			data.radius = +this.template[type].Splash.Range;
			data.shape = this.template[type].Splash.Shape;
			data.isSplash = true;
			data.splashStrengths = this.GetAttackStrengths(type + ".Splash");
			data.splashBonus = this.GetBonusTemplate(type + ".Splash");
		}
		cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_Damage, "MissileHit", timeToTarget * 1000 + +this.template[type].Delay, data);
	}
	else if (type == "Capture")
	{
		if (attackerOwner == INVALID_PLAYER)
			return;

		let multiplier = GetDamageBonus(target, this.GetBonusTemplate(type));
		let cmpHealth = Engine.QueryInterface(target, IID_Health);
		if (!cmpHealth || cmpHealth.GetHitpoints() == 0)
			return;
		multiplier *= cmpHealth.GetMaxHitpoints() / (0.1 * cmpHealth.GetMaxHitpoints() + 0.9 * cmpHealth.GetHitpoints());

		let cmpCapturable = Engine.QueryInterface(target, IID_Capturable);
		if (!cmpCapturable || !cmpCapturable.CanCapture(attackerOwner))
			return;

		let strength = this.GetAttackStrengths("Capture").value * multiplier;
		if (cmpCapturable.Reduce(strength, attackerOwner) && IsOwnedByEnemyOfPlayer(attackerOwner, target))
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
			"multiplier": GetDamageBonus(target, this.GetBonusTemplate(type)),
			"type": type,
			"attackerOwner": attackerOwner
		});
	}
};

/**
 * Get the predicted time of collision between a projectile (or a chaser)
 * and its target, assuming they both move in straight line at a constant speed.
 * Vertical component of movement is ignored.
 * @param {Vector3D} selfPosition - the 3D position of the projectile (or chaser).
 * @param {number} horizSpeed - the horizontal speed of the projectile (or chaser).
 * @param {Vector3D} targetPosition - the 3D position of the target.
 * @param {Vector3D} targetVelocity - the 3D velocity vector of the target.
 * @return {Vector3D|boolean} - the 3D predicted position or false if the collision will not happen.
 */
Attack.prototype.PredictTimeToTarget = function(selfPosition, horizSpeed, targetPosition, targetVelocity)
{
	let relativePosition = new Vector3D.sub(targetPosition, selfPosition);
	let a = targetVelocity.x * targetVelocity.x + targetVelocity.z * targetVelocity.z - horizSpeed * horizSpeed;
	let b = relativePosition.x * targetVelocity.x + relativePosition.z * targetVelocity.z;
	let c = relativePosition.x * relativePosition.x + relativePosition.z * relativePosition.z;
	// The predicted time to reach the target is the smallest non negative solution
	// (when it exists) of the equation a t^2 + 2 b t + c = 0.
	// Using c>=0, we can straightly compute the right solution.

	if (c == 0)
		return 0;

	let disc = b * b - a * c;
	if (a < 0 || b < 0 && disc >= 0)
		return c / (Math.sqrt(disc) - b);

	return false;
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

Attack.prototype.GetRangeOverlays = function()
{
	if (!this.template.Ranged || !this.template.Ranged.RangeOverlay)
		return [];

	let range = this.GetRange("Ranged");
	let rangeOverlays = [];
	for (let i in range)
		if ((i == "min" || i == "max") && range[i])
			rangeOverlays.push({
				"radius": range[i],
				"texture": this.template.Ranged.RangeOverlay.LineTexture,
				"textureMask": this.template.Ranged.RangeOverlay.LineTextureMask,
				"thickness": +this.template.Ranged.RangeOverlay.LineThickness,
			});
	return rangeOverlays;
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
