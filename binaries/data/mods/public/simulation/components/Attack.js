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

var preferredClassesSchema =
	"<optional>" +
		"<element name='PreferredClasses' a:help='Space delimited list of classes preferred for attacking. If an entity has any of theses classes, it is preferred. The classes are in decending order of preference'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>";

var restrictedClassesSchema =
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
		"<Charge>" +
			"<Hack>10.0</Hack>" +
			"<Pierce>0.0</Pierce>" +
			"<Crush>50.0</Crush>" +
			"<MaxRange>24.0</MaxRange>" +
			"<MinRange>20.0</MinRange>" +
		"</Charge>" +
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
				bonusesSchema +
				preferredClassesSchema +
				restrictedClassesSchema +
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
				bonusesSchema +
				preferredClassesSchema +
				restrictedClassesSchema +
				"<optional>" +
					"<element name='Splash'>" +
						"<interleave>" +
							"<element name='Shape' a:help='Shape of the splash damage, can be circular or linear'><text/></element>" +
							"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
							"<element name='Hack' a:help='Hack damage strength'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='Pierce' a:help='Pierce damage strength'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='Crush' a:help='Crush damage strength'><ref name='nonNegativeDecimal'/></element>" +
							bonusesSchema +
						"</interleave>" +
					"</element>" +
				"</optional>" +
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
				preferredClassesSchema +
				restrictedClassesSchema +
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
				bonusesSchema +
				preferredClassesSchema +
				restrictedClassesSchema +
			"</interleave>" +
		"</element>" +
	"</optional>";

Attack.prototype.Init = function()
{
};

Attack.prototype.Serialize = null; // we have no dynamic state to save

Attack.prototype.GetAttackTypes = function()
{
	var ret = [];
	if (this.template.Charge) ret.push("Charge");
	if (this.template.Melee) ret.push("Melee");
	if (this.template.Ranged) ret.push("Ranged");
	return ret;
};

Attack.prototype.GetPreferredClasses = function(type)
{
	if (this.template[type] && this.template[type].PreferredClasses
	    && this.template[type].PreferredClasses._string)
	{
		return this.template[type].PreferredClasses._string.split(/\s+/);
	}
	return [];
};

Attack.prototype.GetRestrictedClasses = function(type)
{
	if (this.template[type] && this.template[type].RestrictedClasses
	    && this.template[type].RestrictedClasses._string)
	{
		return this.template[type].RestrictedClasses._string.split(/\s+/);
	}
	return [];
};

Attack.prototype.CanAttack = function(target)
{
	var cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		return true;

	var cmpThisPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpThisPosition || !cmpTargetPosition || !cmpThisPosition.IsInWorld() || !cmpTargetPosition.IsInWorld())
		return false;

	// Check if the relative height difference is larger than the attack range
	// If the relative height is bigger, it means they will never be able to
	// reach each other, no matter how close they come.
	var heightDiff = Math.abs(cmpThisPosition.GetHeightOffset() - cmpTargetPosition.GetHeightOffset());

	const cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity) 
		return undefined;

	const targetClasses = cmpIdentity.GetClassesList();

	for each (var type in this.GetAttackTypes())
	{
		if (heightDiff > this.GetRange(type).max)
			continue;

		var canAttack = true;
		var restrictedClasses = this.GetRestrictedClasses(type);

		for each (var targetClass in targetClasses)
		{
			if (restrictedClasses.indexOf(targetClass) != -1)
			{
				canAttack = false;
				break;
			}
		}
		if (canAttack)
		{
			return true;
		}
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

	var minPref = null;
	for each (var type in this.GetAttackTypes())
	{
		for each (var targetClass in targetClasses)
		{
			var pref = this.GetPreferredClasses(type).indexOf(targetClass);
			if (pref != -1 && (minPref === null || minPref > pref))
			{
				minPref = pref;
			}
		}
	}
	return minPref;
};

/**
 * Return the type of the best attack.
 * TODO: this should probably depend on range, target, etc,
 * so we can automatically switch between ranged and melee
 */
