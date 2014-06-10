function Auras() {}

var modificationSchema = 
	"<element name='Modifications' a:help='Modification list'>" +
		"<oneOrMore>" +
			"<element a:help='Name of the value to modify'>" +
				"<anyName/>" +
				"<choice>" +
					"<element name='Add'>" +
						"<data type='decimal'/>" +
					"</element>" +
					"<element name='Multiply'>" +
						"<data type='decimal'/>" +
					"</element>" +
				"</choice>" +
			"</element>" +
		"</oneOrMore>" +
	"</element>";

Auras.prototype.Schema =
	"<oneOrMore>" +
		"<element a:help='Any name you want'>" +
			"<anyName/>" +
			"<interleave>" +
				"<optional>" +
					"<element name='Radius' a:help='Define the radius this aura affects, if it is a range aura'>" +
						"<data type='nonNegativeInteger'/>" +
					"</element>" +
				"</optional>" +
				"<element name='Type' a:help='Controls how this aura affects nearby units'>" +
					"<choice>" +
						"<value a:help='Affects units in the same formation'>formation</value>" +
						"<value a:help='Affects units in a certain range'>range</value>" +
						"<value a:help='Affects the structure or unit this unit is garrisoned in'>garrison</value>" +
						"<value a:help='Affects the units that are garrisoned on a certain structure'>garrisonedUnits</value>" +
						"<value a:help='Affects all units while this unit is alive'>global</value>" +
					"</choice>" +
				"</element>" +
				modificationSchema +
				"<optional>" +
					"<element name='AuraName' a:help='name to display in the GUI'>" +
						"<text/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='AuraDescription' a:help='description to display in the GUI, requires a name'>" +
						"<text/>" +
					"</element>" +
				"</optional>" +
				"<element name='Affects' a:help='Affected classes'>" +
					"<text/>" +
				"</element>" +
				"<optional>" +
					"<element name='AffectedPlayers' a:help='Affected players'>" +
						"<text/>" +
					"</element>" +
				"</optional>" +
			"</interleave>" +
		"</element>" +
	"</oneOrMore>";

Auras.prototype.Init = function()
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	this.templateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	var auraNames = this.GetAuraNames();
	this.auras = {};
	this.affectedPlayers = {};
	var cmpTechnologyTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
	for (var name in this.template)
	{
		this.affectedPlayers[name] = []; // will be calculated on ownership change
		var aura = {}
		aura.affects = this.template[name].Affects;
		if (this.template[name].AffectedPlayers)
			aura.affectedPlayers = this.template[name].AffectedPlayers.split(/\s+/);
		aura.modifications = [];
		for (var value in this.template[name].Modifications)
		{
			var mod = {};
			mod.value = value.replace(/\./g, "/");
			if (this.template[name].Modifications[value].Add)
				mod.add = +this.template[name].Modifications[value].Add;
			else if (this.template[name].Modifications[value].Multiply)
				mod.multiply = +this.template[name].Modifications[value].Multiply;
			aura.modifications.push(mod);
		}
		this.auras[name] = aura;
	}
};

Auras.prototype.GetDescriptions = function()
{
	var ret = {}
	for each (var aura in this.template)
		if (aura.AuraName)
			ret[aura.AuraName] = aura.AuraDescription || null;
	return ret;
};

Auras.prototype.GetAuraNames = function()
{
	return Object.keys(this.template);
};

Auras.prototype.GetRange = function(name)
{
	if (!this.IsRangeAura(name))
		return undefined;
	if (this.IsGlobalAura(name))
		return -1; // -1 is infinite range
	return +this.template[name].Radius;
};

Auras.prototype.GetClasses = function(name)
{
	return this.auras[name].affects;
};

Auras.prototype.GetModifications = function(name)
{
	return this.auras[name].modifications;
};

Auras.prototype.GetAffectedPlayers = function(name)
{
	return this.affectedPlayers[name];
};

