/////////////////////////////////////////////////////////////////////////////////////////
//	Entity
//
//	Object for holding entity data
//
//	templateName: string containing name of the template for this entity,
//		optionally prefixed with "actor|".
//	player: id of player who owners this entity.
//	x,z: position of this entity in tiles.
//	orientation: rotation of this entity about the y-axis (up).
//
/////////////////////////////////////////////////////////////////////////////////////////

// TODO: support full position and rotation
function Entity(templateName, player, x, z, orientation)
{
	// Get unique ID
	this.id = g_Map.getEntityID();
	this.templateName = templateName;
	this.player = (player !== undefined ? player : 0);

	// Map units (4.0 map units per 1.0 tile)
	this.position = {
		x: x * CELL_SIZE,
		y: 0,
		z: z * CELL_SIZE
	};
	this.rotation = {
		x: 0,
		y: (orientation !== undefined ? orientation : 0),
		z: 0
	};
}