Attack.prototype.GetBestAttack = function()
{
	return this.GetAttackTypes().pop();
};

Attack.prototype.GetBestAttackAgainst = function(target)
{
	var cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		return this.GetBestAttack();

	const cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity) 
		return undefined;

	const targetClasses = cmpIdentity.GetClassesList();
	const isTargetClass = function (value, i, a) { return targetClasses.indexOf(value) != -1; };
	const types = this.GetAttackTypes();
	const attack = this;
	const isAllowed = function (value, i, a) { return !attack.GetRestrictedClasses(value).some(isTargetClass); }
	const isPreferred = function (value, i, a) { return attack.GetPreferredClasses(value).some(isTargetClass); }
	const byPreference = function (a, b) { return (types.indexOf(a) + (isPreferred(a) ? types.length : 0) ) - (types.indexOf(b) + (isPreferred(b) ? types.length : 0) ); }

	// Always slaughter domestic animals instead of using a normal attack
	if (isTargetClass("Domestic") && this.template.Slaughter) 
		return "Slaughter";

	return types.filter(isAllowed).sort(byPreference).pop();
};

Attack.prototype.CompareEntitiesByPreference = function(a, b)
{
	var aPreference = this.GetPreference(a);
	var bPreference = this.GetPreference(b);

	if (aPreference === null && bPreference === null) return 0;
	if (aPreference === null) return 1;
	if (bPreference === null) return -1;
	return aPreference - bPreference;
};

Attack.prototype.GetTimers = function(type)
{
	var prepare = +(this.template[type].PrepareTime || 0);
	prepare = ApplyValueModificationsToEntity("Attack/" + type + "/PrepareTime", prepare, this.entity);
	
	var repeat = +(this.template[type].RepeatTime || 1000);
	repeat = ApplyValueModificationsToEntity("Attack/" + type + "/RepeatTime", repeat, this.entity);

	return { "prepare": prepare, "repeat": repeat, "recharge": repeat - prepare };
};

Attack.prototype.GetAttackStrengths = function(type)
{
	// Work out the attack values with technology effects
	var self = this;

	var template = this.template[type];
	var splash = "";
	if (!template)
	{
		template = this.template[type.split(".")[0]].Splash;
		splash = "/Splash";
	}
	
	var applyMods = function(damageType)
	{
		return ApplyValueModificationsToEntity("Attack/" + type + splash + "/" + damageType, +(template[damageType] || 0), self.entity);
	};
	
	return {
		hack: applyMods("Hack"),
		pierce: applyMods("Pierce"),
		crush: applyMods("Crush")
	};
};

Attack.prototype.GetRange = function(type)
{
	var max = +this.template[type].MaxRange;
	max = ApplyValueModificationsToEntity("Attack/" + type + "/MaxRange", max, this.entity);
	
	var min = +(this.template[type].MinRange || 0);
	min = ApplyValueModificationsToEntity("Attack/" + type + "/MinRange", min, this.entity);

	var elevationBonus = +(this.template[type].ElevationBonus || 0);
	elevationBonus = ApplyValueModificationsToEntity("Attack/" + type + "/ElevationBonus", elevationBonus, this.entity);
	
	return { "max": max, "min": min, "elevationBonus": elevationBonus};
};

