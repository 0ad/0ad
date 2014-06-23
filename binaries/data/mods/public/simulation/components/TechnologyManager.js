function TechnologyManager() {}

TechnologyManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

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

TechnologyManager.prototype.Init = function ()
{
	var cmpTechTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
	this.allTechs = cmpTechTempMan.GetAllTechs();
	this.researchedTechs = {}; // technologies which have been researched
	this.researchQueued = {};  // technologies which are queued for research
	this.researchStarted = {}; // technologies which are being researched currently (non-queued)

	// This stores the modifications to unit stats from researched technologies
	// Example data: {"ResourceGatherer/Rates/food.grain": [ 
	//                     {"multiply": 1.15, "affects": ["Female", "Infantry Swordsman"]},
	//                     {"add": 2}
	//                 ]}
	this.modifications = {};
	this.modificationCache = {}; // Caches the values after technologies have been applied
	                             // e.g. { "Attack/Melee/Hack" : {5: {"origValue": 8, "newValue": 10}, 7: {"origValue": 9, "newValue": 12}, ...}, ...}
	                             // where 5 and 7 are entity id's

	this.typeCounts = {}; // stores the number of entities of each type 
	this.classCounts = {}; // stores the number of entities of each Class
	this.typeCountsByClass = {}; // stores the number of entities of each type for each class i.e.
	                             // {"someClass": {"unit/spearman": 2, "unit/cav": 5} "someOtherClass":...}

	// Some technologies are automatically researched when their conditions are met.  They have no cost and are 
	// researched instantly.  This allows civ bonuses and more complicated technologies.
	this.autoResearchTech = {};
	for (var key in this.allTechs)
	{
		if (this.allTechs[key].autoResearch || this.allTechs[key].top)
			this.autoResearchTech[key] = this.allTechs[key];
	}
};

TechnologyManager.prototype.OnUpdate = function () 
{
	this.UpdateAutoResearch();
}


// This function checks if the requirements of any autoresearch techs are met and if they are it researches them
TechnologyManager.prototype.UpdateAutoResearch = function ()
{
	for (var key in this.autoResearchTech)
	{
		if ((this.allTechs[key].autoResearch && this.CanResearch(key))
			|| (this.allTechs[key].top && (this.IsTechnologyResearched(this.allTechs[key].top) || this.IsTechnologyResearched(this.allTechs[key].bottom))))
		{
			delete this.autoResearchTech[key];
			this.ResearchTechnology(key);
			return; // We will have recursively handled any knock-on effects so can just return
		}
	}
}

TechnologyManager.prototype.GetTechnologyTemplate = function (tech)
{
	if (!(tech in this.allTechs))
		return undefined;

	return this.allTechs[tech];
};

// Checks an entity template to see if its technology requirements have been met
TechnologyManager.prototype.CanProduce = function (templateName)
{
	var cmpTempManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var template = cmpTempManager.GetTemplate(templateName);

	if (template.Identity && template.Identity.RequiredTechnology)
		return this.IsTechnologyResearched(template.Identity.RequiredTechnology);
	else
		return true; // If there is no required technology then this entity can be produced
};

TechnologyManager.prototype.IsTechnologyResearched = function (tech)
{
	return (this.researchedTechs[tech] !== undefined);
};

// Checks the requirements for a technology to see if it can be researched at the current time
TechnologyManager.prototype.CanResearch = function (tech)
{
	var template = this.GetTechnologyTemplate(tech);
	if (!template)
	{
		warn("Technology \"" + tech + "\" does not exist");
		return false;
	}

	// The technology which this technology supersedes is required
	if (template.supersedes && !this.IsTechnologyResearched(template.supersedes))
		return false;

	if (template.top && this.IsInProgress(template.top) ||
	    template.bottom && this.IsInProgress(template.bottom))
		return false;

	if (template.pair && !this.CanResearch(template.pair))
		return false;

	if (this.IsInProgress(tech))
		return false;

	if (this.IsTechnologyResearched(tech))
		return false;

	return this.CheckTechnologyRequirements(template.requirements || null);
};

