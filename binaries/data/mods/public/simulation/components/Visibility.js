const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Visibility() {}

Visibility.prototype.Schema =
	"<element name='RetainInFog'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='AlwaysVisible'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='Corpse'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='Preview'>" +
		"<data type='boolean'/>" +
	"</element>";

Visibility.prototype.Init = function()
{
	this.retainInFog = this.template.RetainInFog == "true";
	this.alwaysVisible = this.template.AlwaysVisible == "true";
	this.corpse = this.template.Corpse == "true";
	this.preview = this.template.Preview == "true";

	this.activated = false;

	if (this.preview || this.corpse)
		this.SetActivated(true);
};

/**
 * Sets the range manager scriptedVisibility flag for this entity.
 */
Visibility.prototype.SetActivated = function(status)
{
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.ActivateScriptedVisibility(this.entity, status);

	this.activated = status;
};

/**
 * This function is a fallback for some entities whose visibility status
 * cannot be cached by the range manager (especially local entities like previews).
 * Calling the scripts is expensive, so only call it if really needed.
 */
Visibility.prototype.IsActivated = function()
{
	return this.activated;
};

/**
 * This function is called if the range manager scriptedVisibility flag is set to true for this entity.
 * If so, the return value supersedes the visibility computed by the range manager.
 * isVisible: true if the entity is in the vision range of a unit, false otherwise
 * isExplored: true if the entity is in explored territory, false otherwise
 */
Visibility.prototype.GetVisibility = function(player, isVisible, isExplored)
{
	if (this.preview)
	{
		// For the owner only, mock the "RetainInFog" behavior
		let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() == player && isExplored)
			return isVisible ? VIS_VISIBLE : VIS_FOGGED;

		// For others, hide the preview
		return VIS_HIDDEN;
	}
	else if (this.corpse)
	{
		// For the owner only, mock the "RetainInFog" behavior
		let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() == player && isExplored)
			return isVisible ? VIS_VISIBLE : VIS_FOGGED;

		// For others, regular displaying
		return isVisible ? VIS_VISIBLE : VIS_HIDDEN;
	}

	return VIS_VISIBLE;
};

Visibility.prototype.GetRetainInFog = function()
{
	return this.retainInFog;
};

Visibility.prototype.GetAlwaysVisible = function()
{
	return this.alwaysVisible;
};

Engine.RegisterComponentType(IID_Visibility, "Visibility", Visibility);