// Calculate the attack damage multiplier against a target
Attack.prototype.GetAttackBonus = function(type, target)
{
	var attackBonus = 1;
	var template = this.template[type];
	if (!template)
		template = this.template[type.split(".")[0]].Splash;

	if (template.Bonuses)
	{
		var cmpIdentity = Engine.QueryInterface(target, IID_Identity);
		if (!cmpIdentity)
			return 1;

		// Multiply the bonuses for all matching classes
		for (var key in template.Bonuses)
		{
			var bonus = template.Bonuses[key];

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

// Returns a 2d random distribution scaled for a spread of scale 1.
// The current implementation is a 2d gaussian with sigma = 1
Attack.prototype.GetNormalDistribution = function(){

	// Use the Box-Muller transform to get a gaussian distribution
	var a = Math.random();
	var b = Math.random();

	var c = Math.sqrt(-2*Math.log(a)) * Math.cos(2*Math.PI*b);
	var d = Math.sqrt(-2*Math.log(a)) * Math.sin(2*Math.PI*b);

	return [c, d];
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
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		var turnLength = cmpTimer.GetLatestTurnLength()/1000;
		// In the future this could be extended:
		//  * Obstacles like trees could reduce the probability of the target being hit
		//  * Obstacles like walls should block projectiles entirely

		// Get some data about the entity
		var horizSpeed = +this.template[type].ProjectileSpeed;
		var gravity = 9.81; // this affects the shape of the curve; assume it's constant for now

		var spread = +this.template.Ranged.Spread;
		spread = ApplyValueModificationsToEntity("Attack/Ranged/Spread", spread, this.entity);

		//horizSpeed /= 2; gravity /= 2; // slow it down for testing

		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
			return;
		var selfPosition = cmpPosition.GetPosition();
		var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
		if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
			return;
		var targetPosition = cmpTargetPosition.GetPosition();

		var relativePosition = Vector3D.sub(targetPosition, selfPosition);
		var previousTargetPosition = Engine.QueryInterface(target, IID_Position).GetPreviousPosition();

		var targetVelocity = Vector3D.sub(targetPosition, previousTargetPosition).div(turnLength);
		// the component of the targets velocity radially away from the archer
		var radialSpeed = relativePosition.dot(targetVelocity) / relativePosition.length();

		var horizDistance = targetPosition.horizDistanceTo(selfPosition);

		// This is an approximation of the time ot the target, it assumes that the target has a constant radial 
		// velocity, but since units move in straight lines this is not true.  The exact value would be more 
		// difficult to calculate and I think this is sufficiently accurate.  (I tested and for cavalry it was 
		// about 5% of the units radius out in the worst case)
		var timeToTarget = horizDistance / (horizSpeed - radialSpeed);

		// Predict where the unit is when the missile lands.
		var predictedPosition = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);

		// Compute the real target point (based on spread and target speed)
		var range = this.GetRange(type);
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var elevationAdaptedMaxRange = cmpRangeManager.GetElevationAdaptedRange(selfPosition, cmpPosition.GetRotation(), range.max, range.elevationBonus, 0);
		var distanceModifiedSpread = spread * horizDistance/elevationAdaptedMaxRange;

		var randNorm = this.GetNormalDistribution();
		var offsetX = randNorm[0] * distanceModifiedSpread * (1 + targetVelocity.length() / 20);
		var offsetZ = randNorm[1] * distanceModifiedSpread * (1 + targetVelocity.length() / 20);

		var realTargetPosition = new Vector3D(predictedPosition.x + offsetX, targetPosition.y, predictedPosition.z + offsetZ);

		// Calculate when the missile will hit the target position
		var realHorizDistance = realTargetPosition.horizDistanceTo(selfPosition);
		var timeToTarget = realHorizDistance / horizSpeed;

		var missileDirection = Vector3D.sub(realTargetPosition, selfPosition).div(realHorizDistance);

		// Make the arrow appear to land slightly behind the target so that arrows landing next to a guys foot don't count but arrows that go through the torso do
		var graphicalPosition = Vector3D.mult(missileDirection, 2).add(realTargetPosition);
		// Launch the graphical projectile
		var cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
		var id = cmpProjectileManager.LaunchProjectileAtPoint(this.entity, realTargetPosition, horizSpeed, gravity);

		var playerId = Engine.QueryInterface(this.entity, IID_Ownership).GetOwner()
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
 		cmpTimer.SetTimeout(this.entity, IID_Attack, "MissileHit", timeToTarget*1000, {"type": type, "target": target, "position": realTargetPosition, "direction": missileDirection, "projectileId": id, "playerId":playerId});
	}
	else
	{
		// Melee attack - hurt the target immediately
		Damage.CauseDamage({"strengths":this.GetAttackStrengths(type), "target":target, "attacker":this.entity, "multiplier":this.GetAttackBonus(type, target), "type":type});
	}
	// TODO: charge attacks (need to design how they work)
};

Attack.prototype.InterpolatedLocation = function(ent, lateness)
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var turnLength = cmpTimer.GetLatestTurnLength()/1000;
	var cmpTargetPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld()) // TODO: handle dead target properly
		return undefined;
	var curPos = cmpTargetPosition.GetPosition();
	var prevPos = cmpTargetPosition.GetPreviousPosition();
	lateness /= 1000;
	return new Vector3D((curPos.x * (turnLength - lateness) + prevPos.x * lateness) / turnLength,
			0,
	        (curPos.z * (turnLength - lateness) + prevPos.z * lateness) / turnLength);
};

