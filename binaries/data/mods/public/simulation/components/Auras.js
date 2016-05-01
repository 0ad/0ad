function Auras() {}

Auras.prototype.Schema =
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>";

Auras.prototype.Init = function()
{
	let cmpDataTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_DataTemplateManager);
	this.auras = {};
	this.affectedPlayers = {};
	let auraNames = this.GetAuraNames();
	for (let name of auraNames)
	{
		this.affectedPlayers[name] = [];
		this.auras[name] = cmpDataTemplateManager.GetAuraTemplate(name);
	}
	// In case of autogarrisoning, this component can be called before ownership is set.
	// So it needs to be completely initialised from the start.
	this.Clean();
};

// We can modify identifier if we want stackable auras in some case.
Auras.prototype.GetModifierIdentifier = function(name)
{
	if (this.auras[name].stackable)
		return name + this.entity;
	return name;
};

Auras.prototype.GetDescriptions = function()
{
	let auraNames = this.GetAuraNames();
	var ret = {};
	for (let name of auraNames)
	{
		let aura = this.auras[name];
		if (aura.auraName)
			ret[aura.auraName] = aura.auraDescription || null;
	}
	return ret;
};

Auras.prototype.GetAuraNames = function()
{
	return this.template._string.split(/\s+/);
};

Auras.prototype.GetOverlayIcon = function(name)
{
	return this.auras[name].overlayIcon || "";
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
	return +this.auras[name].radius;
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

	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
		cmpPlayer = QueryOwnerInterface(this.entity);
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

Auras.prototype.CanApply = function(name)
{
	if (!this.auras[name].requiredTechnology)
		return true;
	let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (!cmpTechnologyManager || !cmpTechnologyManager.IsTechnologyResearched(this.auras[name].requiredTechnology))
		return false;
	return true;
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
	return this.auras[name].type;
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
	let targetUnitsClone = {};
	// remove all bonuses
	for (let name of auraNames)
	{
		targetUnitsClone[name] = [];
		if (!this[name])
			continue;

		if (this[name].targetUnits)
			targetUnitsClone[name] = this[name].targetUnits.slice();

		if (this.IsGlobalAura(name))
			this.RemoveTemplateBonus(name);

		this.RemoveBonus(name, this[name].targetUnits);

		if (this[name].rangeQuery)
			cmpRangeManager.DestroyActiveQuery(this[name].rangeQuery);
	}

	for (let name of auraNames)
	{
		// only calculate the affected players on re-applying the bonuses
		// this makes sure the template bonuses are removed from the correct players
		this.CalculateAffectedPlayers(name);
		// initialise range query
		this[name] = {};
		this[name].targetUnits = [];
		this[name].isApplied = this.CanApply(name);
		var affectedPlayers = this.GetAffectedPlayers(name);

		if (!affectedPlayers.length)
			continue;

		if (!this.IsRangeAura(name))
		{
			this.ApplyBonus(name, targetUnitsClone[name]);
			continue;
		}

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
	if (!this[name].isApplied)
		return;

	if (!this.IsGlobalAura(name))
		return;
	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);

	for (let mod of modifications)
		for (let player of players)
			cmpAuraManager.ApplyTemplateBonus(mod.value, player, classes, mod, this.GetModifierIdentifier(name));
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
	if (!this[name].isApplied)
		return;
	if (!this.IsGlobalAura(name))
		return;

	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	var classes = this.GetClasses(name);
	var players = this.GetAffectedPlayers(name);

	for (let mod of modifications)
		for (let player of players)
			cmpAuraManager.RemoveTemplateBonus(mod.value, player, classes, this.GetModifierIdentifier(name));
};

Auras.prototype.ApplyBonus = function(name, ents)
{
	var validEnts = this.GiveMembersWithValidClass(name, ents);
	if (!validEnts.length)
		return;

	this[name].targetUnits = this[name].targetUnits.concat(validEnts);

	if (!this[name].isApplied)
		return;

	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);

	for (let mod of modifications)
		cmpAuraManager.ApplyBonus(mod.value, validEnts, mod, this.GetModifierIdentifier(name));
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

	if (!this[name].isApplied)
		return;

	var modifications = this.GetModifications(name);
	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);

	for (let mod of modifications)
		cmpAuraManager.RemoveBonus(mod.value, validEnts, this.GetModifierIdentifier(name));

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
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (cmpPlayer && (cmpPlayer.GetPlayerID() == msg.player || cmpPlayer.GetPlayerID() == msg.otherPlayer) ||
	   IsOwnedByPlayer(msg.player, this.entity) ||
	   IsOwnedByPlayer(msg.otherPlayer, this.entity))
		this.Clean();
};

Auras.prototype.OnGlobalResearchFinished = function(msg)
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if ((!cmpPlayer || cmpPlayer.GetPlayerID() != msg.player) && !IsOwnedByPlayer(msg.player, this.entity))
		return;
	let auraNames = this.GetAuraNames();
	let needsClean = false;
	for (let name of auraNames)
	{
		let requiredTech = this.auras[name].requiredTechnology;
		if (requiredTech && requiredTech == msg.tech)
		{
			needsClean = true;
			break;
		}
	}
	if (needsClean)
		this.Clean();
};

Engine.RegisterComponentType(IID_Auras, "Auras", Auras);
