function Researcher() {}

Researcher.prototype.Schema =
	"<a:help>Allows the entity to research technologies.</a:help>" +
	"<a:example>" +
		"<TechCostMultiplier>" +
			"<food>0.5</food>" +
			"<wood>0.1</wood>" +
			"<stone>0</stone>" +
			"<metal>2</metal>" +
			"<time>0.9</time>" +
		"</TechCostMultiplier>" +
		"<Technologies datatype='tokens'>" +
			"\n    phase_town_{civ}\n    phase_metropolis_ptol\n    unlock_shared_los\n    wonder_population_cap\n  " +
		"</Technologies>" +
	"</a:example>" +
	"<optional>" +
		"<element name='Technologies' a:help='Space-separated list of technology names that this building can research. When present, the special string \"{civ}\" will be automatically replaced either by the civ code of the building&apos;s owner if such a tech exists, or by \"generic\".'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='TechCostMultiplier' a:help='Multiplier to modify resources cost and research time of technologies researched in this building.'>" +
			Resources.BuildSchema("nonNegativeDecimal", ["time"]) +
		"</element>" +
	"</optional>";

/**
 * This object represents a technology being researched.
 * @param {string} templateName - The name of the template we ought to research.
 * @param {number} researcher - The entity ID of our researcher.
 * @param {string} metadata - Optionally any metadata to attach to us.
 */
Researcher.prototype.Item = function(templateName, researcher, metadata)
{
	this.templateName = templateName;
	this.researcher = researcher;
	this.metadata = metadata;
};

/**
 * Prepare for the queue.
 * @param {Object} techCostMultiplier - The multipliers to use when calculating costs.
 * @return {boolean} - Whether the item was successfully initiated.
 */
Researcher.prototype.Item.prototype.Queue = function(techCostMultiplier)
{
	this.player = QueryOwnerInterface(this.researcher).GetPlayerID();
	const cmpTechnologyManager = QueryPlayerIDInterface(this.player, IID_TechnologyManager);
	if (!cmpTechnologyManager.QueuedResearch(this.templateName, this.researcher, techCostMultiplier))
		return false;

	return true;
};

Researcher.prototype.Item.prototype.Stop = function()
{
	QueryPlayerIDInterface(this.player, IID_TechnologyManager).StoppedResearch(this.templateName);
	delete this.started;
};

/**
 * Called when the first work is performed.
 */
Researcher.prototype.Item.prototype.Start = function()
{
	this.started = true;
};

Researcher.prototype.Item.prototype.Finish = function()
{
	this.finished = true;
};

/**
 * @param {number} allocatedTime - The time allocated to this item.
 * @return {number} - The time used for this item.
 */
Researcher.prototype.Item.prototype.Progress = function(allocatedTime)
{
	if (!this.started)
		this.Start();
	if (this.paused)
		this.Unpause();
	const cmpTechnologyManager = QueryPlayerIDInterface(this.player, IID_TechnologyManager);
	const usedTime = cmpTechnologyManager.Progress(this.templateName, allocatedTime);
	if (!cmpTechnologyManager.IsTechnologyQueued(this.templateName))
		this.Finish();
	return usedTime;
};

Researcher.prototype.Item.prototype.Pause = function()
{
	QueryPlayerIDInterface(this.player, IID_TechnologyManager).Pause(this.templateName);
	this.paused = true;
};

Researcher.prototype.Item.prototype.Unpause = function()
{
	delete this.paused;
};

/**
 * @return {Object} - Some basic information of this item.
 */
Researcher.prototype.Item.prototype.GetBasicInfo = function()
{
	const result = QueryPlayerIDInterface(this.player, IID_TechnologyManager).GetBasicInfo(this.templateName);
	result.technologyTemplate = this.templateName;
	result.metadata = this.metadata;
	return result;
};

Researcher.prototype.Item.prototype.SerializableAttributes = [
	"metadata",
	"paused",
	"player",
	"researcher",
	"started",
	"templateName"
];

Researcher.prototype.Item.prototype.Serialize = function(id)
{
	const result = {
		"id": id
	};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];
	return result;
};

Researcher.prototype.Item.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];
};

Researcher.prototype.Init = function()
{
	this.nextID = 1;
	this.queue = new Map();
};

Researcher.prototype.Serialize = function()
{
	const queue = [];
	for (const [id, item] of this.queue)
		queue.push(item.Serialize(id));

	return {
		"nextID": this.nextID,
		"queue": queue
	};
};

Researcher.prototype.Deserialize = function(data)
{
	this.Init();
	this.nextID = data.nextID;
	for (const item of data.queue)
	{
		const newItem = new this.Item();
		newItem.Deserialize(item);
		this.queue.set(item.id, newItem);
	}
};

/*
 * Returns list of technologies that can be researched by this entity.
 */
