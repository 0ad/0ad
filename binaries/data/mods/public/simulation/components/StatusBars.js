function StatusBars() {}

StatusBars.prototype.Schema =
	"<element name='BarWidth'>" +
		"<data type='decimal'/>" +
	"</element>" +
	"<element name='BarHeight'>" +
		"<data type='decimal'/>" +
	"</element>" +
	"<element name='HeightOffset'>" +
		"<data type='decimal'/>" +
	"</element>";

StatusBars.prototype.Init = function()
{
	this.enabled = false;
	this.auraSources = {};
};

/**
 * Don't serialise this.enabled since it's modified by the GUI
 */
StatusBars.prototype.Serialize = function()
{
	return {"auraSources": this.auraSources};
};

StatusBars.prototype.Deserialize = function(data)
{
	this.Init();
	this.auraSources = data.auraSources;
};

StatusBars.prototype.SetEnabled = function(enabled)
{
	// Quick return if no change
	if (enabled == this.enabled)
		return;

	this.enabled = enabled;

	// Update the displayed sprites
	this.RegenerateSprites();
};

StatusBars.prototype.AddAuraSource = function(source, auraName)
{
	if (this.auraSources[source])
		this.auraSources[source].push(auraName);
	else
		this.auraSources[source] = [auraName];
	this.RegenerateSprites();
};

StatusBars.prototype.RemoveAuraSource = function(source, auraName)
{
	let names = this.auraSources[source];
	names.splice(names.indexOf(auraName), 1);
	this.RegenerateSprites();
};

StatusBars.prototype.OnHealthChanged = function(msg)
{
	if (this.enabled)
		this.RegenerateSprites();
};

StatusBars.prototype.OnResourceSupplyChanged = function(msg)
{
	if (this.enabled)
		this.RegenerateSprites();
};

StatusBars.prototype.OnPackProgressUpdate = function(msg)
{
	if (this.enabled)
		this.RegenerateSprites();
};

StatusBars.prototype.RegenerateSprites = function()
{
	let cmpOverlayRenderer = Engine.QueryInterface(this.entity, IID_OverlayRenderer);
	cmpOverlayRenderer.Reset();

	let yoffset = 0;
	if (this.enabled)
		yoffset = this.AddBars(cmpOverlayRenderer, yoffset);
	yoffset = this.AddAuraIcons(cmpOverlayRenderer, yoffset);
	return yoffset;
};

// Internal helper functions
StatusBars.prototype.AddAuraIcons = function(cmpOverlayRenderer, yoffset)
{
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	let sources = cmpGuiInterface.GetEntitiesWithStatusBars().filter(e => this.auraSources[e] && this.auraSources[e].length);

	if (!sources.length)
		return yoffset;

	let iconSet = new Set();
	for (let ent of sources)
	{
		let cmpAuras = Engine.QueryInterface(ent, IID_Auras); 
		if (!cmpAuras) // probably the ent just died 
			continue; 
		for (let name of this.auraSources[ent]) 
			iconSet.add(cmpAuras.GetOverlayIcon(name)); 
	}

	// World-space offset from the unit's position
	var offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	let iconSize = +this.template.BarWidth / 2; 
	let xoffset = -iconSize * (iconSet.size - 1) * 0.6
	for (let icon of iconSet) 
	{ 
		cmpOverlayRenderer.AddSprite( 
			icon, 
			{ "x": xoffset + iconSize/2, "y": yoffset + iconSize }, 
			{ "x": xoffset - iconSize/2, "y": yoffset }, 
			offset
		); 
		xoffset += iconSize * 1.2;
	} 

	return yoffset + iconSize + this.template.BarHeight / 2;
};

StatusBars.prototype.AddBars = function(cmpOverlayRenderer, yoffset)
{
	// Size of health bar (in world-space units)
	var width = +this.template.BarWidth;
	var height = +this.template.BarHeight;

	// World-space offset from the unit's position
	var offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	var AddBar = function(type, amount)
	{
		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/"+type+"_bg.png",
			{ "x": -width/2, "y":yoffset },
			{ "x": width/2, "y": height + yoffset },
			offset
		);

		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/"+type+"_fg.png",
			{ "x": -width/2, "y": yoffset },
			{ "x": width*(amount - 0.5), "y": height + yoffset },
			offset
		);

		yoffset += height * 1.2;
	};

	var cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (cmpPack && cmpPack.IsPacking())
		AddBar("pack", cmpPack.GetProgress());

	var cmpHealth = QueryMiragedInterface(this.entity, IID_Health);
	if (cmpHealth && cmpHealth.GetHitpoints() > 0)
		AddBar("health", cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints());

	var cmpResourceSupply = QueryMiragedInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
		AddBar("supply", cmpResourceSupply.IsInfinite() ? 1 : cmpResourceSupply.GetCurrentAmount() / cmpResourceSupply.GetMaxAmount());

	return yoffset;
	/*
	// Rank icon disabled for now - see discussion around
	// http://www.wildfiregames.com/forum/index.php?s=&showtopic=13608&view=findpost&p=212154

	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
	{
		var rank = cmpIdentity.GetRank();
		if (rank == "Advanced" || rank == "Elite")
		{
			var icon;
			if (rank == "Advanced")
				icon = "art/textures/ui/session/icons/advanced.dds";
			else
				icon = "art/textures/ui/session/icons/elite.dds";

			var rankSize = 0.666;
			var xoffset = -width/2 - rankSize/2;
			cmpOverlayRenderer.AddSprite(
				icon,
				{ "x": -rankSize/2 + xoffset, "y": -rankSize/2 + yoffset },
				{ "x": rankSize/2 + xoffset, "y": rankSize/2 + yoffset },
				offset
			);
		}
	}
	*/
};

Engine.RegisterComponentType(IID_StatusBars, "StatusBars", StatusBars);
