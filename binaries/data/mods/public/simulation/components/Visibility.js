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
	"<element name='Preview'>" +
		"<data type='boolean'/>" +
	"</element>";

Visibility.prototype.Init = function()
{
	this.retainInFog = this.template.RetainInFog == "true";
	this.alwaysVisible = this.template.AlwaysVisible == "true";
	this.preview = this.template.Preview == "true";

	this.activated = false;

	if (this.preview)
		this.activated = true;
};

/**
 * If this function returns true, the range manager will call the GetVisibility function
 * instead of computing the visibility.
 */
Visibility.prototype.IsActivated = function()
{
	return this.activated;
};

/**
 * This function is called if IsActivated returns true.
 * If so, the return value supersedes the visibility computed by the range manager.
 * isVisible: true if the entity is in the vision range of a unit, false otherwise
 * isExplored: true if the entity is in explored territory, false otherwise
 */
Visibility.prototype.GetVisibility = function(player, isVisible, isExplored)
{
	if (!this.activated)
		warn("The Visibility component was asked to provide a superseding visibility while not activated, this should not happen");

	if (this.preview)
	{
		// For the owner only, mock the "RetainInFog" behaviour

		let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() == player && isExplored)
			return isVisible ? VIS_VISIBLE : VIS_FOGGED;

		return VIS_HIDDEN;
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
