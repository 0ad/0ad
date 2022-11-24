function TechnologyManager() {}

TechnologyManager.prototype.Schema =
	"<empty/>";

/**
 * This object represents a technology under research.
 * @param {string} templateName - The name of the template to research.
 * @param {number} player - The player ID researching.
 * @param {number} researcher - The entity ID researching.
 */
TechnologyManager.prototype.Technology = function(templateName, player, researcher)
{
	this.player = player;
	this.researcher = researcher;
	this.templateName = templateName;
};

/**
 * Prepare for the queue.
 * @param {Object} techCostMultiplier - The multipliers to use when calculating costs.
 * @return {boolean} - Whether the technology was successfully initiated.
 */
TechnologyManager.prototype.Technology.prototype.Queue = function(techCostMultiplier)
{
	const template = TechnologyTemplates.Get(this.templateName);
	if (!template)
		return false;

	this.resources = {};
	if (template.cost)
		for (const res in template.cost)
			this.resources[res] = Math.floor(techCostMultiplier[res] * template.cost[res]);

	// ToDo: Subtract resources here or in cmpResearcher?
	const cmpPlayer = Engine.QueryInterface(this.player, IID_Player);
	// TrySubtractResources should report error to player (they ran out of resources).
	if (!cmpPlayer?.TrySubtractResources(this.resources))
		return false;

	const time = techCostMultiplier.time * (template.researchTime || 0) * 1000;
	this.timeRemaining = time;
	this.timeTotal = time;

	const playerID = cmpPlayer.GetPlayerID();
	Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).CallEvent("OnResearchQueued", {
		"playerid": playerID,
		"technologyTemplate": this.templateName,
		"researcherEntity": this.researcher
	});

	return true;
};

TechnologyManager.prototype.Technology.prototype.Stop = function()
{
	const cmpPlayer = Engine.QueryInterface(this.player, IID_Player);
	cmpPlayer?.RefundResources(this.resources);
	delete this.resources;

	if (this.started && this.templateName.startsWith("phase"))
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
			"type": "phase",
			"players": [cmpPlayer.GetPlayerID()],
			"phaseName": this.templateName,
			"phaseState": "aborted"
		});
};

/**
 * Called when the first work is performed.
 */
TechnologyManager.prototype.Technology.prototype.Start = function()
{
	this.started = true;
	if (!this.templateName.startsWith("phase"))
		return;

	const cmpPlayer = Engine.QueryInterface(this.player, IID_Player);
	Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
		"type": "phase",
		"players": [cmpPlayer.GetPlayerID()],
		"phaseName": this.templateName,
		"phaseState": "started"
	});
};

TechnologyManager.prototype.Technology.prototype.Finish = function()
{
	this.finished = true;

	const template = TechnologyTemplates.Get(this.templateName);
	if (template.soundComplete)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_SoundManager)?.PlaySoundGroup(template.soundComplete, this.researcher);

	if (template.modifications)
	{
		const cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.AddModifiers("tech/" + this.templateName, DeriveModificationsFromTech(template), this.player);
	}

	const cmpEntityLimits = Engine.QueryInterface(this.player, IID_EntityLimits);
	const cmpTechnologyManager = Engine.QueryInterface(this.player, IID_TechnologyManager);
	if (template.replaces && template.replaces.length > 0)
		for (const i of template.replaces)
		{
			cmpTechnologyManager.MarkTechnologyAsResearched(i);
			cmpEntityLimits?.UpdateLimitsFromTech(i);
		}

	cmpTechnologyManager.MarkTechnologyAsResearched(this.templateName);

	// ToDo: Move to EntityLimits.js.
	cmpEntityLimits?.UpdateLimitsFromTech(this.templateName);

	const playerID = Engine.QueryInterface(this.player, IID_Player).GetPlayerID();
	Engine.PostMessage(this.player, MT_ResearchFinished, { "player": playerID, "tech": this.templateName });

	if (this.templateName.startsWith("phase") && !template.autoResearch)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
			"type": "phase",
			"players": [playerID],
			"phaseName": this.templateName,
			"phaseState": "completed"
		});
};

