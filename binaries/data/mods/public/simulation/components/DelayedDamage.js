function DelayedDamage() {}

DelayedDamage.prototype.Schema =
	"<a:component type='system'/><empty/>";

DelayedDamage.prototype.Init = function()
{
};

/**
 * Handles hit logic after the projectile travel time has passed.
 * @param {Object}   data - The data sent by the caller.
 * @param {string}   data.type - The type of damage.
 * @param {Object}   data.attackData - Data of the form { 'effectType': { ...opaque effect data... }, 'Bonuses': {...} }.
 * @param {number}   data.target - The entity id of the target.
 * @param {number}   data.attacker - The entity id of the attacker.
 * @param {number}   data.attackerOwner - The player id of the owner of the attacker.
 * @param {Vector2D} data.origin - The origin of the projectile hit.
 * @param {Vector3D} data.position - The expected position of the target.
 * @param {number}   data.projectileId - The id of the projectile.
 * @param {Vector3D} data.direction - The unit vector defining the direction.
 * @param {string}   data.attackImpactSound - The name of the sound emited on impact.
 * ***When splash damage***
 * @param {boolean}  data.splash.friendlyFire - A flag indicating if allied entities are also damaged.
 * @param {number}   data.splash.radius - The radius of the splash damage.
 * @param {string}   data.splash.shape - The shape of the splash range.
 * @param {Object}   data.splash.attackData - same as attackData, for splash.
 */
DelayedDamage.prototype.MissileHit = function(data, lateness)
{
	if (!data.position)
		return;

	let cmpSoundManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager);
	if (cmpSoundManager && data.attackImpactSound)
		cmpSoundManager.PlaySoundGroupAtPosition(data.attackImpactSound, data.position);

	// Do this first in case the direct hit kills the target
	if (data.splash)
	{
		Attacking.CauseDamageOverArea({
			"type": data.type,
			"attackData": data.splash.attackData,
			"attacker": data.attacker,
			"attackerOwner": data.attackerOwner,
			"origin": Vector2D.from3D(data.position),
			"radius": data.splash.radius,
			"shape": data.splash.shape,
			"direction": data.direction,
			"friendlyFire": data.splash.friendlyFire
		});
	}

	let cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);

	// Deal direct damage if we hit the main target
	// and if the target has Resistance (not the case for a mirage for example)
	if (Attacking.TestCollision(data.target, data.position, lateness))
	{
		cmpProjectileManager.RemoveProjectile(data.projectileId);

		Attacking.HandleAttackEffects(data.type, data.attackData, data.target, data.attacker, data.attackerOwner);
		return;
	}

	let targetPosition = Attacking.InterpolatedLocation(data.target, lateness);
	if (!targetPosition)
		return;

	// If we didn't hit the main target look for nearby units
	let cmpPlayer = QueryPlayerIDInterface(data.attackerOwner);
	let ents = Attacking.EntitiesNearPoint(Vector2D.from3D(data.position), targetPosition.horizDistanceTo(data.position) * 2, cmpPlayer.GetEnemies());
	for (let ent of ents)
	{
		if (!Attacking.TestCollision(ent, data.position, lateness))
			continue;

		Attacking.HandleAttackEffects(data.type, data.attackData, ent, data.attacker, data.attackerOwner);

		cmpProjectileManager.RemoveProjectile(data.projectileId);
		break;
	}
};

Engine.RegisterSystemComponentType(IID_DelayedDamage, "DelayedDamage", DelayedDamage);
