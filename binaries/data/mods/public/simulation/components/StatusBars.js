const NATURAL_COLOR = "255 255 255 255"; // pure white

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

/**
 * For every sprite, the code will call their "Add" method when regenerating
 * the sprites. Every sprite adder should return the height it needs.
 *
 * Modders who need extra sprites can just modify this array, and
 * provide the right methods.
 */
StatusBars.prototype.Sprites =
	[
		"PackBar",
		"ResourceSupplyBar",
		"CaptureBar",
		"HealthBar",
		"AuraIcons",
	];

StatusBars.prototype.Init = function()
{
	this.enabled = false;
	this.auraSources = new Map();
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
	if (this.auraSources.has(source))
		this.auraSources.get(source).push(auraName);
	else
		this.auraSources.set(source, [auraName]);
	this.RegenerateSprites();
};

StatusBars.prototype.RemoveAuraSource = function(source, auraName)
{
	let names = this.auraSources.get(source);
	names.splice(names.indexOf(auraName), 1);
	this.RegenerateSprites();
};

StatusBars.prototype.OnHealthChanged = function(msg)
{
	if (this.enabled)
		this.RegenerateSprites();
};

StatusBars.prototype.OnCapturePointsChanged = function(msg)
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
	for (let sprite of this.Sprites)
		yoffset += this["Add"+sprite](cmpOverlayRenderer, yoffset);
};

// Internal helper functions
/**
 * Generic piece of code to add a bar
 */
StatusBars.prototype.AddBar = function(cmpOverlayRenderer, yoffset, type, amount)
{
	// Size of health bar (in world-space units)
	let width = +this.template.BarWidth;
	let height = +this.template.BarHeight;

	// World-space offset from the unit's position
	let offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	// background
	cmpOverlayRenderer.AddSprite(
		"art/textures/ui/session/icons/"+type+"_bg.png",
		{ "x": -width/2, "y": yoffset },
		{ "x": width/2, "y": height + yoffset},
		offset,
		NATURAL_COLOR
	);

	// foreground
	cmpOverlayRenderer.AddSprite(
		"art/textures/ui/session/icons/"+type+"_fg.png",
		{ "x": -width/2, "y": yoffset},
		{ "x": width*(amount - 0.5), "y": height + yoffset},
		offset,
		NATURAL_COLOR
	);

	return height * 1.2;
};

StatusBars.prototype.AddPackBar = function(cmpOverlayRenderer, yoffset)
{
	if (!this.enabled)
		return 0;

	let cmpPack = Engine.QueryInterface(this.entity, IID_Pack);
	if (!cmpPack || !cmpPack.IsPacking())
		return 0;

	return this.AddBar(cmpOverlayRenderer, yoffset, "pack", cmpPack.GetProgress());
};

StatusBars.prototype.AddHealthBar = function(cmpOverlayRenderer, yoffset)
{
	if (!this.enabled)
		return 0;

	let cmpHealth = QueryMiragedInterface(this.entity, IID_Health);
	if (!cmpHealth || !cmpHealth.GetHitpoints() > 0)
		return 0;

	return this.AddBar(cmpOverlayRenderer, yoffset, "health", cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints());
};

StatusBars.prototype.AddResourceSupplyBar = function(cmpOverlayRenderer, yoffset)
{
	if (!this.enabled)
		return 0;

	let cmpResourceSupply = QueryMiragedInterface(this.entity, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return 0;
	let value = cmpResourceSupply.IsInfinite() ? 1 : cmpResourceSupply.GetCurrentAmount() / cmpResourceSupply.GetMaxAmount();
	return this.AddBar(cmpOverlayRenderer, yoffset, "supply", value);
};

StatusBars.prototype.AddCaptureBar = function(cmpOverlayRenderer, yoffset)
{
	if (!this.enabled)
		return 0;

	let cmpCapturable = QueryMiragedInterface(this.entity, IID_Capturable);
	if (!cmpCapturable)
		return 0;

	let cmpOwnership = QueryMiragedInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return 0;

	let owner = cmpOwnership.GetOwner();
	if (owner == -1)
		return 0;
	let cp = cmpCapturable.GetCapturePoints();

	// Size of health bar (in world-space units)
	let width = +this.template.BarWidth;
	let height = +this.template.BarHeight;

	// World-space offset from the unit's position
	let offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	let setCaptureBarPart = function(playerID, startSize)
	{
		let c = QueryPlayerIDInterface(playerID).GetColor();
		let strColor = (c.r * 255) + " " + (c.g * 255) + " " + (c.b * 255) + " 255";
		let size = width * cp[playerID] / cmpCapturable.GetMaxCapturePoints();

		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/capture_bar.png",
			{ "x": startSize, "y": yoffset },
			{ "x": startSize + size, "y": height + yoffset},
			offset,
			strColor
		);

		return size + startSize;
	};

	// first handle the owner's points, to keep those points on the left for clarity
	let size = setCaptureBarPart(owner, -width / 2);
	for (let i in cp)
		if (i != owner && cp[i] > 0)
			size = setCaptureBarPart(i, size);

	return height * 1.2;
};

StatusBars.prototype.AddAuraIcons = function(cmpOverlayRenderer, yoffset)
{
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	let sources = cmpGuiInterface.GetEntitiesWithStatusBars().filter(e => this.auraSources.has(e) && this.auraSources.get(e).length);

	if (!sources.length)
		return 0;

	let iconSet = new Set();
	for (let ent of sources)
	{
		let cmpAuras = Engine.QueryInterface(ent, IID_Auras);
		if (!cmpAuras) // probably the ent just died
			continue;
		for (let name of this.auraSources.get(ent))
			iconSet.add(cmpAuras.GetOverlayIcon(name));
	}

	// World-space offset from the unit's position
	let offset = { "x": 0, "y": +this.template.HeightOffset + yoffset, "z": 0 };

	let iconSize = +this.template.BarWidth / 2;
	let xoffset = -iconSize * (iconSet.size - 1) * 0.6;
	for (let icon of iconSet)
	{
		cmpOverlayRenderer.AddSprite(
			icon,
			{ "x": xoffset - iconSize/2, "y": yoffset },
			{ "x": xoffset + iconSize/2, "y": iconSize + yoffset },
			offset,
			NATURAL_COLOR
		);
		xoffset += iconSize * 1.2;
	}

	return iconSize + this.template.BarHeight / 2;
};



Engine.RegisterComponentType(IID_StatusBars, "StatusBars", StatusBars);
