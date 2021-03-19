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

function MiragedIdentity() {}
MiragedIdentity.prototype.Init = function(cmpIdentity)
{
	// Mirages don't get identity classes via the template-filter, so that code can query
	// identity components via Engine.QueryInterface without having to explicitly check for mirages.
	// This is cloned as otherwise we get a reference to Identity's property,
	// and that array is deleted when serializing (as it's not seralized), which ends in OOS.
	this.classes = clone(cmpIdentity.GetClassesList());
};

MiragedIdentity.prototype.GetClassesList = function() { return this.classes; };
Engine.RegisterGlobal("MiragedIdentity", MiragedIdentity);

Mirage.prototype.CopyIdentity = function(cmpIdentity)
{
	let mirage = new MiragedIdentity();
	mirage.Init(cmpIdentity);
	this.miragedIids.set(IID_Identity, mirage);
};

// Foundation data

function MiragedFoundation() {}
MiragedFoundation.prototype.Init = function(cmpFoundation)
{
	this.numBuilders = cmpFoundation.GetNumBuilders();
	this.buildTime = cmpFoundation.GetBuildTime();
};

MiragedFoundation.prototype.GetNumBuilders = function() { return this.numBuilders; };
MiragedFoundation.prototype.GetBuildTime = function() { return this.buildTime; };
Engine.RegisterGlobal("MiragedFoundation", MiragedFoundation);

Mirage.prototype.CopyFoundation = function(cmpFoundation)
{
	let mirage = new MiragedFoundation();
	mirage.Init(cmpFoundation);
	this.miragedIids.set(IID_Foundation, mirage);
};

// Repairable data

function MiragedRepairable() {}
MiragedRepairable.prototype.Init = function(cmpRepairable)
{
	this.numBuilders = cmpRepairable.GetNumBuilders();
	this.buildTime = cmpRepairable.GetBuildTime();
};

MiragedRepairable.prototype.GetNumBuilders = function() { return this.numBuilders; };
MiragedRepairable.prototype.GetBuildTime = function() { return this.buildTime; };
Engine.RegisterGlobal("MiragedRepairable", MiragedRepairable);

Mirage.prototype.CopyRepairable = function(cmpRepairable)
{
	let mirage = new MiragedRepairable();
	mirage.Init(cmpRepairable);
	this.miragedIids.set(IID_Repairable, mirage);
};

// Health data

function MiragedHealth() {}
MiragedHealth.prototype.Init = function(cmpHealth)
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
Engine.RegisterGlobal("MiragedHealth", MiragedHealth);

Mirage.prototype.CopyHealth = function(cmpHealth)
{
	let mirage = new MiragedHealth();
	mirage.Init(cmpHealth);
	this.miragedIids.set(IID_Health, mirage);
};

// Capture data

function MiragedCapturable() {}
MiragedCapturable.prototype.Init = function(cmpCapturable)
{
	this.capturePoints = clone(cmpCapturable.GetCapturePoints());
	this.maxCapturePoints = cmpCapturable.GetMaxCapturePoints();
};

MiragedCapturable.prototype.GetCapturePoints = function() { return this.capturePoints; };
MiragedCapturable.prototype.GetMaxCapturePoints = function() { return this.maxCapturePoints; };
MiragedCapturable.prototype.CanCapture = Capturable.prototype.CanCapture;
Engine.RegisterGlobal("MiragedCapturable", MiragedCapturable);

Mirage.prototype.CopyCapturable = function(cmpCapturable)
{
	let mirage = new MiragedCapturable();
	mirage.Init(cmpCapturable);
	this.miragedIids.set(IID_Capturable, mirage);
};

// ResourceSupply data
function MiragedResourceSupply() {}
MiragedResourceSupply.prototype.Init = function(cmpResourceSupply)
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

// Apply diminishing returns with more gatherers, for e.g. infinite farms. For most resources this has no effect
// (GetDiminishingReturns will return null). We can assume that for resources that are miraged this is the case.
MiragedResourceSupply.prototype.GetDiminishingReturns = function() { return null; };

Engine.RegisterGlobal("MiragedResourceSupply", MiragedResourceSupply);

Mirage.prototype.CopyResourceSupply = function(cmpResourceSupply)
{
	let mirage = new MiragedResourceSupply();
	mirage.Init(cmpResourceSupply);
	this.miragedIids.set(IID_ResourceSupply, mirage);
};

// Market data
function MiragedMarket() {}
MiragedMarket.prototype.Init = function(cmpMarket, entity, parent, player)
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
Engine.RegisterGlobal("MiragedMarket", MiragedMarket);

Mirage.prototype.CopyMarket = function(cmpMarket)
{
	let mirage = new MiragedMarket();
	mirage.Init(cmpMarket, this.entity, this.parent, this.player);
	this.miragedIids.set(IID_Market, mirage);
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