// Tests whether it point is inside of ent's footprint
Attack.prototype.testCollision = function(ent, point, lateness)
{
	var targetPosition = this.InterpolatedLocation(ent, lateness);
	if (!targetPosition)
		return false;
	var cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
	if (!cmpFootprint)
		return false;
	var targetShape = cmpFootprint.GetShape();

	if (!targetShape || !targetPosition)
		return false;

	if (targetShape.type === 'circle')
	{
		// Use VectorDistanceSquared and square targetShape.radius to avoid square roots.
		return (point.horizDistanceTo(targetPosition) < (targetShape.radius * targetShape.radius));
	}
	else
	{
		var angle = Engine.QueryInterface(ent, IID_Position).GetRotation().y;

		var d = Vector3D.sub(point, targetPosition);
		d = Vector2D.from3D(d).rotate(-angle);

		return d.x < Math.abs(targetShape.width/2) && d.y < Math.abs(targetShape.depth/2);
	}
};

Attack.prototype.MissileHit = function(data, lateness)
{
	var targetPosition = this.InterpolatedLocation(data.target, lateness);
	if (!targetPosition)
		return;

	if (this.template.Ranged.Splash) // splash damage, do this first in case the direct hit kills the target
	{
		var friendlyFire = this.template.Ranged.Splash.FriendlyFire;
		var splashRadius = this.template.Ranged.Splash.Range;
		var splashShape = this.template.Ranged.Splash.Shape;
		var playersToDamage;
		// If friendlyFire isn't enabled, get all player enemies to pass to "Damage.CauseSplashDamage".
		if (friendlyFire == false)
		{
			var cmpPlayer = Engine.QueryInterface(Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetPlayerByID(data.playerId), IID_Player)
			playersToDamage = cmpPlayer.GetEnemies();
		}
		// Damage the units.
		Damage.CauseSplashDamage({"attacker":this.entity, "origin":Vector2D.from3D(data.position), "radius":splashRadius, "shape":splashShape, "strengths":this.GetAttackStrengths(data.type), "direction":data.direction, "playersToDamage":playersToDamage, "type":data.type});
	}

	if (this.testCollision(data.target, data.position, lateness))
	{
		data.attacker = this.entity
		data.multiplier = this.GetAttackBonus(data.type, data.target)
		data.strengths = this.GetAttackStrengths(data.type)
		// Hit the primary target
		Damage.CauseDamage(data);

		// Remove the projectile
		var cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
		cmpProjectileManager.RemoveProjectile(data.projectileId);
	}
	else
	{
		// If we didn't hit the main target look for nearby units
		var cmpPlayer = Engine.QueryInterface(Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetPlayerByID(data.playerId), IID_Player)
		var ents = Damage.EntitiesNearPoint(data.position, data.position.horizDistanceTo(targetPosition) * 2, cmpPlayer.GetEnemies());

		for (var i = 0; i < ents.length; i++)
		{
			if (this.testCollision(ents[i], data.position, lateness))
			{
				var newData = {"strengths":this.GetAttackStrengths(data.type), "target":ents[i], "attacker":this.entity, "multiplier":this.GetAttackBonus(data.type, ents[i]), "type":data.type};
				Damage.CauseDamage(newData);
				// Remove the projectile
				var cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
				cmpProjectileManager.RemoveProjectile(data.projectileId);
			}
		}
	}
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
