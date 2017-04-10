/**
 * Whether to also place all actors.
 */
let actors = false;

/**
 * Coordinates of the first entity.
 */
let startX = 20;
let startZ = 20;

/**
 * Horizontal coordinate of the last entity in the current row.
 */
let stopX = 1580;

/**
 * Coordinates of the current entity.
 */
let x = startX;
let z = startZ;

/**
 * Recall the greatest length in the current row to prevent overlapping.
 */
let maxh = 0;

/**
 * Space between entities.
 */
let gap = 14;

let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
for (let template of cmpTemplateManager.FindAllPlaceableTemplates(actors))
{
	print(template + "...\n");

	let ent = Engine.AddEntity(template);
	if (!ent)
	{
		error("Failed to load " + template + "\n");
		continue;
	}

	let cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
	if (!cmpFootprint)
	{
		print(template + " has no footprint\n");
		continue;
	}

	let shape = cmpFootprint.GetShape();
	let w = shape.width;
	let h = shape.depth;

	if (shape.type == 'circle')
		w = h = shape.radius * 2;

	if (x + w >= stopX)
	{
		// Start a new row
		x = startX;
		z += maxh + gap;
		maxh = 0;
	}

	let cmpPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpPosition)
	{
		warn(template + " has no position\n");
		Engine.DestroyEntity(ent);
		continue;
	}

	cmpPosition.MoveTo(x + w / 2, z);
	cmpPosition.SetYRotation(Math.PI * 3 / 4);

	let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpOwnership)
		cmpOwnership.SetOwner(1);

	x += w + gap;
	maxh = Math.max(maxh, h);
}
