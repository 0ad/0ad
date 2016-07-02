function Upgrade() {}

const UPGRADING_PROGRESS_INTERVAL = 250;

Upgrade.prototype.Schema =
	"<oneOrMore>" +
		"<element>" +
			"<anyName />" +
			"<interleave>" +
				"<element name='Entity' a:help='Entity to upgrade to'>" +
					"<text/>" +
				"</element>" +
				"<optional>" +
					"<element name='Icon' a:help='Icon to show in the GUI'>" +
						"<text/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Tooltip' a:help='This will be added to the tooltip to help the player choose why to upgrade.'>" +
						"<text/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Time' a:help='Time required to upgrade this entity, in seconds'>" +
						"<data type='nonNegativeInteger'/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Cost' a:help='Resource cost to upgrade this unit'>" +
						"<oneOrMore>" +
							"<choice>" +
								"<element name='food'><data type='nonNegativeInteger'/></element>" +
								"<element name='wood'><data type='nonNegativeInteger'/></element>" +
								"<element name='stone'><data type='nonNegativeInteger'/></element>" +
								"<element name='metal'><data type='nonNegativeInteger'/></element>" +
							"</choice>" +
						"</oneOrMore>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='RequiredTechnology' a:help='Define what technology is required for this upgrade'>" +
						"<choice>" +
							"<text/>" +
							"<empty/>" +
						"</choice>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='CheckPlacementRestrictions' a:help='Upgrading will check for placement restrictions (nb:GUI only)'><empty/></element>" +
				"</optional>" +
			"</interleave>" +
		"</element>" +
	"</oneOrMore>";

Upgrade.prototype.Init = function()
{
	this.upgrading = false;
	this.elapsedTime = 0;
	this.timer = undefined;

	this.upgradeTemplates = {};

	for (let choice in this.template)
	{
		let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
		let name = this.template[choice].Entity;
		if (cmpIdentity)
			name = name.replace(/\{civ\}/g, cmpIdentity.GetCiv());
		if (this.upgradeTemplates.name)
			warn("Upgrade Component: entity " + this.entity + " has two upgrades to the same entity, only the last will be used.");
		this.upgradeTemplates[name] = choice;
	}
};

// On owner change, abort the upgrade
// This will also deal with the "OnDestroy" case.
Upgrade.prototype.OnOwnershipChanged = function(msg)
{
	this.CancelUpgrade(msg.from);
	if (msg.to !== -1)
		this.owner = msg.to;
};

Upgrade.prototype.ChangeUpgradedEntityCount = function(amount)
{
	if (!this.IsUpgrading())
		return;
	
	let cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let template = cmpTempMan.GetTemplate(this.upgrading);

	let category;
	if (template.TrainingRestrictions)
		category = template.TrainingRestrictions.Category;
	else if (template.BuildRestrictions)
		category = template.BuildRestrictions.Category;

	if (!category)
		return;

	let cmpEntityLimits = QueryPlayerIDInterface(this.owner, IID_EntityLimits);
	cmpEntityLimits.ChangeCount(category, amount);
};

Upgrade.prototype.CanUpgradeTo = function(template)
{
	return this.upgradeTemplates[template] !== undefined;
};

Upgrade.prototype.GetUpgrades = function()
{
	let ret = [];

	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);

	for (let option in this.template)
	{
		let choice = this.template[option];
		let entType = choice.Entity;
		if (cmpIdentity)
			entType = entType.replace(/\{civ\}/g, cmpIdentity.GetCiv());

		let hasCosts;
		let cost = {};
		if (choice.Cost)
		{
			hasCosts = true;
			for (let type in choice.Cost)
				cost[type] = ApplyValueModificationsToTemplate("Upgrade/Cost/"+type, +choice.Cost[type], this.owner, entType);
		}
		if (choice.Time)
		{
			hasCosts = true;
			cost.time = ApplyValueModificationsToTemplate("Upgrade/Time", +choice.Time, this.owner, entType);
		}
		ret.push({
			"entity": entType,
			"icon": choice.Icon || undefined,
			"cost": hasCosts ? cost : undefined,
			"tooltip": choice.Tooltip || undefined,
			"requiredTechnology": this.GetRequiredTechnology(option),
		});
	}

	return ret;
};

