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

function MiragedIdentity(cmpIdentity)
{
	// Mirages don't get identity classes via the template-filter, so that code can query
	// identity components via Engine.QueryInterface without having to explicitly check for mirages.
	// This is cloned as otherwise we get a reference to Identity's property,
	// and that array is deleted when serializing (as it's not seralized), which ends in OOS.
	this.classes = clone(cmpIdentity.GetClassesList());
};

MiragedIdentity.prototype.GetClassesList = function() { return this.classes; };

Mirage.prototype.CopyIdentity = function(cmpIdentity)
{
	this.miragedIids.set(IID_Identity, new MiragedIdentity(cmpIdentity));
};

// Foundation data

function MiragedFoundation(cmpFoundation)
{
	this.numBuilders = cmpFoundation.GetNumBuilders();
	this.buildTime = cmpFoundation.GetBuildTime();
};

MiragedFoundation.prototype.GetNumBuilders = function() { return this.numBuilders; };
MiragedFoundation.prototype.GetBuildTime = function() { return this.buildTime; };

Mirage.prototype.CopyFoundation = function(cmpFoundation)
{
	this.miragedIids.set(IID_Foundation, new MiragedFoundation(cmpFoundation));
};

// Repairable data

function MiragedRepairable(cmpRepairable)
{
	this.numBuilders = cmpRepairable.GetNumBuilders();
	this.buildTime = cmpRepairable.GetBuildTime();
};

MiragedRepairable.prototype.GetNumBuilders = function() { return this.numBuilders; };
MiragedRepairable.prototype.GetBuildTime = function() { return this.buildTime; };

Mirage.prototype.CopyRepairable = function(cmpRepairable)
{
	this.miragedIids.set(IID_Repairable, new MiragedRepairable(cmpRepairable));
};

// Health data

function MiragedHealth(cmpHealth)
{
	this.maxHitpoints = cmpHealth.GetMaxHitpoints();
	this.hitpoints = cmpHealth.GetHitpoints();
	this.repairable = cmpHealth.IsRepairable();
	this.injured = cmpHealth.IsInjured();
	this.unhealable = cmpHealth.IsUnhealable();
};

MiragedHealth.prototype.GetMaxHitpoints = function() { return this.maxHitpoints; };
MiragedHealth.prototype.GetHitpoints = function() { return this.hitpoints; };
MiragedHealth.prototype.IsRepairable = function() { return this.repairable; };
MiragedHealth.prototype.IsInjured = function() { return this.injured; };
MiragedHealth.prototype.IsUnhealable = function() { return this.unhealable; };

Mirage.prototype.CopyHealth = function(cmpHealth)
{
	this.miragedIids.set(IID_Health, new MiragedHealth(cmpHealth));
};

// Capture data

function MiragedCapture(cmpCapturable)
{
	this.capturePoints = clone(cmpCapturable.GetCapturePoints());
	this.maxCapturePoints = cmpCapturable.GetMaxCapturePoints();
	this.CanCapture = cmpCapturable.CanCapture;
};

MiragedCapture.prototype.GetCapturePoints = function() { return this.capturePoints; };
MiragedCapture.prototype.GetMaxCapturePoints = function() { return this.maxCapturePoints; };

Mirage.prototype.CopyCapturable = function(cmpCapturable)
{
	this.miragedIids.set(IID_Capturable, new MiragedCapture(cmpCapturable));
};

// ResourceSupply data
function MiragedResourceSupply(cmpResourceSupply)
{
	this.maxAmount = cmpResourceSupply.GetMaxAmount();
	this.amount = cmpResourceSupply.GetCurrentAmount();
	this.type = cmpResourceSupply.GetType();
	this.isInfinite = cmpResourceSupply.IsInfinite();
	this.killBeforeGather = cmpResourceSupply.GetKillBeforeGather();
	this.maxGatherers = cmpResourceSupply.GetMaxGatherers();
	this.numGatherers = cmpResourceSupply.GetNumGatherers();
};

MiragedResourceSupply.prototype.GetMaxAmount = function() { return this.maxAmount; };
MiragedResourceSupply.prototype.GetCurrentAmount = function() { return this.amount; };
MiragedResourceSupply.prototype.GetType = function() { return this.type; };
MiragedResourceSupply.prototype.IsInfinite = function() { return this.isInfinite; };
MiragedResourceSupply.prototype.GetKillBeforeGather = function() { return this.killBeforeGather; };
MiragedResourceSupply.prototype.GetMaxGatherers = function() { return this.maxGatherers; };
MiragedResourceSupply.prototype.GetNumGatherers = function() { return this.numGatherers; };

Mirage.prototype.CopyResourceSupply = function(cmpResourceSupply)
{
	this.miragedIids.set(IID_ResourceSupply, new MiragedResourceSupply(cmpResourceSupply));
};

// Market data
function MiragedMarket(cmpMarket, entity, parent, player)
{
	this.entity = entity;
	this.parent = parent;
	this.player = player;

	this.traders = new Set();
	for (let trader of cmpMarket.GetTraders())
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		let cmpOwnership = Engine.QueryInterface(trader, IID_Ownership);
		if (!cmpTrader || !cmpOwnership)
		{
			cmpMarket.RemoveTrader(trader);
			continue;
		}
		if (this.player != cmpOwnership.GetOwner())
			continue;
		cmpTrader.SwitchMarket(cmpMarket.entity, this.entity);
		cmpMarket.RemoveTrader(trader);
		this.AddTrader(trader);
	}
	this.marketType = cmpMarket.GetType();
	this.internationalBonus = cmpMarket.GetInternationalBonus();
};

MiragedMarket.prototype.HasType = function(type) { return this.marketType.has(type); };
MiragedMarket.prototype.GetInternationalBonus = function() { return this.internationalBonus; };
MiragedMarket.prototype.AddTrader = function(trader) { this.traders.add(trader); };
MiragedMarket.prototype.RemoveTrader = function(trader) { this.traders.delete(trader); };

MiragedMarket.prototype.UpdateTraders = function(msg)
{
	let cmpMarket = Engine.QueryInterface(this.parent, IID_Market);
	if (!cmpMarket)	// The parent market does not exist anymore
	{
		for (let trader of this.traders)
		{
			let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
			if (cmpTrader)
				cmpTrader.RemoveMarket(this.entity);
		}
		return;
	}

	// The market becomes visible, switch all traders from the mirage to the market
	for (let trader of this.traders)
	{
		let cmpTrader = Engine.QueryInterface(trader, IID_Trader);
		if (!cmpTrader)
			continue;
		cmpTrader.SwitchMarket(this.entity, cmpMarket.entity);
		this.RemoveTrader(trader);
		cmpMarket.AddTrader(trader);
	}
};

Mirage.prototype.CopyMarket = function(cmpMarket)
{
	this.miragedIids.set(IID_Market, new MiragedMarket(cmpMarket, this.entity, this.parent, this.player));
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
