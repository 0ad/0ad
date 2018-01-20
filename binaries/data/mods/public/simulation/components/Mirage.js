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

	this.miragedIids = new Set();

	this.classesList = [];

	this.buildPercentage = 0;
	this.numBuilders = 0;

	this.maxHitpoints = null;
	this.hitpoints = null;
	this.repairable = null;
	this.unhealable = null;

	this.capturePoints = [];
	this.maxCapturePoints = 0;

	this.maxAmount = null;
	this.amount = null;
	this.type = null;
	this.isInfinite = null;
	this.killBeforeGather = null;
	this.maxGatherers = null;
	this.numGatherers = null;

	this.traders = null;
	this.marketType = null;
	this.internationalBonus = null;
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

Mirage.prototype.Mirages = function(iid)
{
	return this.miragedIids.has(iid);
};

// ============================
// Parent entity data

Mirage.prototype.CopyIdentity = function(cmpIdentity)
{
	this.miragedIids.add(IID_Identity);
	this.classesList = cmpIdentity.GetClassesList();
};

Mirage.prototype.GetClassesList = function() { return this.classesList };

// Foundation data

Mirage.prototype.CopyFoundation = function(cmpFoundation)
{
	this.miragedIids.add(IID_Foundation);
	this.buildPercentage = cmpFoundation.GetBuildPercentage();
	this.numBuilders = cmpFoundation.GetNumBuilders();
};

Mirage.prototype.GetBuildPercentage = function() { return this.buildPercentage; };
Mirage.prototype.GetNumBuilders = function() { return this.numBuilders; };

// Repairable data (numBuilders shared with foundation as entities can't have both)

Mirage.prototype.CopyRepairable = function(cmpRepairable)
{
	this.miragedIids.add(IID_Repairable);
	this.numBuilders = cmpRepairable.GetNumBuilders();
};

// Health data

Mirage.prototype.CopyHealth = function(cmpHealth)
{
	this.miragedIids.add(IID_Health);
	this.maxHitpoints = cmpHealth.GetMaxHitpoints();
	this.hitpoints = cmpHealth.GetHitpoints();
	this.repairable = cmpHealth.IsRepairable();
	this.unhealable = cmpHealth.IsUnhealable();
};

Mirage.prototype.GetMaxHitpoints = function() { return this.maxHitpoints; };
Mirage.prototype.GetHitpoints = function() { return this.hitpoints; };
Mirage.prototype.IsRepairable = function() { return this.repairable; };
Mirage.prototype.IsUnhealable = function() { return this.unhealable; };

// Capture data

Mirage.prototype.CopyCapturable = function(cmpCapturable)
{
	this.miragedIids.add(IID_Capturable);
	this.capturePoints = clone(cmpCapturable.GetCapturePoints());
	this.maxCapturePoints = cmpCapturable.GetMaxCapturePoints();
};

Mirage.prototype.GetMaxCapturePoints = function() { return this.maxCapturePoints; };
Mirage.prototype.GetCapturePoints = function() { return this.capturePoints; };

Mirage.prototype.CanCapture = Capturable.prototype.CanCapture;

// ResourceSupply data

Mirage.prototype.CopyResourceSupply = function(cmpResourceSupply)
{
	this.miragedIids.add(IID_ResourceSupply);
	this.maxAmount = cmpResourceSupply.GetMaxAmount();
	this.amount = cmpResourceSupply.GetCurrentAmount();
	this.type = cmpResourceSupply.GetType();
	this.isInfinite = cmpResourceSupply.IsInfinite();
	this.killBeforeGather = cmpResourceSupply.GetKillBeforeGather();
	this.maxGatherers = cmpResourceSupply.GetMaxGatherers();
	this.numGatherers = cmpResourceSupply.GetNumGatherers();
};

Mirage.prototype.GetMaxAmount = function() { return this.maxAmount; };
Mirage.prototype.GetCurrentAmount = function() { return this.amount; };
Mirage.prototype.GetType = function() { return this.type; };
Mirage.prototype.IsInfinite = function() { return this.isInfinite; };
Mirage.prototype.GetKillBeforeGather = function() { return this.killBeforeGather; };
Mirage.prototype.GetMaxGatherers = function() { return this.maxGatherers; };
Mirage.prototype.GetNumGatherers = function() { return this.numGatherers; };

// Market data

Mirage.prototype.CopyMarket = function(cmpMarket)
{
	this.miragedIids.add(IID_Market);
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

Mirage.prototype.HasType = function(type) { return this.marketType.has(type); };
Mirage.prototype.GetInternationalBonus = function() { return this.internationalBonus; };
Mirage.prototype.AddTrader = function(trader) { this.traders.add(trader); };
Mirage.prototype.RemoveTrader = function(trader) { this.traders.delete(trader); };

Mirage.prototype.UpdateTraders = function(msg)
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

// ============================

Mirage.prototype.OnVisibilityChanged = function(msg)
{
	if (msg.player != this.player || msg.newVisibility != VIS_HIDDEN)
		return;

	if (this.miragedIids.has(IID_Market))
		this.UpdateTraders(msg);

	if (this.parent == INVALID_ENTITY)
		Engine.DestroyEntity(this.entity);
	else
		Engine.PostMessage(this.entity, MT_EntityRenamed, { "entity": this.entity, "newentity": this.parent });
};

Engine.RegisterComponentType(IID_Mirage, "Mirage", Mirage);
