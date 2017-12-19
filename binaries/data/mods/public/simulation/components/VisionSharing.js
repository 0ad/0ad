function VisionSharing() {}

VisionSharing.prototype.Schema =
	"<element name='Bribable'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Duration' a:help='Duration (in second) of the vision sharing for spies'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='FailureCostRatio' a:help='Fraction of the bribe cost that will be incured if a bribe failed'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>";

VisionSharing.prototype.Init = function()
{
	this.activated = false;
	this.shared = undefined;
	this.spyId = 0;
	this.spies = undefined;
};

/**
 * As entities have not necessarily the VisionSharing component, it has to be activated
 * before use so that the rangeManager can register it
 */
VisionSharing.prototype.Activate = function()
{
	if (this.activated)
		return;
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() <= 0)
		return;
	this.shared = new Set([cmpOwnership.GetOwner()]);
	Engine.PostMessage(this.entity, MT_VisionSharingChanged,
		{ "entity": this.entity, "player": cmpOwnership.GetOwner(), "add": true });
	this.activated = true;
};

VisionSharing.prototype.CheckVisionSharings = function()
{
	let shared = new Set();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let owner = cmpOwnership ? cmpOwnership.GetOwner() : -1;
	if (owner >= 0)
	{
		// The owner has vision
		if (owner > 0)
			shared.add(owner);

		// Vision sharing due to garrisoned units
		let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
		{
			for (let ent of cmpGarrisonHolder.GetEntities())
			{
				let cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
				if (!cmpEntOwnership)
					continue;
				let entOwner = cmpEntOwnership.GetOwner();
				if (entOwner > 0 && entOwner != owner)
				{
					shared.add(entOwner);
					// if shared by another player than the owner and not yet activated, do it
					this.Activate();
				}
			}
		}

		// vision sharing due to spies
		if (this.spies)
			for (let spy of this.spies.values())
				if (spy > 0 && spy != owner)
					shared.add(spy);
	}

	if (!this.activated)
		return;

	// compare with previous vision sharing, and update if needed
	for (let player of shared)
		if (!this.shared.has(player))
			Engine.PostMessage(this.entity, MT_VisionSharingChanged,
				{ "entity": this.entity, "player": player, "add": true });
	for (let player of this.shared)
		if (!shared.has(player))
			Engine.PostMessage(this.entity, MT_VisionSharingChanged,
				{ "entity": this.entity, "player": player, "add": false });
	this.shared = shared;
};

VisionSharing.prototype.IsBribable = function()
{
	return this.template.Bribable == "true";
};

VisionSharing.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	this.CheckVisionSharings();
};

VisionSharing.prototype.OnOwnershipChanged = function(msg)
{
	if (this.activated)
		this.CheckVisionSharings();
};

VisionSharing.prototype.AddSpy = function(player, timeLength)
{
	if (!this.IsBribable())
		return 0;

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == player || player <= 0)
		return 0;

	let cmpTechnologyManager = QueryPlayerIDInterface(player, IID_TechnologyManager);
	if (!cmpTechnologyManager || !cmpTechnologyManager.CanProduce("special/spy"))
		return 0;

	let template = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetTemplate("special/spy");
	if (!IncurBribeCost(template, player, cmpOwnership.GetOwner(), false))
		return 0;

	// If no duration given, take it from the spy template and scale it with the ent vision
	// When no duration argument nor in spy template, it is a permanent spy
	let duration = timeLength;
	if (!duration && template.VisionSharing && template.VisionSharing.Duration)
	{
		duration = ApplyValueModificationsToTemplate("VisionSharing/Duration", +template.VisionSharing.Duration, player, template);
		let cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
		if (cmpVision)
			duration *= 60 / Math.max(30, cmpVision.GetRange());
	}

	if (!this.spies)
		this.spies = new Map();

	this.spies.set(++this.spyId, player);
	if (duration)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.SetTimeout(this.entity, IID_VisionSharing, "RemoveSpy", duration * 1000, { "id": this.spyId });
	}
	this.Activate();
	this.CheckVisionSharings();

	// update statistics for successful bribes
	let cmpBribesStatisticsTracker = QueryPlayerIDInterface(player, IID_StatisticsTracker);
	if (cmpBribesStatisticsTracker)
		cmpBribesStatisticsTracker.IncreaseSuccessfulBribesCounter();

	return this.spyId;
};

VisionSharing.prototype.RemoveSpy = function(data)
{
	this.spies.delete(data.id);
	this.CheckVisionSharings();
};

/**
 * Returns true if this entity share its vision with player
 */
VisionSharing.prototype.ShareVisionWith = function(player)
{
	if (this.activated)
		return this.shared.has(player);

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	return cmpOwnership && cmpOwnership.GetOwner() == player;
};

Engine.RegisterComponentType(IID_VisionSharing, "VisionSharing", VisionSharing);