// Private function for checking a set of requirements is met
TechnologyManager.prototype.CheckTechnologyRequirements = function (reqs)
{
	// If there are no requirements then all requirements are met
	if (!reqs)
		return true;

	if (reqs.tech)
	{
		return this.IsTechnologyResearched(reqs.tech);
	}
	else if (reqs.all)
	{
		for (var i = 0; i < reqs.all.length; i++)
		{
			if (!this.CheckTechnologyRequirements(reqs.all[i]))
				return false;
		}
		return true;
	}
	else if (reqs.any)
	{
		for (var i = 0; i < reqs.any.length; i++)
		{
			if (this.CheckTechnologyRequirements(reqs.any[i]))
				return true;
		}
		return false;
	}
	else if (reqs.class)
	{
		if (reqs.numberOfTypes)
		{
			if (this.typeCountsByClass[reqs.class])
				return (reqs.numberOfTypes <= Object.keys(this.typeCountsByClass[reqs.class]).length);
			else
				return false;
		}
		else if (reqs.number)
		{
			if (this.classCounts[reqs.class])
				return (reqs.number <= this.classCounts[reqs.class]);
			else
				return false;
		}
	}
	else if (reqs.civ) 
	{
		var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		if (cmpPlayer && cmpPlayer.GetCiv() == reqs.civ) 
			return true;
		else 
			return false;
	}

	// The technologies requirements are not a recognised format
	error("Bad requirements " + uneval(reqs));
	return false;
};

