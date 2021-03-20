const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Mirage() {}

Mirage.prototype.Schema =
	"<a:help>Mirage entities replace real entities in the fog-of-war.</a:help>" +
	"<empty/>";

Mirage.prototype.Init = function()
{
	this.parent = INVALID_ENTITY;
	this.player = null;

	this.miragedIids = new Map();
};

Mirage.prototype.SetParent = function(ent)
{
	this.parent = ent;
};

Mirage.prototype.GetParent = function()
{
	return this.parent;
};

Mirage.prototype.SetPlayer = function(player)
{
	this.player = player;
};

Mirage.prototype.GetPlayer = function()
{
	return this.player;
};

Mirage.prototype.Mirages = function(iid)
{
	return this.miragedIids.has(iid);
};

Mirage.prototype.Get = function(iid)
{
	return this.miragedIids.get(iid);
};

// ============================
// Parent entity data

Mirage.prototype.CopyCapturable = function(cmpCapturable)
{
	this.miragedIids.set(IID_Capturable, cmpCapturable.Mirage());
};

Mirage.prototype.CopyFoundation = function(cmpFoundation)
{
	this.miragedIids.set(IID_Foundation, cmpFoundation.Mirage());
};

Mirage.prototype.CopyHealth = function(cmpHealth)
{
	this.miragedIids.set(IID_Health, cmpHealth.Mirage());
};

Mirage.prototype.CopyIdentity = function(cmpIdentity)
{
	this.miragedIids.set(IID_Identity, cmpIdentity.Mirage());
};

Mirage.prototype.CopyMarket = function(cmpMarket)
{
	this.miragedIids.set(IID_Market, cmpMarket.Mirage(this.entity, this.player));
};

Mirage.prototype.CopyRepairable = function(cmpRepairable)
{
	this.miragedIids.set(IID_Repairable, cmpRepairable.Mirage());
};

Mirage.prototype.CopyResourceSupply = function(cmpResourceSupply)
{
	this.miragedIids.set(IID_ResourceSupply, cmpResourceSupply.Mirage());
};

// ============================

Mirage.prototype.OnVisibilityChanged = function(msg)
{
	// Mirages get VIS_HIDDEN when the original entity becomes VIS_VISIBLE.
	if (msg.player != this.player || msg.newVisibility != VIS_HIDDEN)
		return;

	if (this.miragedIids.has(IID_Market))
		this.miragedIids.get(IID_Market).UpdateTraders(msg);

	if (this.parent == INVALID_ENTITY)
		Engine.DestroyEntity(this.entity);
	else
		Engine.PostMessage(this.entity, MT_EntityRenamed, { "entity": this.entity, "newentity": this.parent });
};

Engine.RegisterComponentType(IID_Mirage, "Mirage", Mirage);
