function TechnologyManager() {}

TechnologyManager.prototype.Schema =
	"<empty/>";

TechnologyManager.prototype.Serialize = function()
{
	// The modifications cache will be affected by property reads from the GUI and other places so we shouldn't
	// serialize it.

	var ret = {};
	for (var i in this)
	{
		if (this.hasOwnProperty(i))
			ret[i] = this[i];
	}
	ret.modificationCache = {};
	return ret;
};

TechnologyManager.prototype.Init = function()
{
	// Holds names of technologies which have been researched
	this.researchedTechs = new Set();

	this.researchQueued = {};  // technologies which are queued for research

	// technologies which are being researched currently (non-queued)
	this.researchStarted = new Set();

	// This stores the modifications to unit stats from researched technologies
	// Example data: {"ResourceGatherer/Rates/food.grain": [
	//                     {"multiply": 1.15, "affects": ["FemaleCitizen", "Infantry Swordsman"]},
	//                     {"add": 2}
	//                 ]}
	this.modifications = {};
	this.modificationCache = {}; // Caches the values after technologies have been applied
	                             // e.g. { "Attack/Melee/Hack" : {5: {"origValue": 8, "newValue": 10}, 7: {"origValue": 9, "newValue": 12}, ...}, ...}
	                             // where 5 and 7 are entity id's

	this.classCounts = {}; // stores the number of entities of each Class
	this.typeCountsByClass = {}; // stores the number of entities of each type for each class i.e.
	                             // {"someClass": {"unit/spearman": 2, "unit/cav": 5} "someOtherClass":...}

	// Some technologies are automatically researched when their conditions are met.  They have no cost and are
	// researched instantly.  This allows civ bonuses and more complicated technologies.
	this.unresearchedAutoResearchTechs = new Set();
	var allTechs = Engine.QueryInterface(SYSTEM_ENTITY, IID_DataTemplateManager).GetAllTechs();
	for (var key in allTechs)
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
	var cmpDataTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_DataTemplateManager);
	for (let key of this.unresearchedAutoResearchTechs)
	{
		var tech = cmpDataTempMan.GetTechnologyTemplate(key);
		if ((tech.autoResearch && this.CanResearch(key))
			|| (tech.top && (this.IsTechnologyResearched(tech.top) || this.IsTechnologyResearched(tech.bottom))))
		{
			this.unresearchedAutoResearchTechs.delete(key);
			this.ResearchTechnology(key);
			return; // We will have recursively handled any knock-on effects so can just return
		}
	}
};

TechnologyManager.prototype.GetTechnologyTemplate = function(tech)
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_DataTemplateManager).GetTechnologyTemplate(tech);
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
	return this.researchQueued[tech] !== undefined;
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
	let template = this.GetTechnologyTemplate(tech);

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

		// Newly created entity, check if any researched techs might apply
		// (only do this for new entities because even if an entity is converted or captured,
		//	we want it to maintain whatever technologies previously applied)
		if (msg.from == -1)
		{
			var modifiedComponents = {};
			for (var name in this.modifications)
			{
				// We only need to find one one tech per component for a match
				var modifications = this.modifications[name];
				var component = name.split("/")[0];
				for (let modif of modifications)
 					if (DoesModificationApply(modif, classes))
					{
						if (!modifiedComponents[component])
							modifiedComponents[component] = [];
						modifiedComponents[component].push(name);
					}
			}

			// Send mesage(s) to the entity so it knows about researched techs
			for (var component in modifiedComponents)
				Engine.PostMessage(msg.entity, MT_ValueModification, { "entities": [msg.entity], "component": component, "valueNames": modifiedComponents[component] });
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

		this.clearModificationCache(msg.entity);
	}
};

// Marks a technology as researched.  Note that this does not verify that the requirements are met.
TechnologyManager.prototype.ResearchTechnology = function(tech)
{
	this.StoppedResearch(tech, false);

	var template = this.GetTechnologyTemplate(tech);

	if (!template)
	{
		error("Tried to research invalid technology: " + uneval(tech));
		return;
	}

	var modifiedComponents = {};
	this.researchedTechs.add(tech);
	// store the modifications in an easy to access structure
	if (template.modifications)
	{
		let derivedModifiers = DeriveModificationsFromTech(template);
		for (let modifierPath in derivedModifiers)
		{
			if (!this.modifications[modifierPath])
				this.modifications[modifierPath] = [];
			this.modifications[modifierPath] = this.modifications[modifierPath].concat(derivedModifiers[modifierPath]);

			let component = modifierPath.split("/")[0];
			if (!modifiedComponents[component])
				modifiedComponents[component] = [];
			modifiedComponents[component].push(modifierPath);
			this.modificationCache[modifierPath] = {};
		}
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

	for (var component in modifiedComponents)
	{
		Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": playerID, "component": component, "valueNames": modifiedComponents[component]});
		Engine.BroadcastMessage(MT_ValueModification, { "entities": ents, "component": component, "valueNames": modifiedComponents[component]});
	}

	if (tech.startsWith("phase") && !template.autoResearch)
	{
		let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGUIInterface.PushNotification({
			"type": "phase",
			"players": [playerID],
			"phaseName": tech,
			"phaseState": "completed"
		});
	}
};

// Clears the cached data for an entity from the modifications cache
TechnologyManager.prototype.clearModificationCache = function(ent)
{
	for (var valueName in this.modificationCache)
		delete this.modificationCache[valueName][ent];
};

// Caching layer in front of ApplyModificationsWorker
//	Note: be careful with the type of curValue, if it should be a numerical
//		value and is derived from template data, you must convert the string
//		from the template to a number using the + operator, before calling
//		this function!
TechnologyManager.prototype.ApplyModifications = function(valueName, curValue, ent)
{
	if (!this.modificationCache[valueName])
		this.modificationCache[valueName] = {};

	if (!this.modificationCache[valueName][ent] || this.modificationCache[valueName][ent].origValue != curValue)
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity)
			return curValue;
		this.modificationCache[valueName][ent] = {
			"origValue": curValue,
			"newValue": GetTechModifiedProperty(this.modifications, cmpIdentity.GetClassesList(), valueName, curValue)
		};
	}

	return this.modificationCache[valueName][ent].newValue;
};

// Alternative version of ApplyModifications, applies to templates instead of entities
TechnologyManager.prototype.ApplyModificationsTemplate = function(valueName, curValue, template)
{
	if (!template || !template.Identity)
		return curValue;
	return GetTechModifiedProperty(this.modifications, GetIdentityClasses(template.Identity), valueName, curValue);
};

// Marks a technology as being queued for research
TechnologyManager.prototype.QueuedResearch = function(tech, researcher)
{
	this.researchQueued[tech] = researcher;
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

// Marks a technology as not being currently researched
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

	delete this.researchQueued[tech];
	this.researchStarted.delete(tech);
};

// Checks whether a technology is set to be researched
TechnologyManager.prototype.IsInProgress = function(tech)
{
	if (this.researchQueued[tech])
		return true;
	else
		return false;
};

/**
 * Returns the names of technologies that are currently being researched (non-queued).
 */
TechnologyManager.prototype.GetStartedTechs = function()
{
	return this.researchStarted;
};

// Gets the entity currently researching a technology
TechnologyManager.prototype.GetResearcher = function(tech)
{
	if (this.researchQueued[tech])
		return this.researchQueued[tech];
	return undefined;
};

// Get helper data for tech modifications
TechnologyManager.prototype.GetTechModifications = function()
{
	return this.modifications;
};

// called by GUIInterface for PlayerData. AI use.
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
