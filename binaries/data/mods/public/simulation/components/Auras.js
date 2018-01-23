function Auras() {}

Auras.prototype.Schema =
	"<attribute name='datatype'>" +
		"<value>tokens</value>" +
	"</attribute>" +
	"<text a:help='A whitespace-separated list of aura files, placed under simulation/data/auras/'/>";

Auras.prototype.Init = function()
{
	this.affectedPlayers = {};

	for (let name of this.GetAuraNames())
		this.affectedPlayers[name] = [];

	// In case of autogarrisoning, this component can be called before ownership is set.
	// So it needs to be completely initialised from the start.
	this.Clean();
};

// We can modify identifier if we want stackable auras in some case.
Auras.prototype.GetModifierIdentifier = function(name)
{
	if (AuraTemplates.Get(name).stackable)
		return name + this.entity;
	return name;
};

Auras.prototype.GetDescriptions = function()
{
	var ret = {};
	for (let auraID of this.GetAuraNames())
	{
		let aura = AuraTemplates.Get(auraID);
		ret[auraID] = {
			"name": aura.auraName,
			"description": aura.auraDescription || null,
			"radius": this.GetRange(auraID) || null
		};
	}
	return ret;
};

Auras.prototype.GetAuraNames = function()
{
	return this.template._string.split(/\s+/);
};

Auras.prototype.GetOverlayIcon = function(name)
{
	return AuraTemplates.Get(name).overlayIcon || "";
};

Auras.prototype.GetAffectedEntities = function(name)
{
	return this[name].targetUnits;
};

Auras.prototype.GetRange = function(name)
{
	if (this.IsRangeAura(name))
		return +AuraTemplates.Get(name).radius;
	return undefined;
};

Auras.prototype.GetClasses = function(name)
{
	return AuraTemplates.Get(name).affects;
};

Auras.prototype.GetModifications = function(name)
{
	return AuraTemplates.Get(name).modifications;
};

Auras.prototype.GetAffectedPlayers = function(name)
{
	return this.affectedPlayers[name];
};

Auras.prototype.GetRangeOverlays = function()
{
	let rangeOverlays = [];

	for (let name of this.GetAuraNames())
	{
		if (!this.IsRangeAura(name) || !this[name].isApplied)
			continue;

		let rangeOverlay = AuraTemplates.Get(name).rangeOverlay;

		rangeOverlays.push(
			rangeOverlay ?
				{
					"radius": this.GetRange(name),
					"texture": rangeOverlay.lineTexture,
					"textureMask": rangeOverlay.lineTextureMask,
					"thickness": rangeOverlay.lineThickness
				} :
				// Specify default in order not to specify it in about 40 auras
				{
					"radius": this.GetRange(name),
					"texture":  "outline_border.png",
					"textureMask":  "outline_border_mask.png",
					"thickness":  0.2
				});
	}

	return rangeOverlays;
};

Auras.prototype.CalculateAffectedPlayers = function(name)
{
	var affectedPlayers = AuraTemplates.Get(name).affectedPlayers || ["Player"];
	this.affectedPlayers[name] = [];

	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
		cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer || cmpPlayer.GetState() == "defeated")
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
	if (!AuraTemplates.Get(name).requiredTechnology)
		return true;

	let cmpTechnologyManager = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (!cmpTechnologyManager)
		return false;

	return cmpTechnologyManager.IsTechnologyResearched(AuraTemplates.Get(name).requiredTechnology);
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
	return AuraTemplates.Get(name).type;
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
	return this.GetType(name) == "range";
};

Auras.prototype.IsGlobalAura = function(name)
{
	return this.GetType(name) == "global" || this.GetType(name) == "player";
};

Auras.prototype.IsPlayerAura = function(name)
{
	return this.GetType(name) == "player";
};

/**
 * clean all bonuses. Remove the old ones and re-apply the new ones
 */
