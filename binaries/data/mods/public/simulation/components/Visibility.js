const VIS_HIDDEN = 0;
const VIS_FOGGED = 1;
const VIS_VISIBLE = 2;

function Visibility() {}

Visibility.prototype.Schema =
	"<empty/>";

Visibility.prototype.Init = function()
{
	
};

/**
 * This function is called for entities in explored territory.
 * isOutsideFog: true if we're in the vision range of a unit, false otherwise
 * forceRetainInFog: useful for previewed entities, see the RangeManager system component documentation
 */
Visibility.prototype.GetLosVisibility = function(player, isOutsideFog, forceRetainInFog)
{
	if (isOutsideFog)
		return VIS_VISIBLE;

	// Fogged if the 'retain in fog' flag is set, and in a non-visible explored region
	var cmpVision = Engine.QueryInterface(this.entity, IID_Vision);
	if (!forceRetainInFog && !(cmpVision && cmpVision.GetRetainInFog()))
		return VIS_HIDDEN;

	var cmpMirage = Engine.QueryInterface(this.entity, IID_Mirage);
	if (cmpMirage && cmpMirage.GetPlayer() == player)
		return VIS_FOGGED;

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return VIS_FOGGED;

	if (cmpOwnership.GetOwner() == player)
	{
		var cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
		if (!cmpFogging)
			return VIS_FOGGED;

		// Fogged entities must not disappear while the mirage is not ready
		if (!cmpFogging.IsMiraged(player))
			return VIS_FOGGED;

		return VIS_HIDDEN;
	}

	// Fogged entities must not disappear while the mirage is not ready
	var cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging && cmpFogging.WasSeen(player) && !cmpFogging.IsMiraged(player))
		return VIS_FOGGED;

	return VIS_HIDDEN;
};

Engine.RegisterComponentType(IID_Visibility, "Visibility", Visibility);
