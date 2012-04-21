function TechnologyManager() {}

TechnologyManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

TechnologyManager.prototype.Init = function ()
{
	var cmpTechTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
	this.allTechs = cmpTechTempMan.GetAllTechs();
	this.researchedTechs = {}; // technologies which have been researched
	this.inProgressTechs = {}; // technologies which are being researched currently
	
	// This stores the modifications to unit stats from researched technologies
	// Example data: {"ResourceGatherer/Rates/food.grain": [ 
	//                     {"multiplier": 1.15, "affects": ["Female", "Infantry Swordsman"]},
	//                     {"add": 2}
	//                 ]}
	this.modifications = {};
	
	this.typeCounts = {}; // stores the number of entities of each type 
	this.classCounts = {}; // stores the number of entities of each Class
	this.typeCountsByClass = {}; // stores the number of entities of each type for each class i.e.
	                             // {"someClass": {"unit/spearman": 2, "unit/cav": 5} "someOtherClass":...}
	
	// Some technologies are automatically researched when their conditions are met.  They have no cost and are 
	// researched instantly.  This allows civ bonuses and more complicated technologies.
	this.autoResearchTech = {};
	for (var key in this.allTechs)
	{
		if (this.allTechs[key].autoResearch)
			this.autoResearchTech[key] = this.allTechs[key];
	}
	
	this.UpdateAutoResearch();
};

// This function checks if the requirements of any autoresearch techs are met and if they are it researches them
TechnologyManager.prototype.UpdateAutoResearch = function ()
{
	for (var key in this.autoResearchTech)
	{
		if (this.CanResearch(key))
		{
			delete this.autoResearchTech[key];
			this.ResearchTechnology(key);
			return; // We will have recursively handled any knock-on effects so can just return
		}
	}
}

TechnologyManager.prototype.GetTechnologyTemplate = function (tech)
{
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
	
	return this.CheckTechnologyRequirements(template.requirements);
};

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
		if (reqs.numberOfTypes == 0) // silly case but handle it anyway
			return true;
		
		if (this.typeCountsByClass[reqs.class])
			return (reqs.numberOfTypes <= Object.keys(this.typeCountsByClass[reqs.class]).length);
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
		
		// don't use foundations for the class counts
		if (Engine.QueryInterface(msg.entity, IID_Foundation))
			return;
		
		var cmpIdentity = Engine.QueryInterface(msg.entity, IID_Identity);
		if (cmpIdentity)
		{
			var classes = cmpIdentity.GetClassesList();
			for (var i in classes)
			{
				this.classCounts[classes[i]] = this.classCounts[classes[i]] || 0;
				this.classCounts[classes[i]] += 1;
				
				this.typeCountsByClass[classes[i]] = this.typeCountsByClass[classes[i]] || {};
				this.typeCountsByClass[classes[i]][template] = this.typeCountsByClass[classes[i]][template] || 0;
				this.typeCountsByClass[classes[i]][template] += 1;
			}
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
		if (Engine.QueryInterface(msg.entity, IID_Foundation))
			return;
		
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
			
			var mod = {"affects": affects};
			// copy the modification data into our new data structure
			for (var j in modification)
				if (j !== "value")
					mod[j] = modification[j];
			
			this.modifications[modification.value].push(mod);
			modifiedComponents[modification.value.split("/")[0]] = true;
		}
	}
	
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	var player = cmpPlayer.GetPlayerID();
	
	for (var component in modifiedComponents)
		Engine.BroadcastMessage(MT_TechnologyModificationChange, { "component": component, "player": player });
	
	this.UpdateAutoResearch();
};

TechnologyManager.prototype.ApplyModifications = function(valueName, curValue, ent)
{	
	// Get all modifications to this value
	var modifications = this.modifications[valueName];
	if (!modifications) // no modifications so return the original value
		return curValue;
	
	// Get the classes which this entity belongs to
	var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
	var classes = cmpIdentity.GetClassesList();
	
	var retValue = +curValue;
	
	for (var i in modifications)
	{
		var modification = modifications[i];
		var applies = false;
		// See if any of the lists of classes matches this entity
		for (var j in modification.affects)
		{
			var hasAllClasses = true;
			// Check each class in affects is present for the entity
			for (var k in modification.affects[j])
				hasAllClasses = hasAllClasses && (classes.indexOf(modification.affects[j][k]) !== -1);
			
			if (hasAllClasses)
			{
				applies = true;
				break;
			}
		}
		
		// We found a match, apply the modification
		if (applies)
		{
			// Nothing is cumulative so that ordering doesn't matter as much as possible
			if (modification.multiplier)
				retValue += (modification.multiplier - 1) * +curValue;
			else if (modification.add)
				retValue += modification.add;
			else if (modification.replace) // This will depend on ordering because there is no choice
				retValue = modification.replace;
			else
				warn("modification format not recognised (modifying " + valueName + "): " + uneval(modification));
		}
	}
	
	return retValue;
};

// Marks a technology as being currently researched
TechnologyManager.prototype.StartedResearch = function (tech)
{
	this.inProgressTechs[tech] = true;
};

// Marks a technology as not being currently researched
TechnologyManager.prototype.StoppedResearch = function (tech)
{
	delete this.inProgressTechs[tech];
};

// Checks whether a technology is being currently researched
TechnologyManager.prototype.IsInProgress = function(tech)
{
	if (this.inProgressTechs[tech])
		return true;
	else
		return false;
};

Engine.RegisterComponentType(IID_TechnologyManager, "TechnologyManager", TechnologyManager);
