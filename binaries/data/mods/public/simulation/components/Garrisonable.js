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
	return true;
};

/**
 * Resets the garrisonHolder.
 */
Garrisonable.prototype.UnGarrison = function()
{
	delete this.holder;
};

Engine.RegisterComponentType(IID_Garrisonable, "Garrisonable", Garrisonable);
