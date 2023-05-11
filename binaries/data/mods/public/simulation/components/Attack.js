function Attack() {}

var g_AttackTypes = ["Melee", "Ranged", "Capture"];

Attack.prototype.preferredClassesSchema =
	"<optional>" +
		"<element name='PreferredClasses' a:help='Space delimited list of classes preferred for attacking. If an entity has any of theses classes, it is preferred. The classes are in descending order of preference'>" +
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
			"<AttackName>Spear</AttackName>" +
			"<Damage>" +
				"<Hack>10.0</Hack>" +
				"<Pierce>0.0</Pierce>" +
				"<Crush>5.0</Crush>" +
			"</Damage>" +
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
			"<AttackName>Bow</AttackName>" +
			"<Damage>" +
				"<Hack>0.0</Hack>" +
				"<Pierce>10.0</Pierce>" +
				"<Crush>0.0</Crush>" +
			"</Damage>" +
			"<MaxRange>44.0</MaxRange>" +
			"<MinRange>20.0</MinRange>" +
			"<Origin>" +
				"<X>0</X>" +
				"<Y>10.0</Y>" +
				"<Z>0</Z>" +
			"</Origin>" +
			"<PrepareTime>800</PrepareTime>" +
			"<RepeatTime>1600</RepeatTime>" +
			"<EffectDelay>1000</EffectDelay>" +
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
				"<FriendlyFire>false</FriendlyFire>" +
			"</Projectile>" +
			"<RestrictedClasses datatype=\"tokens\">Champion</RestrictedClasses>" +
			"<Splash>" +
				"<Shape>Circular</Shape>" +
				"<Range>20</Range>" +
				"<FriendlyFire>false</FriendlyFire>" +
				"<Damage>" +
					"<Hack>0.0</Hack>" +
					"<Pierce>10.0</Pierce>" +
					"<Crush>0.0</Crush>" +
				"</Damage>" +
			"</Splash>" +
		"</Ranged>" +
		"<Slaughter>" +
			"<Damage>" +
				"<Hack>1000.0</Hack>" +
				"<Pierce>0.0</Pierce>" +
				"<Crush>0.0</Crush>" +
			"</Damage>" +
			"<RepeatTime>1000</RepeatTime>" +
			"<MaxRange>4.0</MaxRange>" +
		"</Slaughter>" +
	"</a:example>" +
	"<oneOrMore>" +
		"<element>" +
			"<anyName a:help='Currently one of Melee, Ranged, Capture or Slaughter.'/>" +
			"<interleave>" +
				"<element name='AttackName' a:help='Name of the attack, to be displayed in the GUI. Optionally includes a translate context attribute.'>" +
					"<optional>" +
						"<attribute name='context'>" +
							"<text/>" +
						"</attribute>" +
					"</optional>" +
					"<text/>" +
				"</element>" +
				AttackHelper.BuildAttackEffectsSchema() +
				"<element name='MaxRange' a:help='Maximum attack range (in metres)'><ref name='nonNegativeDecimal'/></element>" +
				"<optional>" +
					"<element name='MinRange' a:help='Minimum attack range (in metres). Defaults to 0.'><ref name='nonNegativeDecimal'/></element>" +
				"</optional>" +
				"<optional>"+
					"<element name='Origin' a:help='The offset from which the attack occurs, relative to the entity position. Defaults to {0,0,0}.'>" +
						"<interleave>" +
							"<element name='X'>" +
								"<ref name='nonNegativeDecimal'/>" +
							"</element>" +
							"<element name='Y'>" +
								"<ref name='nonNegativeDecimal'/>" +
							"</element>" +
							"<element name='Z'>" +
								"<ref name='nonNegativeDecimal'/>" +
							"</element>" +
						"</interleave>" +
					"</element>" +
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
				"<optional>" +
					"<element name='PrepareTime' a:help='Time from the start of the attack command until the attack actually occurs (in milliseconds). This value relative to RepeatTime should closely match the \"event\" point in the actor&apos;s attack animation. Defaults to 0.'>" +
						"<data type='nonNegativeInteger'/>" +
					"</element>" +
				"</optional>" +
				"<element name='RepeatTime' a:help='Time between attacks (in milliseconds). The attack animation will be stretched to match this time'>" + // TODO: it shouldn't be stretched
					"<data type='positiveInteger'/>" +
				"</element>" +
				"<optional>" +
					"<element name='EffectDelay' a:help='Delay of applying the effects, in milliseconds after the attack has landed. Defaults to 0.'><ref name='nonNegativeDecimal'/></element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Splash'>" +
						"<interleave>" +
							"<element name='Shape' a:help='Shape of the splash damage, can be circular or linear'><text/></element>" +
							"<element name='Range' a:help='Size of the area affected by the splash'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='FriendlyFire' a:help='Whether the splash damage can hurt non enemy units'><data type='boolean'/></element>" +
							AttackHelper.BuildAttackEffectsSchema() +
						"</interleave>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Projectile'>" +
						"<interleave>" +
							"<element name='Speed' a:help='Speed of projectiles (in meters per second).'>" +
								"<ref name='positiveDecimal'/>" +
							"</element>" +
							"<element name='Spread' a:help='Standard deviation of the bivariate normal distribution of hits at 100 meters. A disk at 100 meters from the attacker with this radius (2x this radius, 3x this radius) is expected to include the landing points of 39.3% (86.5%, 98.9%) of the rounds.'><ref name='nonNegativeDecimal'/></element>" +
							"<element name='Gravity' a:help='The gravity affecting the projectile. This affects the shape of the flight curve.'>" +
								"<ref name='nonNegativeDecimal'/>" +
							"</element>" +
							"<element name='FriendlyFire' a:help='Whether stray missiles can hurt non enemy units.'><data type='boolean'/></element>" +
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
				"</optional>" +
				Attack.prototype.preferredClassesSchema +
				Attack.prototype.restrictedClassesSchema +
			"</interleave>" +
		"</element>" +
	"</oneOrMore>";

