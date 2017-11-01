/**
 * The Entity class stores the given template name, owner and location of an entity and assigns an entityID.
 * Instances of this class (with the position using the tile coordinate system) are passed as such to the engine.
 *
 * @param orientation - rotation of this entity about the y-axis (up).
 */
// TODO: support full position and rotation
function Entity(templateName, playerID, x, z, orientation = 0)
{
	this.id = g_Map.getEntityID();
	this.templateName = templateName;
	this.player = playerID;

	this.position = {
		"x": x,
		"y": 0,
		"z": z
	};

	this.rotation = {
		"x": 0,
		"y": orientation,
		"z": 0
	};
}
