function Garrisonable() {}

Garrisonable.prototype.Schema =
	"<a:help>Controls the garrisonability of an entity.</a:help>" +
	"<a:example>" +
		"<Size>10</Size>" +
	"</a:example>" +
	"<element name='Size' a:help='Number of garrison slots the entity occupies.'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>";

Garrisonable.prototype.Init = function()
{
};

/**
 * @return {number} - The number of slots this unit takes in a garrisonHolder.
 */
Garrisonable.prototype.UnitSize = function()
{
	return ApplyValueModificationsToEntity("Garrisonable/Size", +this.template.Size, this.entity);
};

/**
 * Calculates the number of slots this unit takes in a garrisonHolder by
 * adding the number of garrisoned slots to the equation.
 *
 * @return {number} - The number of slots this unit and its garrison takes in a garrisonHolder.
 */
Garrisonable.prototype.TotalSize = function()
{
	let size = this.UnitSize();
	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
		size += cmpGarrisonHolder.OccupiedSlots();
	return size;
};

/**
 * @return {number} - The entity ID of the entity this entity is garrisoned in.
 */
Garrisonable.prototype.HolderID = function()
{
	return this.holder || INVALID_ENTITY;
};

/**
 * @param {number} target - The entity ID to check.
 * @return {boolean} - Whether we can garrison.
 */
Garrisonable.prototype.CanGarrison = function(target)
{
	if (this.holder)
		return false;

	let cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	return cmpGarrisonHolder && cmpGarrisonHolder.IsAllowedToGarrison(this.entity);
};

/**
 * @param {number} target - The entity ID of the entity this entity is being garrisoned in.
 * @return {boolean} - Whether garrisoning succeeded.
 */
Garrisonable.prototype.Garrison = function(target, renamed = false)
{
	if (!this.CanGarrison(target))
		return false;

	let cmpGarrisonHolder = Engine.QueryInterface(target, IID_GarrisonHolder);
	if (!cmpGarrisonHolder || !cmpGarrisonHolder.Garrison(this.entity))
		return false;

	this.holder = target;

	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.PauseProduction();

	let cmpAura = Engine.QueryInterface(this.entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.ApplyGarrisonAura(target);

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
		cmpPosition.MoveOutOfWorld();

	if (renamed)
		return true;

	let cmpTurretHolder = Engine.QueryInterface(target, IID_TurretHolder);
	if (cmpTurretHolder)
		cmpTurretHolder.OccupyTurret(this.entity);

	return true;
};

/**
 * Called on game init when the entity was part of init garrison.
 * @param {number} target - The entityID to autogarrison.
 * @return {boolean} - Whether garrisoning succeeded.
 */
Garrisonable.prototype.Autogarrison = function(target)
{
	if (!this.Garrison(target))
		return false;

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.Autogarrison(target);
	return true;
};

/**
 * @param {boolean} forced - Optionally whether the spawning is forced.
 * @param {boolean} renamed - Optionally whether the ungarrisoning is due to renaming.
 * @return {boolean} - Whether the ungarrisoning succeeded.
 */
Garrisonable.prototype.UnGarrison = function(forced = false, renamed = false)
{
	if (!this.holder)
		return true;

	let cmpGarrisonHolder = Engine.QueryInterface(this.holder, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return false;

	let pos = cmpGarrisonHolder.GetSpawnPosition(this.entity, forced);
	if (!pos)
		return false;

	if (!cmpGarrisonHolder.Eject(this.entity, forced))
		return false;

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
		cmpUnitAI.Ungarrison();

	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.UnpauseProduction();

	let cmpAura = Engine.QueryInterface(this.entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.RemoveGarrisonAura(this.holder);

	if (renamed)
		return true;

	let cmpTurretHolder = Engine.QueryInterface(this.holder, IID_TurretHolder);
	if (cmpTurretHolder)
		cmpTurretHolder.LeaveTurret(this.entity);

	cmpGarrisonHolder.OrderToRallyPoint(this.entity);

	delete this.holder;
	return true;
};

Garrisonable.prototype.OnEntityRenamed = function(msg)
{
	if (!this.holder)
		return;

	let cmpGarrisonHolder = Engine.QueryInterface(this.holder, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		this.UnGarrison(true, true);
		let cmpGarrisonable = Engine.QueryInterface(msg.newentity, IID_Garrisonable);
		if (cmpGarrisonable)
			cmpGarrisonable.Garrison(this.holder, true);
	}

	// We process EntityRenamed of turrets seperately since we
	// want to occupy the same position after being renamed.
	let cmpTurretHolder = Engine.QueryInterface(this.holder, IID_TurretHolder);
	if (cmpTurretHolder)
		cmpTurretHolder.SwapEntities(msg.entity, msg.newentity);

	delete this.holder;
};

Engine.RegisterComponentType(IID_Garrisonable, "Garrisonable", Garrisonable);