Researcher.prototype.GetTechnologiesList = function()
{
	const string = ApplyValueModificationsToEntity("Researcher/Technologies/_string", this.template?.Technologies?._string || "", this.entity);
	if (!string)
		return [];

	const owner = Engine.QueryInterface(this.entity, IID_Ownership)?.GetOwner();
	if (!owner || owner === INVALID_PLAYER)
		return [];

	const playerEnt = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetPlayerByID(owner);
	if (!playerEnt)
		return [];

	const cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return [];

	const cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
	if (!cmpPlayer)
		return [];

	let techs = string.split(/\s+/);

	// Replace the civ specific technologies.
	const civ = Engine.QueryInterface(playerEnt, IID_Identity).GetCiv();
	for (let i = 0; i < techs.length; ++i)
	{
		const tech = techs[i];
		if (tech.indexOf("{civ}") == -1)
			continue;
		const civTech = tech.replace("{civ}", civ);
		techs[i] = TechnologyTemplates.Has(civTech) ? civTech : tech.replace("{civ}", "generic");
	}

	// Remove any technologies that can't be researched by this civ.
	techs = techs.filter(tech =>
		cmpTechnologyManager.CheckTechnologyRequirements(
			DeriveTechnologyRequirements(TechnologyTemplates.Get(tech), civ),
			true));

	const techList = [];
	const superseded = {};

	const disabledTechnologies = cmpPlayer.GetDisabledTechnologies();

	// Add any top level technologies to an array which corresponds to the displayed icons.
	// Also store what technology is superseded in the superseded object { "tech1":"techWhichSupercedesTech1", ... }.
	for (const tech of techs)
	{
		if (disabledTechnologies && disabledTechnologies[tech])
			continue;

		const template = TechnologyTemplates.Get(tech);
		if (!template.supersedes || techs.indexOf(template.supersedes) === -1)
			techList.push(tech);
		else
			superseded[template.supersedes] = tech;
	}

	// Now make researched/in progress techs invisible.
	for (const i in techList)
	{
		let tech = techList[i];
		while (this.IsTechnologyResearchedOrInProgress(tech))
			tech = superseded[tech];

		techList[i] = tech;
	}

	const ret = [];

	// This inserts the techs into the correct positions to line up the technology pairs.
	for (let i = 0; i < techList.length; ++i)
	{
		const tech = techList[i];
		if (!tech)
		{
			ret[i] = undefined;
			continue;
		}

		const template = TechnologyTemplates.Get(tech);
		if (template.top)
			ret[i] = { "pair": true, "top": template.top, "bottom": template.bottom };
		else
			ret[i] = tech;
	}

	return ret;
};

/**
 * @return {Object} - The multipliers to change the costs of any research with.
 */
Researcher.prototype.GetTechCostMultiplier = function()
{
	const techCostMultiplier = {};
	for (const res of Resources.GetCodes().concat(["time"]))
		techCostMultiplier[res] = ApplyValueModificationsToEntity(
		    "Researcher/TechCostMultiplier/" + res,
		    +(this.template?.TechCostMultiplier?.[res] || 1),
		    this.entity);

	return techCostMultiplier;
};

/**
 * Checks whether we can research the given technology, minding paired techs.
 */
Researcher.prototype.IsTechnologyResearchedOrInProgress = function(tech)
{
	if (!tech)
		return false;

	const cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return false;

	const template = TechnologyTemplates.Get(tech);
	if (template.top)
		return cmpTechnologyManager.IsTechnologyResearched(template.top) ||
		    cmpTechnologyManager.IsInProgress(template.top) ||
		    cmpTechnologyManager.IsTechnologyResearched(template.bottom) ||
		    cmpTechnologyManager.IsInProgress(template.bottom);

	return cmpTechnologyManager.IsTechnologyResearched(tech) || cmpTechnologyManager.IsInProgress(tech);
};

/**
 * @param {string} templateName - The technology to queue.
 * @param {string} metadata - Any metadata attached to the item.
 * @return {number} - The ID of the item. -1 if the item could not be researched.
 */
Researcher.prototype.QueueTechnology = function(templateName, metadata)
{
	if (!this.GetTechnologiesList().some(tech =>
		tech && (tech == templateName ||
			tech.pair && (tech.top == templateName || tech.bottom == templateName))))
	{
		error("This entity cannot research " + templateName + ".");
		return -1;
	}

	const item = new this.Item(templateName, this.entity, metadata);

	const techCostMultiplier = this.GetTechCostMultiplier();
	if (!item.Queue(techCostMultiplier))
		return -1;

	const id = this.nextID++;
	this.queue.set(id, item);
	return id;
};

/**
 * @param {number} id - The id of the technology researched here we need to stop.
 */
Researcher.prototype.StopResearching = function(id)
{
	this.queue.get(id).Stop();
	this.queue.delete(id);
};

/**
 * @param {number} id - The id of the technology.
 */
Researcher.prototype.PauseTechnology = function(id)
{
	this.queue.get(id).Pause();
};

/**
 * @param {number} id - The ID of the item to check.
 * @return {boolean} - Whether we are currently training the item.
 */
Researcher.prototype.HasItem = function(id)
{
	return this.queue.has(id);
};

/**
 * @parameter {number} id - The id of the research.
 * @return {Object} - Some basic information about the research.
 */
Researcher.prototype.GetResearchingTechnology = function(id)
{
	return this.queue.get(id).GetBasicInfo();
};

/**
 * @param {number} id - The ID of the item we spent time on.
 * @param {number} allocatedTime - The time we spent on the given item.
 * @return {number} - The time we've actually used.
 */
Researcher.prototype.Progress = function(id, allocatedTime)
{
	const item = this.queue.get(id);
	const usedTime = item.Progress(allocatedTime);
	if (item.finished)
		this.queue.delete(id);
	return usedTime;
};

Engine.RegisterComponentType(IID_Researcher, "Researcher", Researcher);
