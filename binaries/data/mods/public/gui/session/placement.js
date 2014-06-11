function PlacementSupport()
{
	this.Reset();
}

PlacementSupport.DEFAULT_ANGLE = Math.PI*3/4;

/**
 * Resets the building placement support state. Use this to cancel construction of an entity.
 */
PlacementSupport.prototype.Reset = function()
{
	this.mode = null;
	this.position = null;
	this.template = null;
	this.tooltipMessage = "";        // tooltip text to show while the user is placing a building
	this.tooltipError = false;
	this.wallSet = null;             // maps types of wall pieces ("tower", "long", "short", ...) to template names
	this.wallSnapEntities = null;    // list of candidate entities to snap the starting and (!) ending positions to when building walls
	this.wallEndPosition = null;
	this.wallSnapEntitiesIncludeOffscreen = false; // should the next update of the snap candidate list include offscreen towers?
	
	this.SetDefaultAngle();
	this.RandomizeActorSeed();

	this.attack = null;
	
	Engine.GuiInterfaceCall("SetBuildingPlacementPreview", {"template": ""});
	Engine.GuiInterfaceCall("SetWallPlacementPreview", {"wallSet": null});
};

PlacementSupport.prototype.SetDefaultAngle = function()
{
	this.angle = PlacementSupport.DEFAULT_ANGLE;
};

PlacementSupport.prototype.RandomizeActorSeed = function()
{
	this.actorSeed = Math.floor(65535 * Math.random());
};

var placementSupport = new PlacementSupport();