Auras.prototype.CalculateAffectedPlayers = function(name)
{
	if (this.auras[name].affectedPlayers)
		var affectedPlayers = this.auras[name].affectedPlayers;
	else
		var affectedPlayers = ["Player"];

	this.affectedPlayers[name] = [];

	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	if (!cmpPlayer)
		return;

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var numPlayers = cmpPlayerManager.GetNumPlayers();

	for (var i = 0; i < numPlayers; ++i)
	{
		for each (var p in affectedPlayers)
		{
			if (p == "Player" ? cmpPlayer.GetPlayerID() == i : cmpPlayer["Is" + p](i))
			{
				this.affectedPlayers[name].push(i);
				break;
			}
		}
	}
};

Auras.prototype.HasFormationAura = function()
{
	return this.GetAuraNames().some(this.IsFormationAura.bind(this));
};

Auras.prototype.HasGarrisonAura = function()
{
	return this.GetAuraNames().some(this.IsGarrisonAura.bind(this));
};

Auras.prototype.HasGarrisonedUnitsAura = function()
{
	return this.GetAuraNames().some(this.IsGarrisonedUnitsAura.bind(this));
};

Auras.prototype.GetType = function(name)
{
	return this.template[name].Type;
};

Auras.prototype.IsFormationAura = function(name)
{
	return this.GetType(name) == "formation";
};

Auras.prototype.IsGarrisonAura = function(name)
{
	return this.GetType(name) == "garrison";
};

Auras.prototype.IsGarrisonedUnitsAura = function(name)
{
	return this.GetType(name) == "garrisonedUnits";
};

Auras.prototype.IsRangeAura = function(name)
{
	// A global aura is also treated as a range aura with infinite range.
	return ["range", "global"].indexOf(this.GetType(name)) != -1;
};

Auras.prototype.IsGlobalAura = function(name)
{
	return this.GetType(name) == "global";
};

/**
 * clean all bonuses. Remove the old ones and re-apply the new ones
 */
Auras.prototype.Clean = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var auraNames = this.GetAuraNames();
	// remove all bonuses
	for each (var name in auraNames)
	{
		if (!this[name])
			continue;

		if (this.IsGlobalAura(name))
			this.RemoveTemplateBonus(name);

		for each(var ent in this[name].targetUnits)
			this.RemoveBonus(name, ent);

		if (this[name].rangeQuery)
			cmpRangeManager.DestroyActiveQuery(this[name].rangeQuery);
	}

	for each (var name in auraNames)
	{
		// only calculate the affected players on re-applying the bonuses
		// this makes sure the template bonuses are removed from the correct players
		this.CalculateAffectedPlayers(name);
		// initialise range query
		this[name] = {};
		this[name].targetUnits = [];
		var affectedPlayers = this.GetAffectedPlayers(name);

		if (!affectedPlayers.length)
			continue;

		if (!this.IsRangeAura(name))
			continue;

		this[name].rangeQuery = cmpRangeManager.CreateActiveQuery(
		    this.entity,
		    0,
		    this.GetRange(name),
		    affectedPlayers,
		    IID_Identity,
		    cmpRangeManager.GetEntityFlagMask("normal")
		);
		cmpRangeManager.EnableActiveQuery(this[name].rangeQuery);

		if (this.IsGlobalAura(name))
		{
			// update stats in for all templates 
			this.ApplyTemplateBonus(name, affectedPlayers);
			// Add self to your own query for consistency with templates.
			this.OnRangeUpdate({"tag":this[name].rangeQuery, "added":[this.entity], "removed":[]});
		}
	}
};

Auras.prototype.GiveMembersWithValidClass = function(auraName, entityList)
{
	var match = this.GetClasses(auraName);
	var r = [];
	for (var ent of entityList)
	{
		var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), match))
			r.push(ent);
	}
	return r;
}

