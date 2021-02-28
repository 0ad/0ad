function Garrisonable() {}

Garrisonable.prototype.Schema = "<empty/>";

Garrisonable.prototype.Init = function()
{
};

/**
 * @return {number} - The entity ID of the entity this entity is garrisoned in.
 */
Garrisonable.prototype.HolderID = function()
{
	return this.holder || INVALID_ENTITY;
};

/**
 * @param {number} entity - The entity ID of the entity this entity is being garrisoned in.
 * @return {boolean} - Whether garrisoning succeeded.
 */
Garrisonable.prototype.Garrison = function(entity)
{
	if (this.holder)
		return false;

	this.holder = entity;

	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.PauseProduction();

	let cmpAura = Engine.QueryInterface(this.entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.ApplyGarrisonAura(entity);

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
		cmpPosition.MoveOutOfWorld();

	return true;
};

/**
 * @param {boolean} forced - Optionally whether the spawning is forced.
 * @return {boolean} - Whether the ungarrisoning succeeded.
 */
Garrisonable.prototype.UnGarrison = function(forced)
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
	{
		let pos;
		let cmpGarrisonHolder = Engine.QueryInterface(this.holder, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
			pos = cmpGarrisonHolder.GetSpawnPosition(this.entity, forced);

		if (!pos)
			return false;

		cmpPosition.JumpTo(pos.x, pos.z);
		cmpPosition.SetHeightOffset(0);

		let cmpHolderPosition = Engine.QueryInterface(this.holder, IID_Position);
		if (cmpHolderPosition)
			cmpPosition.SetYRotation(cmpHolderPosition.GetPosition().horizAngleTo(pos));
	}

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.Ungarrison();

	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.UnpauseProduction();

	let cmpAura = Engine.QueryInterface(this.entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.RemoveGarrisonAura(this.holder);

	delete this.holder;
	return true;
};

Engine.RegisterComponentType(IID_Garrisonable, "Garrisonable", Garrisonable);