TechnologyManager.prototype.OnGlobalOwnershipChanged = function (msg)
{
	// This automatically updates typeCounts, classCounts and typeCountsByClass
	var playerID = (Engine.QueryInterface(this.entity, IID_Player)).GetPlayerID();
	if (msg.to == playerID)
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var template = cmpTemplateManager.GetCurrentTemplateName(msg.entity);

		this.typeCounts[template] = this.typeCounts[template] || 0;
		this.typeCounts[template] += 1;

		var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
		if (!cmpIdentity)
			return;

		var classes = cmpIdentity.GetClassesList();
		// don't use foundations for the class counts but check if techs apply (e.g. health increase)
		if (!Engine.QueryInterface(msg.entity, IID_Foundation))
		{
			for (var i in classes)
			{
				this.classCounts[classes[i]] = this.classCounts[classes[i]] || 0;
				this.classCounts[classes[i]] += 1;
			
				this.typeCountsByClass[classes[i]] = this.typeCountsByClass[classes[i]] || {};
				this.typeCountsByClass[classes[i]][template] = this.typeCountsByClass[classes[i]][template] || 0;
				this.typeCountsByClass[classes[i]][template] += 1;
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
				for (var i in modifications)
 					if (DoesModificationApply(modifications[i], classes))
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
		
		this.typeCounts[template] -= 1;
		if (this.typeCounts[template] <= 0)
			delete this.typeCounts[template];

		// don't use foundations for the class counts
		if (!Engine.QueryInterface(msg.entity, IID_Foundation))
		{
			var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
			if (cmpIdentity)
			{
				var classes = cmpIdentity.GetClassesList();
				for (var i in classes)
				{
					this.classCounts[classes[i]] -= 1;
					if (this.classCounts[classes[i]] <= 0)
						delete this.classCounts[classes[i]];

					this.typeCountsByClass[classes[i]][template] -= 1;
					if (this.typeCountsByClass[classes[i]][template] <= 0)
						delete this.typeCountsByClass[classes[i]][template];
				}
			}
		}

		this.clearModificationCache(msg.entity);
	}
};

// Marks a technology as researched.  Note that this does not verify that the requirements are met.
TechnologyManager.prototype.ResearchTechnology = function (tech)
{
	this.StoppedResearch(tech); // The tech is no longer being currently researched

	var template = this.GetTechnologyTemplate(tech);

	if (!template)
	{
		error("Tried to research invalid techonology: " + uneval(tech));
		return;
	}

	var modifiedComponents = {};
	this.researchedTechs[tech] = template;
	// store the modifications in an easy to access structure
	if (template.modifications)
	{
		var affects = [];
		if (template.affects && template.affects.length > 0)
		{
			for (var i in template.affects)
			{
				// Put the list of classes into an array for convenient access
				affects.push(template.affects[i].split(/\s+/));
			}
		}
		else
		{
			affects.push([]);
		}

		// We add an item to this.modifications for every modification in the template.modifications array
		for (var i in template.modifications)
		{
			var modification = template.modifications[i];
			if (!this.modifications[modification.value])
				this.modifications[modification.value] = [];
			
			var modAffects = affects.slice();
			if (modification.affects)
			{
				var extraAffects = modification.affects.split(/\s+/);
				for (var a in modAffects)
					modAffects[a] = modAffects[a].concat(extraAffects);
			}

			var mod = {"affects": modAffects};

			// copy the modification data into our new data structure
			for (var j in modification)
				if (j !== "value" && j !== "affects")
					mod[j] = modification[j];

			this.modifications[modification.value].push(mod);
			var component = modification.value.split("/")[0];
			if (!modifiedComponents[component])
				modifiedComponents[component] = [];
			modifiedComponents[component].push(modification.value);
			this.modificationCache[modification.value] = {};
		}
	}

	this.UpdateAutoResearch();

	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer || cmpPlayer.GetPlayerID() === undefined)
		return;
	var playerID = cmpPlayer.GetPlayerID();
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var ents = cmpRangeManager.GetEntitiesByPlayer(playerID);

	// Call the related trigger event 
	var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.CallEvent("ResearchFinished", {"player": playerID, "tech": tech});
	
	for (var component in modifiedComponents)
	{
		Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": playerID, "component": component, "valueNames": modifiedComponents[component]});
		Engine.BroadcastMessage(MT_ValueModification, { "entities": ents, "component": component, "valueNames": modifiedComponents[component]});
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
		this.modificationCache[valueName][ent] = {"origValue": curValue};
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var templateName = cmpTemplateManager.GetCurrentTemplateName(ent);
		// Ensure that preview or construction entites have the same properties as the final building
		if (templateName.indexOf("preview|") > -1 || templateName.indexOf("construction|") > -1 )
			templateName = templateName.slice(templateName.indexOf("|") + 1);
		this.modificationCache[valueName][ent].newValue = GetTechModifiedProperty(this.modifications, cmpTemplateManager.GetTemplate(templateName), valueName, curValue);
	}

	return this.modificationCache[valueName][ent].newValue;
};

// Alternative version of ApplyModifications, applies to templates instead of entities
TechnologyManager.prototype.ApplyModificationsTemplate = function(valueName, curValue, template)
{
	return GetTechModifiedProperty(this.modifications, template, valueName, curValue);
};

// Marks a technology as being queued for research
TechnologyManager.prototype.QueuedResearch = function (tech, researcher)
{
	this.researchQueued[tech] = researcher;
};

// Marks a technology as actively being researched
TechnologyManager.prototype.StartedResearch = function (tech)
{
	this.researchStarted[tech] = true;
};

// Marks a technology as not being currently researched
TechnologyManager.prototype.StoppedResearch = function (tech)
{
	delete this.researchQueued[tech];
	delete this.researchStarted[tech];
};

// Checks whether a technology is set to be researched
TechnologyManager.prototype.IsInProgress = function(tech)
{
	if (this.researchQueued[tech])
		return true;
	else
		return false;
};

// Get all techs that are currently being researched
TechnologyManager.prototype.GetTechsStarted = function()
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
TechnologyManager.prototype.GetStartedResearch = function()
{
	return this.researchStarted;
};
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