/**
 * @param {number} allocatedTime - The time allocated to this item.
 * @return {number} - The time used for this item.
 */
TechnologyManager.prototype.Technology.prototype.Progress = function(allocatedTime)
{
	if (!this.started)
		this.Start();
	if (this.paused)
		this.Unpause();
	if (this.timeRemaining > allocatedTime)
	{
		this.timeRemaining -= allocatedTime;
		return allocatedTime;
	}
	this.Finish();
	return this.timeRemaining;
};

TechnologyManager.prototype.Technology.prototype.Pause = function()
{
	this.paused = true;
};

TechnologyManager.prototype.Technology.prototype.Unpause = function()
{
	delete this.paused;
};

TechnologyManager.prototype.Technology.prototype.GetBasicInfo = function()
{
	return {
		"paused": this.paused,
		"progress": 1 - (this.timeRemaining / (this.timeTotal || 1)),
		"researcher": this.researcher,
		"templateName": this.templateName,
		"timeRemaining": this.timeRemaining
	};
};

TechnologyManager.prototype.Technology.prototype.SerializableAttributes = [
	"paused",
	"player",
	"researcher",
	"resources",
	"started",
	"templateName",
	"timeRemaining",
	"timeTotal"
];

TechnologyManager.prototype.Technology.prototype.Serialize = function()
{
	const result = {};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];
	return result;
};

TechnologyManager.prototype.Technology.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];
};

TechnologyManager.prototype.Init = function()
{
	// Holds names of technologies that have been researched.
	this.researchedTechs = new Set();

	// Maps from technolgy name to the technology object.
	this.researchQueued = new Map();

	this.classCounts = {}; // stores the number of entities of each Class
	this.typeCountsByClass = {}; // stores the number of entities of each type for each class i.e.
	                             // {"someClass": {"unit/spearman": 2, "unit/cav": 5} "someOtherClass":...}

	// Some technologies are automatically researched when their conditions are met.  They have no cost and are
	// researched instantly.  This allows civ bonuses and more complicated technologies.
	this.unresearchedAutoResearchTechs = new Set();
	let allTechs = TechnologyTemplates.GetAll();
	for (let key in allTechs)
		if (allTechs[key].autoResearch || allTechs[key].top)
			this.unresearchedAutoResearchTechs.add(key);
};

TechnologyManager.prototype.SerializableAttributes = [
	"researchedTechs",
	"classCounts",
	"typeCountsByClass",
	"unresearchedAutoResearchTechs"
];

TechnologyManager.prototype.Serialize = function()
{
	const result = {};
	for (const att of this.SerializableAttributes)
		if (this.hasOwnProperty(att))
			result[att] = this[att];

	result.researchQueued = [];
	for (const [techName, techObject] of this.researchQueued)
		result.researchQueued.push(techObject.Serialize());

	return result;
};

TechnologyManager.prototype.Deserialize = function(data)
{
	for (const att of this.SerializableAttributes)
		if (att in data)
			this[att] = data[att];

	this.researchQueued = new Map();
	for (const tech of data.researchQueued)
	{
		const newTech = new this.Technology();
		newTech.Deserialize(tech);
		this.researchQueued.set(tech.templateName, newTech);
	}
};

TechnologyManager.prototype.OnUpdate = function()
{
	this.UpdateAutoResearch();
};

// This function checks if the requirements of any autoresearch techs are met and if they are it researches them
TechnologyManager.prototype.UpdateAutoResearch = function()
{
	for (let key of this.unresearchedAutoResearchTechs)
	{
		let tech = TechnologyTemplates.Get(key);
		if ((tech.autoResearch && this.CanResearch(key)) ||
			(tech.top && (this.IsTechnologyResearched(tech.top) || this.IsTechnologyResearched(tech.bottom))))
		{
			this.unresearchedAutoResearchTechs.delete(key);
			this.ResearchTechnology(key);
			return; // We will have recursively handled any knock-on effects so can just return
		}
	}
};

