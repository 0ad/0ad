function TechnologyManager() {}

TechnologyManager.prototype.Schema =
	"<empty/>";

TechnologyManager.prototype.Init = function()
{
	// Holds names of technologies that have been researched.
	this.researchedTechs = new Set();

	// Maps from technolgy name to the entityID of the researcher.
	this.researchQueued = new Map();

	// Holds technologies which are being researched currently (non-queued).
	this.researchStarted = new Set();

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
		if ((tech.autoResearch && this.CanResearch(key))
			|| (tech.top && (this.IsTechnologyResearched(tech.top) || this.IsTechnologyResearched(tech.bottom))))
		{
			this.unresearchedAutoResearchTechs.delete(key);
			this.ResearchTechnology(key);
			return; // We will have recursively handled any knock-on effects so can just return
		}
	}
};

// Checks an entity template to see if its technology requirements have been met
TechnologyManager.prototype.CanProduce = function (templateName)
{
	var cmpTempManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempManager.GetTemplate(templateName);

	if (template.Identity && template.Identity.RequiredTechnology)
		return this.IsTechnologyResearched(template.Identity.RequiredTechnology);
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

TechnologyManager.prototype.IsTechnologyStarted = function(tech)
{
	return this.researchStarted.has(tech);
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

	return this.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, Engine.QueryInterface(this.entity, IID_Player).GetCiv()));
};

/**
 * Private function for checking a set of requirements is met
 * @param {object} reqs - Technology requirements as derived from the technology template by globalscripts
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

// Marks a technology as researched.  Note that this does not verify that the requirements are met.
TechnologyManager.prototype.ResearchTechnology = function(tech)
{
	this.StoppedResearch(tech, false);

	var modifiedComponents = {};
	this.researchedTechs.add(tech);

	// store the modifications in an easy to access structure
	let template = TechnologyTemplates.Get(tech);
	if (template.modifications)
	{
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.AddModifiers("tech/" + tech, DeriveModificationsFromTech(template), this.entity);
	}

	if (template.replaces && template.replaces.length > 0)
	{
		for (var i of template.replaces)
		{
			if (!i || this.IsTechnologyResearched(i))
				continue;

			this.researchedTechs.add(i);

			// Change the EntityLimit if any
			let cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
			if (cmpPlayer && cmpPlayer.GetPlayerID() !== undefined)
			{
				let playerID = cmpPlayer.GetPlayerID();
				let cmpPlayerEntityLimits = QueryPlayerIDInterface(playerID, IID_EntityLimits);
				if (cmpPlayerEntityLimits)
					cmpPlayerEntityLimits.UpdateLimitsFromTech(i);
			}
		}
	}

	this.UpdateAutoResearch();

	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer || cmpPlayer.GetPlayerID() === undefined)
		return;
	var playerID = cmpPlayer.GetPlayerID();
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var ents = cmpRangeManager.GetEntitiesByPlayer(playerID);
	ents.push(this.entity);

	// Change the EntityLimit if any
	var cmpPlayerEntityLimits = QueryPlayerIDInterface(playerID, IID_EntityLimits);
	if (cmpPlayerEntityLimits)
		cmpPlayerEntityLimits.UpdateLimitsFromTech(tech);

	// always send research finished message
	Engine.PostMessage(this.entity, MT_ResearchFinished, {"player": playerID, "tech": tech});
};

/**
 * Marks a technology as being queued for research at the given entityID.
 */
TechnologyManager.prototype.QueuedResearch = function(tech, researcher)
{
	this.researchQueued.set(tech, researcher);
};

// Marks a technology as actively being researched
TechnologyManager.prototype.StartedResearch = function(tech, notification)
{
	this.researchStarted.add(tech);

	if (notification && tech.startsWith("phase"))
	{
		let cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({
			"type": "phase",
			"players": [cmpPlayer.GetPlayerID()],
			"phaseName": tech,
			"phaseState": "started"
		});
	}
};

/**
 *  Marks a technology as not being currently researched and optionally sends a GUI notification.
 */
TechnologyManager.prototype.StoppedResearch = function(tech, notification)
{
	if (notification && tech.startsWith("phase") && this.researchStarted.has(tech))
	{
		let cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
			"type": "phase",
			"players": [cmpPlayer.GetPlayerID()],
			"phaseName": tech,
			"phaseState": "aborted"
		});
	}

	this.researchQueued.delete(tech);
	this.researchStarted.delete(tech);
};

/**
 * Checks whether a technology is set to be researched.
 */
TechnologyManager.prototype.IsInProgress = function(tech)
{
	return this.researchQueued.has(tech);
};

/**
 * Returns the names of technologies that are currently being researched (non-queued).
 */
TechnologyManager.prototype.GetStartedTechs = function()
{
	return this.researchStarted;
};

/**
 *  Gets the entity currently researching the technology.
 */
TechnologyManager.prototype.GetResearcher = function(tech)
{
	return this.researchQueued.get(tech);
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