Auras.prototype.OnRangeUpdate = function(msg)
{
	var auraNames = this.GetAuraNames();
	for each (var n in auraNames)
	{
		if (msg.tag == this[n].rangeQuery)
		{
			var name = n;
			break;
		}
	}

	if (!name)
		return;

	var targetUnits = this[name].targetUnits;
	var classes = this.GetClasses(name);

	if (msg.added.length > 0)
	{
		var validList = this.GiveMembersWithValidClass(name, msg.added);
		for each (var e in validList)
		{
			targetUnits.push(e);
			this.ApplyBonus(name, e);
		}
	}

	if (msg.removed.length > 0)
	{
		for each (var e in msg.removed)
		{
			targetUnits.splice(targetUnits.indexOf(e), 1);
			this.RemoveBonus(name, e);
		}
	}

};

Auras.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	var auraNames = this.GetAuraNames();
	for each (var name in auraNames)
	{
		if (!this.IsGarrisonedUnitsAura(name))
			continue;
		for each (var ent in msg.added)
			this.ApplyBonus(name, ent);
		for each (var ent in msg.removed)
			this.RemoveBonus(name, ent);
	}
};

Auras.prototype.ApplyFormationBonus = function(memberList)
{
	var auraNames = this.GetAuraNames();
	for each (var name in auraNames)
	{
		if (!this.IsFormationAura(name))
			continue;

		var validList = this.GiveMembersWithValidClass(name, memberList);
		for each (var ent in validList)
		{
			this[name].targetUnits.push(ent);
			this.ApplyBonus(name,ent);
		}
	}
};

Auras.prototype.ApplyGarrisonBonus = function(structure)
{
	var auraNames = this.GetAuraNames();
	for each (var name in auraNames)
	{
		if (!this.IsGarrisonAura(name))
			continue;

		var validList = this.GiveMembersWithValidClass(name, [structure]);
		if (validList.length)
		{
			this[name].targetUnits.push(validList[0]);
			this.ApplyBonus(name,validList[0]);
		}
	}
};

Auras.prototype.ApplyTemplateBonus = function(name, players)
{
	if (!this.IsGlobalAura(name))
		return;
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);

	for each (var mod in modifications)
		for each (var player in players)
			cmpAuraManager.ApplyTemplateBonus(mod.value, player, classes, mod, this.templateName + "/" + name + "/" + mod.value);
};

Auras.prototype.RemoveFormationBonus = function(memberList)
{
	var auraNames = this.GetAuraNames();
	for each (var name in auraNames)
	{
		if (!this.IsFormationAura(name))
			continue;

		for each (var ent in memberList)
		{
			this.RemoveBonus(name,ent);
			this[name].targetUnits.splice(this[name].targetUnits.indexOf(ent), 1);
		}
	}
};

Auras.prototype.RemoveGarrisonBonus = function(structure)
{
	var auraNames = this.GetAuraNames();
	for each (var name in auraNames)
	{
		if (!this.IsGarrisonAura(name))
			continue;

		this.RemoveBonus(name,structure);
		this[name].targetUnits.splice(this[name].targetUnits.indexOf(structure), 1);
	}
};

Auras.prototype.RemoveTemplateBonus = function(name)
{
	if (!this.IsGlobalAura(name))
		return;

	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);
	var players =  this.GetAffectedPlayers(name);

	for each (var mod in modifications)
		for each (var player in players)
			cmpAuraManager.RemoveTemplateBonus(mod.value, player, classes, this.templateName + "/" + name + "/" + mod.value);
};

Auras.prototype.ApplyBonus = function(name, ent)
{
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	for each (var mod in modifications)
		cmpAuraManager.ApplyBonus(mod.value, ent, mod, this.templateName + "/" + name + "/" + mod.value);

};

Auras.prototype.RemoveBonus = function(name, ent)
{
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);

	for each (var mod in modifications)
		cmpAuraManager.RemoveBonus(mod.value, ent, this.templateName + "/" + name + "/" + mod.value);
};

Auras.prototype.OnOwnershipChanged = function(msg)
{
	this.Clean();
};

Auras.prototype.OnDiplomacyChanged = function(msg)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && cmpOwnership.GetOwner() == msg.player)
		this.Clean();
};

Auras.prototype.OnValueModification = function(msg)
{
	if (msg.component == "Auras")
		this.Clean();
};

Engine.RegisterComponentType(IID_Auras, "Auras", Auras);
