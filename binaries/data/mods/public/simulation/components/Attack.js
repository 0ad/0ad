function Attack() {}

Attack.prototype.Init = function()
{
};

/*
 * TODO: to handle secondary attacks in the future, what we might do is
 * add a 'mode' parameter to most of these functions, to indicate which
 * attack mode we're trying to use, and some other function that allows
 * UnitAI to pick the best attack mode (based on range, damage, etc)
 */

Attack.prototype.GetTimers = function()
{
	var prepare = +(this.template.PrepareTime || 0);
	var repeat = +(this.template.RepeatTime || 1000);
	return { "prepare": prepare, "repeat": repeat, "recharge": repeat - prepare };
};

Attack.prototype.GetAttackStrengths = function()
{
	// Convert attack values to numbers, default 0 if unspecified
	return {
		hack: +(this.template.Hack || 0),
		pierce: +(this.template.Pierce || 0),
		crush: +(this.template.Crush || 0)
	};
};

Attack.prototype.GetRange = function()
{
	var max = +this.template.Range;
	var min = +(this.template.MinRange || 0);
	return { "max": max, "min": min };
}

/**
 * Attack the target entity. This should only be called after a successful range check,
 * and should only be called after GetTimers().repeat msec has passed since the last
 * call to PerformAttack.
 */
Attack.prototype.PerformAttack = function(target)
{
	// If this is a ranged attack, then launch a projectile
	if (this.template.ProjectileSpeed)
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
		var horizSpeed = +this.template.ProjectileSpeed;
		var gravity = 9.81; // this affects the shape of the curve; assume it's constant for now
		var accuracy = 6; // TODO: get from entity template

		//horizSpeed /= 8; gravity /= 8; // slow it down for testing

		// Find the distance to the target
		var selfPosition = Engine.QueryInterface(this.entity, IID_Position).GetPosition();
		var targetPosition = Engine.QueryInterface(target, IID_Position).GetPosition();
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
			cmpTimer.SetTimeout(this.entity, IID_Attack, "CauseDamage", timeToTarget*1000, target);
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
		this.CauseDamage(target);
	}
};


// Inflict damage on the target
Attack.prototype.CauseDamage = function(target)
{
	var strengths = this.GetAttackStrengths();

	var cmpDamageReceiver = Engine.QueryInterface(target, IID_DamageReceiver);
	if (!cmpDamageReceiver)
		return;
	cmpDamageReceiver.TakeDamage(strengths.hack, strengths.pierce, strengths.crush);
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
