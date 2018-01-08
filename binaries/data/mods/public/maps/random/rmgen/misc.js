function placeDefaultChicken(playerX, playerZ, tileClass, constraint = undefined, template = "gaia/fauna_chicken")
{
	for (let j = 0; j < 2; ++j)
		for (var tries = 0; tries < 10; ++tries)
		{
			let aAngle = randFloat(0, 2 * Math.PI);

			// Roman and ptolemian civic centers have a big footprint!
			let aDist = 9;

			let aX = Math.round(playerX + aDist * cos(aAngle));
			let aZ = Math.round(playerZ + aDist * sin(aAngle));

			let group = new SimpleGroup(
				[new SimpleObject(template, 5,5, 0,2)],
				true, tileClass, aX, aZ
			);

			if (createObjectGroup(group, 0, constraint))
				break;
		}
}

/**
 * Typically used for placing grass tufts around the civic centers.
 */
function placeDefaultDecoratives(playerX, playerZ, template, tileclass, radius, constraint = undefined)
{
	for (let i = 0; i < PI * radius * radius / 250; ++i)
	{
		let angle = randFloat(0, 2 * PI);
		let dist = radius - randIntInclusive(5, 11);

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(template, 2, 5, 0, 1, -PI/8, PI/8)],
				false,
				tileclass,
				Math.round(playerX + dist * Math.cos(angle)),
				Math.round(playerZ + dist * Math.sin(angle))
			), 0, constraint);
	}
}
