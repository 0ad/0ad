function RangeOverlayManager() {}

RangeOverlayManager.prototype.Schema = "<empty/>";

RangeOverlayManager.prototype.Init = function()
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
RangeOverlayManager.prototype.Serialize = null;

RangeOverlayManager.prototype.Deserialize = function(data)
{
	this.Init();
};

RangeOverlayManager.prototype.UpdateRangeOverlays = function(componentName)
{
	let cmp = Engine.QueryInterface(this.entity, global["IID_" + componentName]);
	if (cmp)
		this.rangeVisualizations.set(componentName, cmp.GetRangeOverlays());
};

RangeOverlayManager.prototype.SetEnabled = function(enabled, enabledRangeTypes, forceUpdate)
{
	this.enabled = enabled;
	this.enabledRangeTypes = enabledRangeTypes;

	this.RegenerateRangeOverlayManagers(forceUpdate);
};

RangeOverlayManager.prototype.RegenerateRangeOverlayManagers = function(forceUpdate)
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

RangeOverlayManager.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == INVALID_PLAYER)
		return;
	for (let type in this.enabledRangeTypes)
		this.UpdateRangeOverlays(type);

	this.RegenerateRangeOverlayManagers(false);
};

RangeOverlayManager.prototype.OnValueModification = function(msg)
{
	if (msg.valueNames.indexOf("Heal/Range") == -1 &&
	    msg.valueNames.indexOf("Attack/Ranged/MinRange") == -1 &&
	    msg.valueNames.indexOf("Attack/Ranged/MaxRange") == -1)
		return;

	this.UpdateRangeOverlays(msg.component);
	this.RegenerateRangeOverlayManagers(false);
};

/** 
 * RangeOverlayManager component is deserialized before the TechnologyManager, so need to update the ranges here
 */
RangeOverlayManager.prototype.OnDeserialized = function(msg)
{
	for (let type in this.enabledRangeTypes)
		this.UpdateRangeOverlays(type);
};

Engine.RegisterComponentType(IID_RangeOverlayManager, "RangeOverlayManager", RangeOverlayManager);