Attack.prototype.Init = function()
{
};

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
	const cmpFormation = Engine.QueryInterface(target, IID_Formation);
	if (cmpFormation)
		return true;

	const cmpThisPosition = Engine.QueryInterface(this.entity, IID_Position);
	const cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpThisPosition || !cmpTargetPosition || !cmpThisPosition.IsInWorld() || !cmpTargetPosition.IsInWorld())
		return false;

	const cmpResistance = QueryMiragedInterface(target, IID_Resistance);
	if (!cmpResistance)
		return false;

	const cmpIdentity = QueryMiragedInterface(target, IID_Identity);
	if (!cmpIdentity)
		return false;

	const cmpHealth = QueryMiragedInterface(target, IID_Health);
	const targetClasses = cmpIdentity.GetClassesList();
	if (targetClasses.indexOf("Domestic") != -1 && this.template.Slaughter && cmpHealth && cmpHealth.GetHitpoints() &&
	   (!wantedTypes || !wantedTypes.filter(wType => wType.indexOf("!") != 0).length || wantedTypes.indexOf("Slaughter") != -1))
		return true;

	const cmpEntityPlayer = QueryOwnerInterface(this.entity);
	const cmpTargetPlayer = QueryOwnerInterface(target);
	if (!cmpTargetPlayer || !cmpEntityPlayer)
		return false;

	const types = this.GetAttackTypes(wantedTypes);
	const entityOwner = cmpEntityPlayer.GetPlayerID();
	const targetOwner = cmpTargetPlayer.GetPlayerID();
	const cmpCapturable = QueryMiragedInterface(target, IID_Capturable);

	// Check if the relative height difference is larger than the attack range
	// If the relative height is bigger, it means they will never be able to
	// reach each other, no matter how close they come.
	const heightDiff = Math.abs(cmpThisPosition.GetHeightOffset() - cmpTargetPosition.GetHeightOffset());

	for (const type of types)
	{
		if (type != "Capture" && (!cmpEntityPlayer.IsEnemy(targetOwner) || !cmpHealth || !cmpHealth.GetHitpoints()))
			continue;

		if (type == "Capture" && (!cmpCapturable || !cmpCapturable.CanCapture(entityOwner)))
			continue;

		if (heightDiff > this.GetRange(type).max)
			continue;

		const restrictedClasses = this.GetRestrictedClasses(type);
		if (!restrictedClasses.length)
			return true;

		if (!MatchesClassList(targetClasses, restrictedClasses))
			return true;
	}

	return false;
};

/**
 * Returns undefined if we have no preference or the lowest index of a preferred class.
 */
