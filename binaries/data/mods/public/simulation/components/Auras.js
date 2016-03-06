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
				"<optional>" +
					"<element name='OverlayIcon' a:help='Icon to show on the entities affected by this aura'>" +
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
	this.auras = {};
	this.affectedPlayers = {};
	for (var name in this.template)
	{
		this.affectedPlayers[name] = []; // will be calculated on ownership change
		var aura = {};
		aura.affects = this.template[name].Affects;
		if (this.template[name].AffectedPlayers)
			aura.affectedPlayers = this.template[name].AffectedPlayers.split(/\s+/);
		this.auras[name] = aura;
	}
};

Auras.prototype.GetModifierIdentifier = function(name, mod)
{
		return this.templateName + "/" + name + "/" + mod.value;
};

Auras.prototype.GetDescriptions = function()
{
	var ret = {};
	for (let name in this.template)
	{
		let aura = this.template[name];
		if (aura.AuraName)
			ret[aura.AuraName] = aura.AuraDescription || null;
	}
	return ret;
};

Auras.prototype.GetAuraNames = function()
{
	return Object.keys(this.template);
};

Auras.prototype.GetOverlayIcon = function(name)
{
	return this.template[name].OverlayIcon || "";
};

Auras.prototype.GetAffectedEntities = function(name)
{
	return this[name].targetUnits;
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
	var affectedPlayers = this.auras[name].affectedPlayers || ["Player"];
	this.affectedPlayers[name] = [];

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;

	var numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	for (var i = 0; i < numPlayers; ++i)
	{
		for (let p of affectedPlayers)
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
	return this.GetAuraNames().some(n => this.IsFormationAura(n));
};

Auras.prototype.HasGarrisonAura = function()
{
	return this.GetAuraNames().some(n => this.IsGarrisonAura(n));
};

Auras.prototype.HasGarrisonedUnitsAura = function()
{
	return this.GetAuraNames().some(n => this.IsGarrisonedUnitsAura(n));
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
	for (let name of auraNames)
	{
		if (!this[name])
			continue;

		if (this.IsGlobalAura(name))
			this.RemoveTemplateBonus(name);

		this.RemoveBonus(name, this[name].targetUnits);

		if (this[name].rangeQuery)
			cmpRangeManager.DestroyActiveQuery(this[name].rangeQuery);
	}

	for (let name in this.template)
	{
		let modifications = [];
		for (let value in this.template[name].Modifications)
		{
			let mod = {};
			mod.value = value.replace(/\./g, "/").replace(/\/\//g, ".");
			let templateModifications = this.template[name].Modifications[value];
			if (templateModifications.Add)
				mod.add = ApplyValueModificationsToEntity("Auras/"+name+"/Modifications/"+mod.value+"/Add",
					+templateModifications.Add, this.entity);
			else if (templateModifications.Multiply)
				mod.multiply = ApplyValueModificationsToEntity("Auras/"+name+"/Modifications/"+mod.value+"/Multiply",
					+templateModifications.Multiply, this.entity);
			modifications.push(mod);
		}
		this.auras[name].modifications = modifications;
	}

	for (let name of auraNames)
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
			this.ApplyTemplateBonus(name, affectedPlayers);

			// Add self to your own query for consistency with templates.
			this.OnRangeUpdate({
				"tag": this[name].rangeQuery,
				"added": [this.entity],
				"removed": []
			});
		}
	}
};

Auras.prototype.GiveMembersWithValidClass = function(auraName, entityList)
{
	var match = this.GetClasses(auraName);
	return entityList.filter(ent => {
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), match);
	});
};

Auras.prototype.OnRangeUpdate = function(msg)
{
	var auraNames = this.GetAuraNames().filter(n => this[n] && msg.tag == this[n].rangeQuery);
	for (let name of auraNames)
	{
		this.ApplyBonus(name, msg.added);
		this.RemoveBonus(name, msg.removed);
	}
};

Auras.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	var auraNames = this.GetAuraNames().filter(n => this.IsGarrisonedUnitsAura(n));
	for (let name of auraNames)
	{
		this.ApplyBonus(name, msg.added);
		this.RemoveBonus(name, msg.removed);
	}
};

Auras.prototype.ApplyFormationBonus = function(memberList)
{
	var auraNames = this.GetAuraNames().filter(n => this.IsFormationAura(n));
	for (let name of auraNames)
		this.ApplyBonus(name, memberList);
};

Auras.prototype.ApplyGarrisonBonus = function(structure)
{
	var auraNames = this.GetAuraNames().filter(n => this.IsGarrisonAura(n));
	for (let name of auraNames)
		this.ApplyBonus(name, [structure]);
};

Auras.prototype.ApplyTemplateBonus = function(name, players)
{
	if (!this.IsGlobalAura(name))
		return;
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);

	for (let mod of modifications)
		for (let player of players)
			cmpAuraManager.ApplyTemplateBonus(mod.value, player, classes, mod, this.GetModifierIdentifier(name, mod));
};

Auras.prototype.RemoveFormationBonus = function(memberList)
{
	var auraNames = this.GetAuraNames().filter(n => this.IsFormationAura(n));
	for (let name of auraNames)
		this.RemoveBonus(name, memberList);
};

Auras.prototype.RemoveGarrisonBonus = function(structure)
{
	var auraNames = this.GetAuraNames().filter(n => this.IsGarrisonAura(n));
	for (let name of auraNames)
		this.RemoveBonus(name, [structure]);
};

Auras.prototype.RemoveTemplateBonus = function(name)
{
	if (!this.IsGlobalAura(name))
		return;

	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);
	var players = this.GetAffectedPlayers(name);

	for (let mod of modifications)
		for (let player of players)
			cmpAuraManager.RemoveTemplateBonus(mod.value, player, classes, this.GetModifierIdentifier(name, mod));
};

Auras.prototype.ApplyBonus = function(name, ents)
{
	var validEnts = this.GiveMembersWithValidClass(name, ents);
	if (!validEnts.length)
		return;

	this[name].targetUnits = this[name].targetUnits.concat(validEnts);
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);

	for (let mod of modifications)
		cmpAuraManager.ApplyBonus(mod.value, validEnts, mod, this.GetModifierIdentifier(name, mod));

	// update status bars if this has an icon
	if (!this.GetOverlayIcon(name))
		return;

	for (let ent of validEnts)
	{
		var cmpStatusBars = Engine.QueryInterface(ent, IID_StatusBars);
		if (cmpStatusBars)
			cmpStatusBars.AddAuraSource(this.entity, name);
	}
};

Auras.prototype.RemoveBonus = function(name, ents)
{
	var validEnts = this.GiveMembersWithValidClass(name, ents);
	if (!validEnts.length)
		return;

	this[name].targetUnits = this[name].targetUnits.filter(v => validEnts.indexOf(v) == -1);
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);

	for (let mod of modifications)
		cmpAuraManager.RemoveBonus(mod.value, validEnts, this.GetModifierIdentifier(name, mod));

	// update status bars if this has an icon
	if (!this.GetOverlayIcon(name))
		return;

	for (let ent of validEnts)
	{
		var cmpStatusBars = Engine.QueryInterface(ent, IID_StatusBars);
		if (cmpStatusBars)
			cmpStatusBars.RemoveAuraSource(this.entity, name);
	}
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
