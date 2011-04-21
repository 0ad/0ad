/////////////////////////////////////////////////////////////////////////////////////////
//	Entity
//
//	Object for holding entity data
//	TODO: support y position or offset (height) and full 3D rotation
//
/////////////////////////////////////////////////////////////////////////////////////////

function Entity(name, player, x, z, orientation)
{
	// Get unique ID
	this.id = g_Map.getEntityID();
	this.name = name;
	
	// Tile units
	this.tileX = x;
	this.tileZ = z;
	
	// Map units (4.0 map units per 1.0 tile)
	this.x = x * CELL_SIZE
	this.y = 0;
	this.z = z * CELL_SIZE;
	
	this.player = (player !== undefined ? player : 0);	
	this.orientation = (orientation !== undefined ? orientation : 0);
}