Upgrade.prototype.CancelTimer = function()
{
	if (!this.timer)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	this.timer = undefined;
};

Upgrade.prototype.IsUpgrading = function()
{
	return !!this.upgrading;
};

Upgrade.prototype.GetUpgradingTo = function()
{
	return this.upgrading;
};

Upgrade.prototype.WillCheckPlacementRestrictions = function(template)
{
	if (!this.upgradeTemplates[template])
		return undefined;

	// is undefined by default so use X in Y
	return "CheckPlacementRestrictions" in this.template[this.upgradeTemplates[template]];
};

Upgrade.prototype.GetRequiredTechnology = function(templateArg)
{
	let choice = this.upgradeTemplates[templateArg] || templateArg

	if (this.template[choice].RequiredTechnology)
		return this.template[choice].RequiredTechnology;

	if (!("RequiredTechnology" in this.template[choice]))
		return undefined;

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);

	let entType = this.template[choice].Entity;
	if (cmpIdentity)
		entType = entType.replace(/\{civ\}/g, cmpIdentity.GetCiv());

	let template = cmpTemplateManager.GetTemplate(entType);
	if (template.Identity.RequiredTechnology)
		return template.Identity.RequiredTechnology;

	return undefined;
};

Upgrade.prototype.GetResourceCosts = function(template)
{
	if (!this.upgradeTemplates[template])
		return undefined;

	let choice = this.upgradeTemplates[template];
	if (!this.template[choice].Cost)
		return {};

	let costs = {};
	for (let r in this.template[choice].Cost)
	{
		costs[r] = ApplyValueModificationsToEntity("Upgrade/Cost/"+r, +this.template[choice].Cost[r], this.entity);
	}
	return costs;
};

Upgrade.prototype.Upgrade = function(template)
{
	if (this.IsUpgrading() || !this.upgradeTemplates[template])
		return false;

	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	if (!cmpPlayer.TrySubtractResources(this.GetResourceCosts(template)))
		return false;

	this.upgrading = template;

	// Prevent cheating
	this.ChangeUpgradedEntityCount(1);

	if (this.GetUpgradeTime(template) !== 0)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetInterval(this.entity, IID_Upgrade, "UpgradeProgress", 0, UPGRADING_PROGRESS_INTERVAL, { "upgrading": template });
	}
	else
		this.UpgradeProgress();

	return true;
};

Upgrade.prototype.CancelUpgrade = function(owner = undefined)
{
	if (!this.IsUpgrading())
		return;

	let cmpPlayer = owner ? QueryPlayerIDInterface(owner, IID_Player) : QueryOwnerInterface(this.entity, IID_Player);
	if (cmpPlayer)
		cmpPlayer.AddResources(this.GetResourceCosts(this.upgrading));

	this.ChangeUpgradedEntityCount(-1);

	this.upgrading = false;
	this.CancelTimer();
	this.SetElapsedTime(0);
};

Upgrade.prototype.GetUpgradeTime = function(templateArg)
{
	let template = this.upgrading || templateArg;
	let choice = this.upgradeTemplates[template];
	if (!choice)
		return undefined;
	return this.template[choice].Time ? ApplyValueModificationsToEntity("Upgrade/Time", +this.template[choice].Time, this.entity) : 0;
};

Upgrade.prototype.GetElapsedTime = function()
{
	return this.elapsedTime;
};

Upgrade.prototype.GetProgress = function()
{
	if (!this.IsUpgrading())
		return undefined;
	return this.GetUpgradeTime() == 0 ? 1 : Math.min(this.elapsedTime / 1000.0 / this.GetUpgradeTime(), 1.0);
};

Upgrade.prototype.SetElapsedTime = function(time)
{
	this.elapsedTime = time;
};

Upgrade.prototype.UpgradeProgress = function(data, lateness)
{
	if (this.elapsedTime/1000.0 < this.GetUpgradeTime())
	{
		this.SetElapsedTime(this.GetElapsedTime() + UPGRADING_PROGRESS_INTERVAL + lateness);
		return;
	}

	this.CancelTimer();

	let newEntity = ChangeEntityTemplate(this.entity, this.upgrading);

	if (newEntity)
		PlaySound("upgraded", newEntity);
};

Engine.RegisterComponentType(IID_Upgrade, "Upgrade", Upgrade);