Attack.prototype.GetPreference = function(target)
{
	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	let targetClasses = cmpIdentity.GetClassesList();

	let minPref;
	for (let type of this.GetAttackTypes())
	{
		let preferredClasses = this.GetPreferredClasses(type);
		for (let pref = 0; pref < preferredClasses.length; ++pref)
		{
			if (MatchesClassList(targetClasses, preferredClasses[pref]))
			{
				if (pref === 0)
					return pref;
				if ((minPref === undefined || minPref > pref))
					minPref = pref;
			}
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

Attack.prototype.GetAttackEffectsData = function(type, splash)
{
	let template = this.template[type];
	if (!template)
		return undefined;
	if (splash)
		template = template.Splash;
	return AttackHelper.GetAttackEffectsData("Attack/" + type + (splash ? "/Splash" : ""), template, this.entity);
};

/**
 * Find the best attack against a target.
 * @param {number} target - The entity-ID of the target.
 * @param {boolean} allowCapture - Whether capturing is allowed.
 * @return {string} - The preferred attack type.
 */
Attack.prototype.GetBestAttackAgainst = function(target, allowCapture)
{
	let types = this.GetAttackTypes();
	if (Engine.QueryInterface(target, IID_Formation))
		// TODO: Formation against formation needs review
		return g_AttackTypes.find(attack => types.indexOf(attack) != -1);

	const cmpIdentity = Engine.QueryInterface(target, IID_Identity);
	if (!cmpIdentity)
		return undefined;

	// Always slaughter domestic animals instead of using a normal attack
	if (this.template.Slaughter && cmpIdentity.HasClass("Domestic"))
		return "Slaughter";

	const targetClasses = cmpIdentity.GetClassesList();
	const getPreferrence = attackType => {
		let pref = 0;
		if (MatchesClassList(targetClasses, this.GetPreferredClasses(attackType)))
			pref += 2;
		if (allowCapture ? attackType === "Capture" : attackType !== "Capture")
			pref++;
		return pref;
	};

	return types.filter(type => this.CanAttack(target, [type])).sort((a, b) => {
		const prefA = getPreferrence(a);
		const prefB = getPreferrence(b);
		return (types.indexOf(a) + (prefA > 0 ? prefA + types.length : 0)) -
			(types.indexOf(b) + (prefB > 0 ? prefB + types.length : 0))
	}).pop();
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

Attack.prototype.GetAttackName = function(type)
{
	return {
		"name": this.template[type].AttackName._string || this.template[type].AttackName,
		"context": this.template[type].AttackName["@context"]
	};
};

Attack.prototype.GetRepeatTime = function(type)
{
	let repeatTime = 1000;

	if (this.template[type] && this.template[type].RepeatTime)
		repeatTime = +this.template[type].RepeatTime;

	return ApplyValueModificationsToEntity("Attack/" + type + "/RepeatTime", repeatTime, this.entity);
};

Attack.prototype.GetTimers = function(type)
{
	return {
		"prepare": ApplyValueModificationsToEntity("Attack/" + type + "/PrepareTime", +(this.template[type].PrepareTime || 0), this.entity),
		"repeat": this.GetRepeatTime(type)
	};
};

Attack.prototype.GetSplashData = function(type)
{
	if (!this.template[type].Splash)
		return undefined;

	return {
		"attackData": this.GetAttackEffectsData(type, true),
		"friendlyFire": this.template[type].Splash.FriendlyFire == "true",
		"radius": ApplyValueModificationsToEntity("Attack/" + type + "/Splash/Range", +this.template[type].Splash.Range, this.entity),
		"shape": this.template[type].Splash.Shape,
	};
};

Attack.prototype.GetRange = function(type)
{
	if (!type)
		return this.GetFullAttackRange();

	let max = +this.template[type].MaxRange;
	max = ApplyValueModificationsToEntity("Attack/" + type + "/MaxRange", max, this.entity);

	let min = +(this.template[type].MinRange || 0);
	min = ApplyValueModificationsToEntity("Attack/" + type + "/MinRange", min, this.entity);

	return { "max": max, "min": min };
};

Attack.prototype.GetAttackYOrigin = function(type)
{
	if (!this.template[type].Origin)
		return 0;
	return ApplyValueModificationsToEntity("Attack/" + type + "/Origin/Y", +this.template[type].Origin.Y, this.entity);
};

/**
 * @param {number} target - The target to attack.
 * @param {string} type - The type of attack to use.
 * @param {number} callerIID - The IID to notify on specific events.
 *
 * @return {boolean} - Whether we started attacking.
 */
Attack.prototype.StartAttacking = function(target, type, callerIID)
{
	if (this.target)
		this.StopAttacking();

	if (!this.CanAttack(target, [type]))
		return false;

	const cmpResistance = QueryMiragedInterface(target, IID_Resistance);
	if (!cmpResistance || !cmpResistance.AddAttacker(this.entity))
		return false;

	let timings = this.GetTimers(type);
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	// If the repeat time since the last attack hasn't elapsed,
	// delay the action to avoid attacking too fast.
	let prepare = timings.prepare;
	if (this.lastAttacked)
	{
		let repeatLeft = this.lastAttacked + timings.repeat - cmpTimer.GetTime();
		prepare = Math.max(prepare, repeatLeft);
	}

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
	{
		cmpVisual.SelectAnimation("attack_" + type.toLowerCase(), false, 1.0);
		cmpVisual.SetAnimationSyncRepeat(timings.repeat);
		cmpVisual.SetAnimationSyncOffset(prepare);
	}

	// If using a non-default prepare time, re-sync the animation when the timer runs.
	this.resyncAnimation = prepare != timings.prepare;
	this.target = target;
	this.callerIID = callerIID;
	this.timer = cmpTimer.SetInterval(this.entity, IID_Attack, "Attack", prepare, timings.repeat, type);

	return true;
};

/**
 * @param {string} reason - The reason why we stopped attacking.
 */
Attack.prototype.StopAttacking = function(reason)
{
	if (!this.target)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;

	const cmpResistance = QueryMiragedInterface(this.target, IID_Resistance);
	if (cmpResistance)
		cmpResistance.RemoveAttacker(this.entity);

	delete this.target;

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("idle", false, 1.0);

	// The callerIID component may start again,
	// replacing the callerIID, hence save that.
	let callerIID = this.callerIID;
	delete this.callerIID;

	if (reason && callerIID)
	{
		let component = Engine.QueryInterface(this.entity, callerIID);
		if (component)
			component.ProcessMessage(reason, null);
	}
};

/**
 * Attack our target entity.
 * @param {string} data - The attack type to use.
 * @param {number} lateness - The offset of the actual call and when it was expected.
 */
Attack.prototype.Attack = function(type, lateness)
{
	if (!this.CanAttack(this.target, [type]))
	{
		this.StopAttacking("TargetInvalidated");
		return;
	}

	// ToDo: Enable entities to keep facing a target.
	Engine.QueryInterface(this.entity, IID_UnitAI)?.FaceTowardsTarget(this.target);

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.lastAttacked = cmpTimer.GetTime() - lateness;

	// BuildingAI has its own attack routine.
	if (!Engine.QueryInterface(this.entity, IID_BuildingAI))
		this.PerformAttack(type, this.target);

	if (!this.target)
		return;

	// We check the range after the attack to facilitate chasing.
	if (!this.IsTargetInRange(this.target, type))
	{
		this.StopAttacking("OutOfRange");
		return;
	}

	if (this.resyncAnimation)
	{
		let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
		if (cmpVisual)
		{
			let repeat = this.GetTimers(type).repeat;
			cmpVisual.SetAnimationSyncRepeat(repeat);
			cmpVisual.SetAnimationSyncOffset(repeat);
		}
		delete this.resyncAnimation;
	}
};

/**
 * Attack the target entity. This should only be called after a successful range check,
 * and should only be called after GetTimers().repeat msec has passed since the last
 * call to PerformAttack.
 */
Attack.prototype.PerformAttack = function(type, target)
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	let selfPosition = cmpPosition.GetPosition();

	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return;
	let targetPosition = cmpTargetPosition.GetPosition();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return;
	let attackerOwner = cmpOwnership.GetOwner();

	let data = {
		"type": type,
		"attackData": this.GetAttackEffectsData(type),
		"splash": this.GetSplashData(type),
		"attacker": this.entity,
		"attackerOwner": attackerOwner,
		"target": target,
	};

	let delay = +(this.template[type].EffectDelay || 0);

	if (this.template[type].Projectile)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		let turnLength = cmpTimer.GetLatestTurnLength()/1000;
		// In the future this could be extended:
		//  * Obstacles like trees could reduce the probability of the target being hit
		//  * Obstacles like walls should block projectiles entirely

		let horizSpeed = +this.template[type].Projectile.Speed;
		let gravity = +this.template[type].Projectile.Gravity;
		// horizSpeed /= 2; gravity /= 2; // slow it down for testing

		// We will try to estimate the position of the target, where we can hit it.
		// We first estimate the time-till-hit by extrapolating linearly the movement
		// of the last turn. We compute the time till an arrow will intersect the target.
		let targetVelocity = Vector3D.sub(targetPosition, cmpTargetPosition.GetPreviousPosition()).div(turnLength);

		let timeToTarget = PositionHelper.PredictTimeToTarget(selfPosition, horizSpeed, targetPosition, targetVelocity);

		// 'Cheat' and use UnitMotion to predict the position in the near-future.
		// This avoids 'dancing' issues with units zigzagging over very short distances.
		// However, this could fail if the player gives several short move orders, so
		// occasionally fall back to basic interpolation.
		let predictedPosition = targetPosition;
		if (timeToTarget !== false)
		{
			// Don't predict too far in the future, but avoid threshold effects.
			// After 1 second, always use the 'dumb' interpolated past-motion prediction.
			let useUnitMotion = randBool(Math.max(0, 0.75 - timeToTarget / 1.333));
			if (useUnitMotion)
			{
				let cmpTargetUnitMotion = Engine.QueryInterface(target, IID_UnitMotion);
				let cmpTargetUnitAI = Engine.QueryInterface(target, IID_UnitAI);
				if (cmpTargetUnitMotion && (!cmpTargetUnitAI || !cmpTargetUnitAI.IsFormationMember()))
				{
					let pos2D = cmpTargetUnitMotion.EstimateFuturePosition(timeToTarget);
					predictedPosition.x = pos2D.x;
					predictedPosition.z = pos2D.y;
				}
				else
					predictedPosition = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);
			}
			else
				predictedPosition = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);
		}

		let predictedHeight = cmpTargetPosition.GetHeightAt(predictedPosition.x, predictedPosition.z);

		// Add inaccuracy based on spread.
		let distanceModifiedSpread = ApplyValueModificationsToEntity("Attack/" + type + "/Spread", +this.template[type].Projectile.Spread, this.entity) *
			predictedPosition.horizDistanceTo(selfPosition) / 100;

		let randNorm = randomNormal2D();
		let offsetX = randNorm[0] * distanceModifiedSpread;
		let offsetZ = randNorm[1] * distanceModifiedSpread;

		data.position = new Vector3D(predictedPosition.x + offsetX, predictedHeight, predictedPosition.z + offsetZ);

		let realHorizDistance = data.position.horizDistanceTo(selfPosition);
		timeToTarget = realHorizDistance / horizSpeed;
		delay += timeToTarget * 1000;

		data.direction = Vector3D.sub(data.position, selfPosition).div(realHorizDistance);

		let actorName = this.template[type].Projectile.ActorName || "";
		let impactActorName = this.template[type].Projectile.ImpactActorName || "";
		let impactAnimationLifetime = this.template[type].Projectile.ImpactAnimationLifetime || 0;

		// TODO: Use unit rotation to implement x/z offsets.
		let deltaLaunchPoint = new Vector3D(0, +this.template[type].Projectile.LaunchPoint["@y"], 0);
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

		let cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);
		data.projectileId = cmpProjectileManager.LaunchProjectileAtPoint(launchPoint, data.position, horizSpeed, gravity, actorName, impactActorName, impactAnimationLifetime);

		let cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
		data.attackImpactSound = cmpSound ? cmpSound.GetSoundGroup("attack_impact_" + type.toLowerCase()) : "";

		data.friendlyFire = this.template[type].Projectile.FriendlyFire == "true";
	}
	else
	{
		data.position = targetPosition;
		data.direction = Vector3D.sub(targetPosition, selfPosition);
	}
	if (delay)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_DelayedDamage, "Hit", delay, data);
	}
	else
		Engine.QueryInterface(SYSTEM_ENTITY, IID_DelayedDamage).Hit(data, 0);
};

/**
 * @param {number} - The entity ID of the target to check.
 * @return {boolean} - Whether this entity is in range of its target.
 */
Attack.prototype.IsTargetInRange = function(target, type)
{
	const range = this.GetRange(type);
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager).IsInTargetParabolicRange(
		this.entity,
		target,
		range.min,
		range.max,
		this.GetAttackYOrigin(type),
		false);
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

Attack.prototype.GetRangeOverlays = function(type = "Ranged")
{
	if (!this.template[type] || !this.template[type].RangeOverlay)
		return [];

	let range = this.GetRange(type);
	let rangeOverlays = [];
	for (let i in range)
		if ((i == "min" || i == "max") && range[i])
			rangeOverlays.push({
				"radius": range[i],
				"texture": this.template[type].RangeOverlay.LineTexture,
				"textureMask": this.template[type].RangeOverlay.LineTextureMask,
				"thickness": +this.template[type].RangeOverlay.LineThickness,
			});
	return rangeOverlays;
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
