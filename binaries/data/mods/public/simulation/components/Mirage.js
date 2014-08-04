const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Mirage() {}

Mirage.prototype.Schema =
	"<a:help>Mirage entities replace real entities in the fog-of-war.</a:help>" +
	"<empty/>";

Mirage.prototype.Init = function()
{
	this.player = null;
	this.parent = INVALID_ENTITY;

	this.foundation = false;
	this.buildPercentage = null;

	this.health = false;
	this.maxHitpoints = null;
	this.hitpoints = null;
	this.needsRepair = null;

	this.resourceSupply = false;
	this.maxAmount = null;
	this.amount = null;
	this.type = null;
	this.isInfinite = null;
};

Mirage.prototype.SetParent = function(ent)
{
	this.parent = ent;
};

Mirage.prototype.GetPlayer = function()
{
	return this.player;
};

Mirage.prototype.SetPlayer = function(player)
{
	this.player = player;
};

// ============================
// Parent entity data

// Foundation data

Mirage.prototype.AddFoundation = function(buildPercentage)
{
	this.foundation = true;
	this.buildPercentage = buildPercentage;
};

Mirage.prototype.Foundation = function()
{
	return this.foundation;
};

Mirage.prototype.GetBuildPercentage = function()
{
	return this.buildPercentage;
};

// Health data

Mirage.prototype.AddHealth = function(maxHitpoints, hitpoints, needsRepair)
{
	this.health = true;
	this.maxHitpoints = maxHitpoints;
	this.hitpoints = Math.ceil(hitpoints);
	this.needsRepair = needsRepair;
};

Mirage.prototype.Health = function()
{
	return this.health;
};

Mirage.prototype.GetMaxHitpoints = function()
{
	return this.maxHitpoints;
};

Mirage.prototype.GetHitpoints = function()
{
	return this.hitpoints;
};

Mirage.prototype.NeedsRepair = function()
{
	return this.needsRepair;
};

// ResourceSupply data

Mirage.prototype.AddResourceSupply = function(maxAmount, amount, type, isInfinite)
{
	this.resourceSupply = true;
	this.maxAmount = maxAmount;
	this.amount = amount;
	this.type = type;
	this.isInfinite = isInfinite;
};

Mirage.prototype.ResourceSupply = function()
{
	return this.resourceSupply;
};

Mirage.prototype.GetMaxAmount = function()
{
	return this.maxAmount;
};

Mirage.prototype.GetAmount = function()
{
	return this.amount;
};

Mirage.prototype.GetType = function()
{
	return this.type;
};

Mirage.prototype.IsInfinite = function()
{
	return this.isInfinite;
};

// ============================

Mirage.prototype.OnVisibilityChanged = function(msg)
{
	if (msg.player == this.player && msg.newVisibility == VIS_VISIBLE && this.parent == INVALID_ENTITY)
		Engine.DestroyEntity(this.entity);
};

Mirage.prototype.OnDestroy = function(msg)
{
	if (this.parent == INVALID_ENTITY)
		return;

	Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: this.parent });
};

Engine.RegisterComponentType(IID_Mirage, "Mirage", Mirage);