// Checks an entity template to see if its technology requirements have been met
TechnologyManager.prototype.CanProduce = function(templateName)
{
	var cmpTempManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempManager.GetTemplate(templateName);

	if (template.Identity?.Requirements)
		return RequirementsHelper.AreRequirementsMet(template.Identity.Requirements, Engine.QueryInterface(this.entity, IID_Player).GetPlayerID());
	// If there is no required technology then this entity can be produced
	return true;
};

TechnologyManager.prototype.IsTechnologyQueued = function(tech)
{
	return this.researchQueued.has(tech);
};

TechnologyManager.prototype.IsTechnologyResearched = function(tech)
{
	return this.researchedTechs.has(tech);
};

// Checks the requirements for a technology to see if it can be researched at the current time
TechnologyManager.prototype.CanResearch = function(tech)
{
	let template = TechnologyTemplates.Get(tech);

	if (!template)
	{
		warn("Technology \"" + tech + "\" does not exist");
		return false;
	}

	if (template.top && this.IsInProgress(template.top) ||
	    template.bottom && this.IsInProgress(template.bottom))
		return false;

	if (template.pair && !this.CanResearch(template.pair))
		return false;

	if (this.IsInProgress(tech))
		return false;

	if (this.IsTechnologyResearched(tech))
		return false;

	return this.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, Engine.QueryInterface(this.entity, IID_Identity).GetCiv()));
};

/**
 * Private function for checking a set of requirements is met
 * @param {Object} reqs - Technology requirements as derived from the technology template by globalscripts
 * @param {boolean} civonly - True if only the civ requirement is to be checked
 *
 * @return true if the requirements pass, false otherwise
 */
TechnologyManager.prototype.CheckTechnologyRequirements = function(reqs, civonly = false)
{
	let cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);

	if (!reqs)
		return false;

	if (civonly || !reqs.length)
		return true;

	return reqs.some(req => {
		return Object.keys(req).every(type => {
			switch (type)
			{
			case "techs":
				return req[type].every(this.IsTechnologyResearched, this);

			case "entities":
				return req[type].every(this.DoesEntitySpecPass, this);
			}
			return false;
		});
	});
};

TechnologyManager.prototype.DoesEntitySpecPass = function(entity)
{
	switch (entity.check)
	{
	case "count":
		if (!this.classCounts[entity.class] || this.classCounts[entity.class] < entity.number)
			return false;
		break;

	case "variants":
		if (!this.typeCountsByClass[entity.class] || Object.keys(this.typeCountsByClass[entity.class]).length < entity.number)
			return false;
		break;
	}
	return true;
};

TechnologyManager.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// This automatically updates classCounts and typeCountsByClass
	var playerID = (Engine.QueryInterface(this.entity, IID_Player)).GetPlayerID();
	if (msg.to == playerID)
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var template = cmpTemplateManager.GetCurrentTemplateName(msg.entity);

		var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
		if (!cmpIdentity)
			return;

		var classes = cmpIdentity.GetClassesList();
		// don't use foundations for the class counts but check if techs apply (e.g. health increase)
		if (!Engine.QueryInterface(msg.entity, IID_Foundation))
		{
			for (let cls of classes)
			{
				this.classCounts[cls] = this.classCounts[cls] || 0;
				this.classCounts[cls] += 1;

				this.typeCountsByClass[cls] = this.typeCountsByClass[cls] || {};
				this.typeCountsByClass[cls][template] = this.typeCountsByClass[cls][template] || 0;
				this.typeCountsByClass[cls][template] += 1;
			}
		}
	}
	if (msg.from == playerID)
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var template = cmpTemplateManager.GetCurrentTemplateName(msg.entity);

		// don't use foundations for the class counts
		if (!Engine.QueryInterface(msg.entity, IID_Foundation))
		{
			var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
			if (cmpIdentity)
			{
				var classes = cmpIdentity.GetClassesList();
				for (let cls of classes)
				{
					this.classCounts[cls] -= 1;
					if (this.classCounts[cls] <= 0)
						delete this.classCounts[cls];

					this.typeCountsByClass[cls][template] -= 1;
					if (this.typeCountsByClass[cls][template] <= 0)
						delete this.typeCountsByClass[cls][template];
				}
			}
		}
	}
};

