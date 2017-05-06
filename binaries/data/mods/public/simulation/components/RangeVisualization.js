function RangeVisualization() {}

RangeVisualization.prototype.Schema = "<empty/>";

RangeVisualization.prototype.Init = function()
{
	this.enabled = false;
	this.enabledRangeTypes = {
		"Aura": false
	};

	this.rangeVisualizations = new Map();
	for (let type in this.enabledRangeTypes)
		this["GetVisual" + type + "Ranges"](type);
};

// The GUI enables visualizations
RangeVisualization.prototype.Serialize = null;

RangeVisualization.prototype.Deserialize = function(data)
{
	this.Init();
};

RangeVisualization.prototype.GetVisualAuraRanges = function(type)
{
	let cmpAuras = Engine.QueryInterface(this.entity, IID_Auras);
	if (!cmpAuras)
		return;

	this.rangeVisualizations.set(type, []);

	for (let auraName of cmpAuras.GetVisualAuraRangeNames())
		this.rangeVisualizations.get(type).push({
			"radius": cmpAuras.GetRange(auraName),
			"texture": cmpAuras.GetLineTexture(auraName),
			"textureMask": cmpAuras.GetLineTextureMask(auraName),
			"thickness": cmpAuras.GetLineThickness(auraName),
		});
};

RangeVisualization.prototype.SetEnabled = function(enabled, enabledRangeTypes)
{
	this.enabled = enabled;
	this.enabledRangeTypes = enabledRangeTypes;

	this.RegenerateRangeVisualizations();
};

RangeVisualization.prototype.RegenerateRangeVisualizations = function()
{
	let cmpSelectable = Engine.QueryInterface(this.entity, IID_Selectable);
	if (!cmpSelectable)
		return;

	cmpSelectable.ResetRangeOverlays();

	if (!this.enabled)
		return;

	// Only render individual range types that have been enabled
	for (let rangeOverlayType of this.rangeVisualizations.keys())
		if (this.enabledRangeTypes[rangeOverlayType])
			for (let rangeOverlay of this.rangeVisualizations.get(rangeOverlayType))
				cmpSelectable.AddRangeOverlay(
					rangeOverlay.radius,
					rangeOverlay.texture,
					rangeOverlay.textureMask,
					rangeOverlay.thickness);
};

RangeVisualization.prototype.OnOwnershipChanged = function(msg)
{
	if (this.enabled && msg.to != -1)
		this.RegenerateRangeVisualizations();
};

Engine.RegisterComponentType(IID_RangeVisualization, "RangeVisualization", RangeVisualization);
