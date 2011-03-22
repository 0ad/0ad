function Entity(name, player, x, y, angle)
{
	// Get unique ID
	this.id = g_Map.getEntityID();
	this.name = name;
	
	// Convert from tile coords to map coords
	this.x = x;
	this.y = y;
	
	if (player !== undefined)
	{
		this.player = player;
		this.isActor = false;
	}
	else
	{	// Actors  have no player ID
		this.isActor = true;
	}
	
	this.orientation = (angle !== undefined ? angle : 0);
}
