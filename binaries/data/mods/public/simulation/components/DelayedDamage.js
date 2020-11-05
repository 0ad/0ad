function DelayedDamage() {}

DelayedDamage.prototype.Schema =
	"<a:component type='system'/><empty/>";

DelayedDamage.prototype.Init = function()
{
};

/**
 * When missiles miss their target, other units in MISSILE_HIT_RADIUS range are considered.
 * Large missiles should probably implement splash damage anyways,
 * so keep this value low for performance.
 */
DelayedDamage.prototype.MISSILE_HIT_RADIUS = 2;

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
 * @param {boolean}  data.friendlyFire - A flag indicating whether allied entities can also be damaged.
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

	// Do this first in case the direct hit kills the target.
	if (data.splash)
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

	let cmpProjectileManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ProjectileManager);

	let target = data.target;
	// Since we can't damage mirages, replace a miraged target by the real target.
	let cmpMirage = Engine.QueryInterface(data.target, IID_Mirage);
	if (cmpMirage)
		target = cmpMirage.GetParent();

	// Deal direct damage if we hit the main target
	// and we could handle the attack.
	if (PositionHelper.TestCollision(target, data.position, lateness) &&
		Attacking.HandleAttackEffects(target, data.type, data.attackData, data.attacker, data.attackerOwner))
	{
		cmpProjectileManager.RemoveProjectile(data.projectileId);
		return;
	}

	// If we didn't hit the main target look for nearby units.
	let ents = PositionHelper.EntitiesNearPoint(Vector2D.from3D(data.position), this.MISSILE_HIT_RADIUS,
		Attacking.GetPlayersToDamage(data.attackerOwner, data.friendlyFire));

	for (let ent of ents)
	{
		if (!PositionHelper.TestCollision(ent, data.position, lateness) ||
			!Attacking.HandleAttackEffects(ent, data.type, data.attackData, data.attacker, data.attackerOwner))
			continue;

		cmpProjectileManager.RemoveProjectile(data.projectileId);
		break;
	}
};

Engine.RegisterSystemComponentType(IID_DelayedDamage, "DelayedDamage", DelayedDamage);