/**
 * This does neither apply effects nor verify requirements.
 * @param {string} tech - The name of the technology to mark as researched.
 */
TechnologyManager.prototype.MarkTechnologyAsResearched = function(tech)
{
	this.researchedTechs.add(tech);
	this.UpdateAutoResearch();
};

/**
 * Note that this does not verify whether the requirements are met.
 * @param {string} tech - The technology to research.
 * @param {number} researcher - Optionally the entity to couple with the research.
 */
TechnologyManager.prototype.ResearchTechnology = function(tech, researcher = INVALID_ENTITY)
{
	if (this.IsTechnologyQueued(tech) || this.IsTechnologyResearched(tech))
		return;
	const technology = new this.Technology(tech, this.entity, researcher);
	technology.Finish();
};

/**
 * Marks a technology as being queued for research at the given entityID.
 * @param {string} tech - The technology to queue.
 * @param {number} researcher - The entity ID of the entity researching this technology.
 * @param {Object} techCostMultiplier - The multipliers used when calculating the costs.
 *
 * @return {boolean} - Whether we successfully have queued the technology.
 */
TechnologyManager.prototype.QueuedResearch = function(tech, researcher, techCostMultiplier)
{
	// ToDo: Check whether the technology is researched already?
	const technology = new this.Technology(tech, this.entity, researcher);
	if (!technology.Queue(techCostMultiplier))
		return false;
	this.researchQueued.set(tech, technology);
	return true;
};

/**
 * Marks a technology as not being currently researched and optionally sends a GUI notification.
 * @param {string} tech - The name of the technology to stop.
 * @param {boolean} notification - Whether a GUI notification ought to be sent.
 */
TechnologyManager.prototype.StoppedResearch = function(tech)
{
	this.researchQueued.get(tech).Stop();
	this.researchQueued.delete(tech);
};

/**
 * @param {string} tech -
 */
TechnologyManager.prototype.Pause = function(tech)
{
	this.researchQueued.get(tech).Pause();
};

/**
 * @param {string} tech - The technology to advance.
 * @param {number} allocatedTime - The time allocated to the technology.
 * @return {number} - The time we've actually used.
 */
TechnologyManager.prototype.Progress = function(techName, allocatedTime)
{
	const technology = this.researchQueued.get(techName);
	const usedTime = technology.Progress(allocatedTime);
	if (technology.finished)
		this.researchQueued.delete(techName);
	return usedTime;
};

/**
 * @param {string} tech - The technology name to retreive some basic information for.
 * @return {Object} - Some basic information about the technology under research.
 */
TechnologyManager.prototype.GetBasicInfo = function(tech)
{
	return this.researchQueued.get(tech).GetBasicInfo();
};

/**
 * Checks whether a technology is set to be researched.
 */
TechnologyManager.prototype.IsInProgress = function(tech)
{
	return this.researchQueued.has(tech);
};

TechnologyManager.prototype.GetBasicInfoOfStartedTechs = function()
{
	const result = {};
	for (const [techName, tech] of this.researchQueued)
		if (tech.started)
			result[techName] = tech.GetBasicInfo();
	return result;
};

/**
 * Called by GUIInterface for PlayerData. AI use.
 */
TechnologyManager.prototype.GetQueuedResearch = function()
{
	return this.researchQueued;
};

/**
 * Returns the names of technologies that have already been researched.
 */
TechnologyManager.prototype.GetResearchedTechs = function()
{
	return this.researchedTechs;
};

TechnologyManager.prototype.GetClassCounts = function()
{
	return this.classCounts;
};

TechnologyManager.prototype.GetTypeCountsByClass = function()
{
	return this.typeCountsByClass;
};

Engine.RegisterComponentType(IID_TechnologyManager, "TechnologyManager", TechnologyManager);
