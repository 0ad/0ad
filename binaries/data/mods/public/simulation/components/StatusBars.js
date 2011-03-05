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
};

// Because this is enabled directly by the GUI and is not
// network-synchronised (it only affects local rendering),
// we disable serialization in order to prevent OOS errors
StatusBars.prototype.Serialize = null;

StatusBars.prototype.Deserialize = function()
{
	// Use default initialisation
	this.Init();
};

StatusBars.prototype.SetEnabled = function(enabled)
{
	// Quick return if no change
	if (enabled == this.enabled)
		return;

	// Update the displayed sprites

	this.enabled = enabled;

	if (enabled)
		this.RegenerateSprites();
	else
		this.ResetSprites();
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

StatusBars.prototype.ResetSprites = function()
{
	var cmpOverlayRenderer = Engine.QueryInterface(this.entity, IID_OverlayRenderer);
	cmpOverlayRenderer.Reset();
};

StatusBars.prototype.RegenerateSprites = function()
{
	var cmpOverlayRenderer = Engine.QueryInterface(this.entity, IID_OverlayRenderer);
	cmpOverlayRenderer.Reset();

	// Size of health bar (in world-space units)
	var width = +this.template.BarWidth;
	var height = +this.template.BarHeight;

	// World-space offset from the unit's position
	var offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	// Billboard offset of next bar
	var yoffset = 0;

	var AddBar = function(type, amount)
	{
		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/"+type+"_bg.png",
			{ "x": -width/2, "y": -height/2 + yoffset },
			{ "x": width/2, "y": height/2 + yoffset },
			offset
		);

		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/"+type+"_fg.png",
			{ "x": -width/2, "y": -height/2 + yoffset },
			{ "x": width*(amount - 0.5), "y": height/2 + yoffset },
			offset
		);

		yoffset -= height * 1.2;
	};

	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth && cmpHealth.GetHitpoints() > 0)
	{
		AddBar("health", cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints());
	}

	var cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
	{
		AddBar("supply", cmpResourceSupply.GetCurrentAmount() / cmpResourceSupply.GetMaxAmount());
	}

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
