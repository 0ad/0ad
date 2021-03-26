function Turretable() {}

Turretable.prototype.Schema =
	"<empty/>";

Turretable.prototype.Init = function()
{
};

/**
 * @return {number} - The entity ID of the entity this entity is turreted on.
 */
Turretable.prototype.HolderID = function()
{
	return this.holder || INVALID_ENTITY;
};

/**
 * @return {boolean} - Whether we're turreted.
 */
Turretable.prototype.IsTurreted = function()
{
	return !!this.holder;
};

/**
 * @param {number} target - The entity ID to check.
 * @return {boolean} - Whether we can occupy the turret.
 */
Turretable.prototype.CanOccupy = function(target)
{
	if (this.holder)
		return false;

	let cmpTurretHolder = Engine.QueryInterface(target, IID_TurretHolder);
	return cmpTurretHolder && cmpTurretHolder.CanOccupy(this.entity);
};

/**
 * @param {number} target - The entity ID of the entity this entity is being turreted on.
 * @param {string} turretPointName - Optionally the turret point name to occupy.
 * @return {boolean} - Whether occupying succeeded.
 */
Turretable.prototype.OccupyTurret = function(target, turretPointName = "")
{
	if (!this.CanOccupy(target))
		return false;

	let cmpTurretHolder = Engine.QueryInterface(target, IID_TurretHolder);
	if (!cmpTurretHolder || !cmpTurretHolder.OccupyNamedTurret(this.entity, turretPointName))
		return false;

	this.holder = target;

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
	{
		cmpUnitAI.SetGarrisoned();
		cmpUnitAI.SetTurretStance();
	}

	let cmpUnitMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotion)
		cmpUnitMotion.SetFacePointAfterMove(false);

	// Remove the unit's obstruction to avoid interfering with pathing.
	let cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction)
		cmpObstruction.SetActive(false);

	Engine.PostMessage(this.entity, MT_TurretedStateChanged, {
		"oldHolder": INVALID_ENTITY,
		"holderID": target
	});

	return true;
};

/**
 * @param {boolean} forced - Optionally whether the leaving the turret is forced.
 * @return {boolean} - Whether leaving the turret succeeded.
 */
Turretable.prototype.LeaveTurret = function(forced = false)
{
	if (!this.holder)
		return true;

	let pos = PositionHelper.GetSpawnPosition(this.holder, this.entity, forced);
	if (!pos)
		return false;

	let cmpTurretHolder = Engine.QueryInterface(this.holder, IID_TurretHolder);
	if (!cmpTurretHolder || !cmpTurretHolder.LeaveTurret(this.entity))
		return false;

	let cmpUnitMotionEntity = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpUnitMotionEntity)
		cmpUnitMotionEntity.SetFacePointAfterMove(true);

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
	{
		cmpPosition.JumpTo(pos.x, pos.z);
		cmpPosition.SetHeightOffset(0);
	}

	let cmpHolderPosition = Engine.QueryInterface(this.holder, IID_Position);
	if (cmpHolderPosition)
		cmpPosition.SetYRotation(cmpHolderPosition.GetPosition().horizAngleTo(pos));

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
	{
		cmpUnitAI.Ungarrison();
		cmpUnitAI.UnsetGarrisoned();
		cmpUnitAI.ResetTurretStance();
	}

	// Reset the obstruction flags to template defaults.
	let cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction)
		cmpObstruction.SetActive(true);

	Engine.PostMessage(this.entity, MT_TurretedStateChanged, {
		"oldHolder": this.holder,
		"holderID": INVALID_ENTITY
	});

	let cmpRallyPoint = Engine.QueryInterface(this.holder, IID_RallyPoint);
	if (cmpRallyPoint)
		cmpRallyPoint.OrderToRallyPoint(this.entity, ["occupy-turret"]);

	delete this.holder;
	return true;
};

Turretable.prototype.OnEntityRenamed = function(msg)
{
	if (!this.holder)
		return;

	let cmpTurretHolder = Engine.QueryInterface(this.holder, IID_TurretHolder);
	if (!cmpTurretHolder)
		return;

	let holder = this.holder;
	let currentPoint = cmpTurretHolder.GetOccupiedTurretName(this.entity);
	this.LeaveTurret(true);
	let cmpTurretableNew = Engine.QueryInterface(msg.newentity, IID_Turretable);
	if (cmpTurretableNew)
		cmpTurretableNew.OccupyTurret(holder, currentPoint);
};

Turretable.prototype.OnOwnershipChanged = function(msg)
{
	if (!this.holder)
		return;

	if (msg.to == INVALID_PLAYER)
		this.LeaveTurret(true);
	else if (!IsOwnedByMutualAllyOfEntity(this.entity, this.holder))
		this.LeaveTurret();
};

Engine.RegisterComponentType(IID_Turretable, "Turretable", Turretable);
