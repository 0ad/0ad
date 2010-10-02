function StatusBars() {}

StatusBars.prototype.Schema =
	"<element name='HeightOffset'>" +
		"<data type='decimal'/>" +
	"</element>";

// TODO: should add rank icon too

StatusBars.prototype.Init = function()
{
	this.enabled = false;
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
	var width = 2;
	var height = 1/3;

	// Offset from the unit's position
	var offset = { "x": 0, "y": +this.template.HeightOffset, "z": 0 };

	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth)
	{
		var filled = cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints();

		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/health_bg.png",
			{ "x": -width/2, "y": -height/2 }, { "x": width/2, "y": height/2 },
			offset
		);

		cmpOverlayRenderer.AddSprite(
			"art/textures/ui/session/icons/health_fg.png",
			{ "x": -width/2, "y": -height/2 }, { "x": width*(filled - 0.5), "y": height/2 },
			offset
		);
	}
};

Engine.RegisterComponentType(IID_StatusBars, "StatusBars", StatusBars);
