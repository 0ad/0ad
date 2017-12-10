function RangeVisualization() {}

RangeVisualization.prototype.Schema = "<empty/>";

RangeVisualization.prototype.Init = function()
{
	this.enabled = false;
	this.enabledRangeTypes = {
		"Attack": false,
		"Auras": false,
		"Heal": false
	};

	this.rangeVisualizations = new Map();
};

// The GUI enables visualizations
RangeVisualization.prototype.Serialize = null;

RangeVisualization.prototype.Deserialize = function(data)
{
	this.Init();
};

RangeVisualization.prototype.UpdateRangeOverlays = function(componentName)
{
	let cmp = Engine.QueryInterface(this.entity, global["IID_" + componentName]);
	if (cmp)
		this.rangeVisualizations.set(componentName, cmp.GetRangeOverlays());
};

RangeVisualization.prototype.SetEnabled = function(enabled, enabledRangeTypes, forceUpdate)
{
	this.enabled = enabled;
	this.enabledRangeTypes = enabledRangeTypes;

	this.RegenerateRangeVisualizations(forceUpdate);
};

RangeVisualization.prototype.RegenerateRangeVisualizations = function(forceUpdate)
{
	let cmpRangeOverlayRenderer = Engine.QueryInterface(this.entity, IID_RangeOverlayRenderer);
	if (!cmpRangeOverlayRenderer)
		return;

	cmpRangeOverlayRenderer.ResetRangeOverlays();

	if (!this.enabled && !forceUpdate)
		return;

	// Only render individual range types that have been enabled
	for (let rangeOverlayType of this.rangeVisualizations.keys())
		if (this.enabledRangeTypes[rangeOverlayType])
			for (let rangeOverlay of this.rangeVisualizations.get(rangeOverlayType))
				cmpRangeOverlayRenderer.AddRangeOverlay(
					rangeOverlay.radius,
					rangeOverlay.texture,
					rangeOverlay.textureMask,
					rangeOverlay.thickness);
};

RangeVisualization.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == -1)
		return;
	for (let type in this.enabledRangeTypes)
		this.UpdateRangeOverlays(type);

	this.RegenerateRangeVisualizations(false);
};

RangeVisualization.prototype.OnValueModification = function(msg)
{
	if (msg.valueNames.indexOf("Heal/Range") == -1 &&
	    msg.valueNames.indexOf("Attack/Ranged/MinRange") == -1 &&
	    msg.valueNames.indexOf("Attack/Ranged/MaxRange") == -1)
		return;

	this.UpdateRangeOverlays(msg.component);
	this.RegenerateRangeVisualizations(false);
};

/** 
 * RangeVisualization component is deserialized before the TechnologyManager, so need to update the ranges here
 */
RangeVisualization.prototype.OnDeserialized = function(msg)
{
	for (let type in this.enabledRangeTypes)
		this.UpdateRangeOverlays(type);
};

Engine.RegisterComponentType(IID_RangeVisualization, "RangeVisualization", RangeVisualization);