Auras.prototype.Clean = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var auraNames = this.GetAuraNames();
	let targetUnitsClone = {};
	let needVisualizationUpdate = false;
	// remove all bonuses
	for (let name of auraNames)
	{
		targetUnitsClone[name] = [];
		if (!this[name])
			continue;

		if (this.IsRangeAura(name))
			needVisualizationUpdate = true;

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

		if (this.IsGlobalAura(name))
		{
			for (let player of affectedPlayers)
			{
				this.ApplyTemplateBonus(name, affectedPlayers);
				if (this.IsPlayerAura(name))
				{
					let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
					let playerEnts = affectedPlayers.map(player => cmpPlayerManager.GetPlayerByID(player));
					this.ApplyBonus(name, playerEnts);
				}
				else
					this.ApplyBonus(name, cmpRangeManager.GetEntitiesByPlayer(player));
			}
			continue;
		}

		if (!this.IsRangeAura(name))
		{
			this.ApplyBonus(name, targetUnitsClone[name]);
			continue;
		}

		needVisualizationUpdate = true;

		if (this[name].isApplied)
		{
			this[name].rangeQuery = cmpRangeManager.CreateActiveQuery(
			    this.entity,
			    0,
			    this.GetRange(name),
			    affectedPlayers,
			    IID_Identity,
			    cmpRangeManager.GetEntityFlagMask("normal")
			);
			cmpRangeManager.EnableActiveQuery(this[name].rangeQuery);
		}
	}

	if (needVisualizationUpdate)
	{
		let cmpRangeOverlayManager = Engine.QueryInterface(this.entity, IID_RangeOverlayManager);
		if (cmpRangeOverlayManager)
		{
			cmpRangeOverlayManager.UpdateRangeOverlays("Auras");
			cmpRangeOverlayManager.RegenerateRangeOverlays(false);
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
	for (let name of this.GetAuraNames().filter(n => this[n] && msg.tag == this[n].rangeQuery))
	{
		this.ApplyBonus(name, msg.added);
		this.RemoveBonus(name, msg.removed);
	}
};

Auras.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	for (let name of this.GetAuraNames().filter(n => this.IsGarrisonedUnitsAura(n)))
	{
		this.ApplyBonus(name, msg.added);
		this.RemoveBonus(name, msg.removed);
	}
};

Auras.prototype.RegisterGlobalOwnershipChanged = function(msg)
{
	for (let name of this.GetAuraNames().filter(n => this.IsGlobalAura(n)))
	{
		let affectedPlayers = this.GetAffectedPlayers(name);
		let wasApplied = affectedPlayers.indexOf(msg.from) != -1;
		let willBeApplied = affectedPlayers.indexOf(msg.to) != -1;
		if (wasApplied && !willBeApplied)
			this.RemoveBonus(name, [msg.entity]);
		if (willBeApplied && !wasApplied)
			this.ApplyBonus(name, [msg.entity]);
	}
};

Auras.prototype.ApplyFormationBonus = function(memberList)
{
	for (let name of this.GetAuraNames().filter(n => this.IsFormationAura(n)))
		this.ApplyBonus(name, memberList);
};

Auras.prototype.ApplyGarrisonBonus = function(structure)
{
	for (let name of this.GetAuraNames().filter(n => this.IsGarrisonAura(n)))
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

	cmpAuraManager.RegisterGlobalAuraSource(this.entity);

	for (let mod of modifications)
		for (let player of players)
			cmpAuraManager.ApplyTemplateBonus(mod.value, player, classes, mod, this.GetModifierIdentifier(name));
};

Auras.prototype.RemoveFormationBonus = function(memberList)
{
	for (let name of this.GetAuraNames().filter(n => this.IsFormationAura(n)))
		this.RemoveBonus(name, memberList);
};

Auras.prototype.RemoveGarrisonBonus = function(structure)
{
	for (let name of this.GetAuraNames().filter(n => this.IsGarrisonAura(n)))
		this.RemoveBonus(name, [structure]);
};

Auras.prototype.RemoveTemplateBonus = function(name)
{
	if (!this[name].isApplied)
		return;
	if (!this.IsGlobalAura(name))
		return;

	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	cmpAuraManager.UnregisterGlobalAuraSource(this.entity);

	var modifications = this.GetModifications(name);
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
	for (let name of this.GetAuraNames())
	{
		let requiredTech = AuraTemplates.Get(name).requiredTechnology;
		if (requiredTech && requiredTech == msg.tech)
		{
			this.Clean();
			return;
		}
	}
};

Auras.prototype.OnPlayerDefeated = function(msg)
{
	this.Clean();
};

Engine.RegisterComponentType(IID_Auras, "Auras", Auras);
