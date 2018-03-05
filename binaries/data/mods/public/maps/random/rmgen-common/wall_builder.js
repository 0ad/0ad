/**
 * @file Contains functionality to place walls on random maps.
 */

/**
 * Set some globals for this module.
 */
var g_WallStyles = loadWallsetsFromCivData();
var g_FortressTypes = createDefaultFortressTypes();

/**
 * Fetches wallsets from {civ}.json files, and then uses them to load
 * basic wall elements.
 */
function loadWallsetsFromCivData()
{
	let wallsets = {};
	for (let civ in g_CivData)
	{
		let civInfo = g_CivData[civ];
		if (!civInfo.WallSets)
			continue;

		for (let path of civInfo.WallSets)
		{
			// File naming conventions:
			// - other/wallset_{style}
			// - structures/{civ}_wallset_{style}
			let style = basename(path).split("_");
			style = style[0] == "wallset" ? style[1] : style[0] + "_" + style[2];

			if (!wallsets[style])
				wallsets[style] = loadWallset(Engine.GetTemplate(path), civ);
		}
	}
	return wallsets;
}

function loadWallset(wallsetPath, civ)
{
	let newWallset = { "curves": [] };
	let wallsetData = GetTemplateDataHelper(wallsetPath).wallSet;

	for (let element in wallsetData.templates)
		if (element == "curves")
			for (let filename of wallsetData.templates.curves)
				newWallset.curves.push(readyWallElement(filename, civ));
		else
			newWallset[element] = readyWallElement(wallsetData.templates[element], civ);

	newWallset.overlap = wallsetData.minTowerOverlap * newWallset.tower.length;

	return newWallset;
}

/**
 * Fortress class definition
 *
 * We use "fortress" to describe a closed wall built of multiple wall
 * elements attached together surrounding a central point. We store the
 * abstract of the wall (gate, tower, wall, ...) and only apply the style
 * when we get to build it.
 *
 * @param {string} type - Descriptive string, example: "tiny". Not really needed (WallTool.wallTypes["type string"] is used). Mainly for custom wall elements.
 * @param {array} [wall] - Array of wall element strings. May be defined at a later point.
 *                         Example: ["medium", "cornerIn", "gate", "cornerIn", "medium", "cornerIn", "gate", "cornerIn"]
 * @param {object} [centerToFirstElement] - Vector from the visual center of the fortress to the first wall element.
 * @param {number} [centerToFirstElement.x]
 * @param {number} [centerToFirstElement.y]
 */
function Fortress(type, wall=[], centerToFirstElement=undefined)
{
	this.type = type;
	this.wall = wall;
	this.centerToFirstElement = centerToFirstElement;
}

function createDefaultFortressTypes()
{
	let defaultFortresses = {};

	/**
	 * Define some basic default fortress types.
	 */
	let addFortress = (type, walls) => defaultFortresses[type] = { "wall": walls.concat(walls, walls, walls) };
	addFortress("tiny", ["gate", "tower", "short", "cornerIn", "short", "tower"]);
	addFortress("small", ["gate", "tower", "medium", "cornerIn", "medium", "tower"]);
	addFortress("medium", ["gate", "tower", "long", "cornerIn", "long", "tower"]);
	addFortress("normal", ["gate", "tower", "medium", "cornerIn", "medium", "cornerOut", "medium", "cornerIn", "medium", "tower"]);
	addFortress("large", ["gate", "tower", "long", "cornerIn", "long", "cornerOut", "long", "cornerIn", "long", "tower"]);
	addFortress("veryLarge", ["gate", "tower", "medium", "cornerIn", "medium", "cornerOut", "long", "cornerIn", "long", "cornerOut", "medium", "cornerIn", "medium", "tower"]);
	addFortress("giant", ["gate", "tower", "long", "cornerIn", "long", "cornerOut", "long", "cornerIn", "long", "cornerOut", "long", "cornerIn", "long", "tower"]);

	/**
	 * Define some fortresses based on those above, but designed for use
	 * with the "palisades" wallset.
	 */
	for (let fortressType in defaultFortresses)
	{
		const fillTowersBetween = ["short", "medium", "long", "start", "end", "cornerIn", "cornerOut"];
		const newKey = fortressType + "Palisades";
		const oldWall = defaultFortresses[fortressType].wall;

		defaultFortresses[newKey] = { "wall": [] };
		for (let j = 0; j < oldWall.length; ++j)
		{
			defaultFortresses[newKey].wall.push(oldWall[j]);

			if (j + 1 < oldWall.length &&
				fillTowersBetween.indexOf(oldWall[j]) != -1 &&
				fillTowersBetween.indexOf(oldWall[j + 1]) != -1)
			{
				defaultFortresses[newKey].wall.push("tower");
			}
		}
	}

	return defaultFortresses;
}

/**
 * Define some helper functions
 */

/**
 * Get a wall element of a style.
 *
 * Valid elements:
 *   long, medium, short, start, end, cornerIn, cornerOut, tower, fort, gate, entry, entryTower, entryFort
 *
 * Dynamic elements:
 *   `gap_{x}` returns a non-blocking gap of length `x` meters.
 *   `turn_{x}` returns a zero-length turn of angle `x` radians.
 *
 * Any other arbitrary string passed will be attempted to be used as: `structures/{civ}_{arbitrary_string}`.
 *
 * @param {string} element - What sort of element to fetch.
 * @param {string} [style] - The style from which this element should come from.
 * @returns {object} The wall element requested. Or a tower element.
 */
function getWallElement(element, style)
{
	style = validateStyle(style);
	if (g_WallStyles[style][element])
		return g_WallStyles[style][element];

	// Attempt to derive any unknown elements.
	// Defaults to a wall tower piece
	const quarterBend = Math.PI / 2;
	let wallset = g_WallStyles[style];
	let civ = style.split("_")[0];
	let ret = wallset.tower ? clone(wallset.tower) : { "angle": 0, "bend": 0, "length": 0, "indent": 0 };

	switch (element)
	{
	case "cornerIn":
		if (wallset.curves)
			for (let curve of wallset.curves)
				if (curve.bend == quarterBend)
					ret = curve;

		if (ret.bend != quarterBend)
		{
			ret.angle += Math.PI / 4;
			ret.indent = ret.length / 4;
			ret.length = 0;
			ret.bend = Math.PI / 2;
		}
		break;

	case "cornerOut":
		if (wallset.curves)
			for (let curve of wallset.curves)
				if (curve.bend == quarterBend)
				{
					ret = clone(curve);
					ret.angle += Math.PI / 2;
					ret.indent -= ret.indent * 2;
				}

		if (ret.bend != quarterBend)
		{
			ret.angle -= Math.PI / 4;
			ret.indent = -ret.length / 4;
			ret.length = 0;
		}
		ret.bend = -Math.PI / 2;
		break;

	case "entry":
		ret.templateName = undefined;
		ret.length = wallset.gate.length;
		break;

	case "entryTower":
		ret.templateName = g_CivData[civ] ? "structures/" + civ + "_defense_tower" : "other/palisades_rocks_watchtower";
		ret.indent = ret.length * -3;
		ret.length = wallset.gate.length;
		break;

	case "entryFort":
		ret = clone(wallset.fort);
		ret.angle -= Math.PI;
		ret.length *= 1.5;
		ret.indent = ret.length;
		break;

	case "start":
		if (wallset.end)
		{
			ret = clone(wallset.end);
			ret.angle += Math.PI;
		}
		break;

	case "end":
		if (wallset.end)
			ret = wallset.end;
		break;

	default:
		if (element.startsWith("gap_"))
		{
			ret.templateName = undefined;
			ret.angle = 0;
			ret.length = +element.slice("gap_".length);
		}
		else if (element.startsWith("turn_"))
		{
			ret.templateName = undefined;
			ret.bend = +element.slice("turn_".length) * Math.PI;
			ret.length = 0;
		}
		else
		{
			if (!g_CivData[civ])
				civ = Object.keys(g_CivData)[0];

			let templateName = "structures/" + civ + "_" + element;
			if (Engine.TemplateExists(templateName))
			{
				ret.indent = ret.length * (element == "outpost" || element.endsWith("_tower") ? -3 : 3.5);
				ret.templateName = templateName;
				ret.length = 0;
			}
			else
				warn("Unrecognised wall element: '" + element + "' (" + style + "). Defaulting to " + (wallset.tower ? "'tower'." : "a blank element."));
		}
	}

	// Cache to save having to calculate this element again.
	g_WallStyles[style][element] = deepfreeze(ret);

	return ret;
}

/**
 * Prepare a wall element for inclusion in a style.
 *
 * @param {string} path - The template path to read values from
 */
function readyWallElement(path, civCode)
{
	path = path.replace(/\{civ\}/g, civCode);
	let template = GetTemplateDataHelper(Engine.GetTemplate(path), null, null, {}, g_DamageTypes, {});
	let length = template.wallPiece ? template.wallPiece.length : template.obstruction.shape.width;

	return deepfreeze({
		"templateName": path,
		"angle": template.wallPiece ? template.wallPiece.angle : Math.PI,
		"length": length / TERRAIN_TILE_SIZE,
		"indent": template.wallPiece ? template.wallPiece.indent / TERRAIN_TILE_SIZE : 0,
		"bend": template.wallPiece ? template.wallPiece.bend : 0
	});
}

/**
 * Returns a list of objects containing all information to place all the wall elements entities with placeObject (but the player ID)
 * Placing the first wall element at startX/startY placed with an angle given by orientation
 * An alignment can be used to get the "center" of a "wall" (more likely used for fortresses) with getCenterToFirstElement
 *
 * @param {Vector2D} position
 * @param {array} [wall]
 * @param {string} [style]
 * @param {number} [orientation]
 * @returns {array}
 */
function getWallAlignment(position, wall = [], style = "athen_stone", orientation = 0)
{
	style = validateStyle(style);
	let alignment = [];
	let wallPosition = position.clone();

	for (let i = 0; i < wall.length; ++i)
	{
		let element = getWallElement(wall[i], style);
		if (!element && i == 0)
		{
			warn("Not a valid wall element: style = " + style + ", wall[" + i + "] = " + wall[i] + "; " + uneval(element));
			continue;
		}

		// Add wall elements entity placement arguments to the alignment
		alignment.push({
			"position": Vector2D.sub(wallPosition, new Vector2D(element.indent, 0).rotate(-orientation)),
			"templateName": element.templateName,
			"angle": orientation + element.angle
		});

		// Preset vars for the next wall element
		if (i + 1 < wall.length)
		{
			orientation += element.bend;
			let nextElement = getWallElement(wall[i + 1], style);
			if (!nextElement)
			{
				warn("Not a valid wall element: style = " + style + ", wall[" + (i + 1) + "] = " + wall[i + 1] + "; " + uneval(nextElement));
				continue;
			}

			let distance = (element.length + nextElement.length) / 2 - g_WallStyles[style].overlap;

			// Corrections for elements with indent AND bending
			let indent = element.indent;
			let bend = element.bend;
			if (bend != 0 && indent != 0)
			{
				// Indent correction to adjust distance
				distance += indent * Math.sin(bend);

				// Indent correction to normalize indentation
				wallPosition.add(new Vector2D(indent).rotate(-orientation));
			}

			// Set the next coordinates of the next element in the wall without indentation adjustment
			wallPosition.add(new Vector2D(distance, 0).rotate(-orientation).perpendicular());
		}
	}
	return alignment;
}

/**
 * Center calculation works like getting the center of mass assuming all wall elements have the same "weight"
 *
 * Used to get centerToFirstElement of fortresses by default
 *
 * @param {number} alignment
 * @returns {object} Vector from the center of the set of aligned wallpieces to the first wall element.
 */
function getCenterToFirstElement(alignment)
{
	return alignment.reduce((result, align) => result.sub(Vector2D.div(align.position, alignment.length)), new Vector2D(0, 0));
}

/**
 * Does not support bending wall elements like corners.
 *
 * @param {string} style
 * @param {array} wall
 * @returns {number} The sum length (in terrain cells, not meters) of the provided wall.
 */
function getWallLength(style, wall)
{
	style = validateStyle(style);

	let length = 0;
	let overlap = g_WallStyles[style].overlap;
	for (let element of wall)
		length += getWallElement(element, style).length - overlap;

	return length;
}

/**
 * Makes sure the style exists and, if not, provides a fallback.
 *
 * @param {string} style
 * @param {number} [playerId]
 * @returns {string} Valid style.
 */
function validateStyle(style, playerId = 0)
{
	if (!style || !g_WallStyles[style])
	{
		if (playerId == 0)
			return Object.keys(g_WallStyles)[0];

		style = getCivCode(playerId) + "_stone";
		return !g_WallStyles[style] ? Object.keys(g_WallStyles)[0] : style;
	}
	return style;
}

/**
 * Define the different wall placer functions
 */

/**
 * Places an abitrary wall beginning at the location comprised of the array of elements provided.
 *
 * @param {Vector2D} position
 * @param {array} [wall] - Array of wall element types. Example: ["start", "long", "tower", "long", "end"]
 * @param {string} [style] - Wall style string.
 * @param {number} [playerId] - Identifier of the player for whom the wall will be placed.
 * @param {number} [orientation] - Angle at which the first wall element is placed.
 *                                 0 means "outside" or "front" of the wall is right (positive X) like placeObject
 *                                 It will then be build towards top/positive Y (if no bending wall elements like corners are used)
 *                                 Raising orientation means the wall is rotated counter-clockwise like placeObject
 */
function placeWall(position, wall = [], style, playerId = 0, orientation = 0)
{
	style = validateStyle(style, playerId);

	for (let align of getWallAlignment(position, wall, style, orientation))
		if (align.templateName)
			g_Map.placeEntityPassable(align.templateName, playerId, align.position, align.angle);
}

/**
 * Places an abitrarily designed "fortress" (closed loop of wall elements)
 * centered around a given point.
 *
 * The fortress wall should always start with the main entrance (like
 * "entry" or "gate") to get the orientation correct.
 *
 * @param {Vector2D} centerPosition
 * @param {object} [fortress] - If not provided, defaults to the predefined "medium" fortress type.
 * @param {string} [style] - Wall style string.
 * @param {number} [playerId] - Identifier of the player for whom the wall will be placed.
 * @param {number} [orientation] - Angle the first wall element (should be a gate or entrance) is placed. Default is 0
 */
function placeCustomFortress(centerPosition, fortress, style, playerId = 0, orientation = 0)
{
	fortress = fortress || g_FortressTypes.medium;
	style = validateStyle(style, playerId);

	// Calculate center if fortress.centerToFirstElement is undefined (default)
	let centerToFirstElement = fortress.centerToFirstElement;
	if (centerToFirstElement === undefined)
		centerToFirstElement = getCenterToFirstElement(getWallAlignment(new Vector2D(0, 0), fortress.wall, style));

	// Placing the fortress wall
	let position = Vector2D.sum([
		centerPosition,
		new Vector2D(centerToFirstElement.x, 0).rotate(-orientation),
		new Vector2D(centerToFirstElement.y, 0).perpendicular().rotate(-orientation)
	]);

	placeWall(position, fortress.wall, style, playerId, orientation);
}

/**
 * Places a predefined fortress centered around the provided point.
 *
 * @see Fortress
 *
 * @param {string} [type] - Predefined fortress type, as used as a key in g_FortressTypes.
 */
function placeFortress(centerPosition, type = "medium", style, playerId = 0, orientation = 0)
{
	placeCustomFortress(centerPosition, g_FortressTypes[type], style, playerId, orientation);
}

/**
 * Places a straight wall from a given point to another, using the provided
 * wall parts repeatedly.
 *
 * Note: Any "bending" wall pieces passed will be complained about.
 *
 * @param {Vector2D} startPosition - Approximate start point of the wall.
 * @param {Vector2D} targetPosition - Approximate end point of the wall.
 * @param {array} [wallPart=["tower", "long"]]
 * @param {number} [playerId]
 * @param {boolean} [endWithFirst] - If true, the first wall element will also be the last.
 */
function placeLinearWall(startPosition, targetPosition, wallPart = undefined, style, playerId = 0, endWithFirst = true)
{
	wallPart = wallPart || ["tower", "long"];
	style = validateStyle(style, playerId);

	// Check arguments
	for (let element of wallPart)
		if (getWallElement(element, style).bend != 0)
			warn("placeLinearWall : Bending is not supported by this function, but the following bending wall element was used: " + element);

	// Setup number of wall parts

	let totalLength = startPosition.distanceTo(targetPosition);
	let wallPartLength = getWallLength(style, wallPart);
	let numParts = Math.ceil(totalLength / wallPartLength);
	if (endWithFirst)
		numParts = Math.ceil((totalLength - getWallElement(wallPart[0], style).length) / wallPartLength);

	// Setup scale factor
	let scaleFactor = totalLength / (numParts * wallPartLength);
	if (endWithFirst)
		scaleFactor = totalLength / (numParts * wallPartLength + getWallElement(wallPart[0], style).length);

	// Setup angle
	let wallAngle = getAngle(startPosition.x, startPosition.y, targetPosition.x, targetPosition.y);
	let placeAngle = wallAngle - Math.PI / 2;

	// Place wall entities
	let position = startPosition.clone();
	let overlap = g_WallStyles[style].overlap;
	for (let partIndex = 0; partIndex < numParts; ++partIndex)
		for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
		{
			let wallEle = getWallElement(wallPart[elementIndex], style);

			let wallLength = (wallEle.length - overlap) / 2;
			let dist = new Vector2D(scaleFactor * wallLength, 0).rotate(-wallAngle);

			// Length correction
			position.add(dist);

			// Indent correction
			let place = Vector2D.add(position, new Vector2D(0, wallEle.indent).rotate(-wallAngle));

			if (wallEle.templateName)
				g_Map.placeEntityPassable(wallEle.templateName, playerId, place, placeAngle + wallEle.angle);

			position.add(dist);
		}

	if (endWithFirst)
	{
		let wallEle = getWallElement(wallPart[0], style);
		let wallLength = (wallEle.length - overlap) / 2;
		position.add(new Vector2D(scaleFactor * wallLength, 0).rotate(-wallAngle));
		if (wallEle.templateName)
			g_Map.placeEntityPassable(wallEle.templateName, playerId, position, placeAngle + wallEle.angle);
	}
}

/**
 * Places a (semi-)circular wall of repeated wall elements around a central
 * point at a given radius.
 *
 * The wall does not have to be closed, and can be left open in the form
 * of an arc if maxAngle < 2 * Pi. In this case, the orientation determines
 * where this open part faces, with 0 meaning "right" like an unrotated
 * building's drop-point.
 *
 * Note: Any "bending" wall pieces passed will be complained about.
 *
 * @param {Vector2D} center - Center of the circle or arc.
 * @param (number} radius - Approximate radius of the circle. (Given the maxBendOff argument)
 * @param {array} [wallPart]
 * @param {string} [style]
 * @param {number} [playerId]
 * @param {number} [orientation] - Where the open part of the arc should face, if applicable.
 * @param {number} [maxAngle] - How far the wall should circumscribe the center. Default is Pi * 2 (for a full circle).
 * @param {boolean} [endWithFirst] - If true, the first wall element will also be the last. For full circles, the default is false. For arcs, true.
 * @param {number} [maxBendOff]    Optional. How irregular the circle should be. 0 means regular circle, PI/2 means very irregular. Default is 0 (regular circle)
 */
function placeCircularWall(center, radius, wallPart, style, playerId = 0, orientation = 0, maxAngle = Math.PI * 2, endWithFirst, maxBendOff = 0)
{
	wallPart = wallPart || ["tower", "long"];
	style = validateStyle(style, playerId);

	if (endWithFirst === undefined)
		endWithFirst = maxAngle < Math.PI * 2 - 0.001; // Can this be done better?

	// Check arguments
	if (maxBendOff > Math.PI / 2 || maxBendOff < 0)
		warn("placeCircularWall : maxBendOff should satisfy 0 < maxBendOff < PI/2 (~1.5rad) but it is: " + maxBendOff);

	for (let element of wallPart)
		if (getWallElement(element, style).bend != 0)
			warn("placeCircularWall : Bending is not supported by this function, but the following bending wall element was used: " + element);

	// Setup number of wall parts
	let totalLength = maxAngle * radius;
	let wallPartLength = getWallLength(style, wallPart);
	let numParts = Math.ceil(totalLength / wallPartLength);
	if (endWithFirst)
		numParts = Math.ceil((totalLength - getWallElement(wallPart[0], style).length) / wallPartLength);

	// Setup scale factor
	let scaleFactor = totalLength / (numParts * wallPartLength);
	if (endWithFirst)
		scaleFactor = totalLength / (numParts * wallPartLength + getWallElement(wallPart[0], style).length);

	// Place wall entities
	let actualAngle = orientation + (Math.PI * 2 - maxAngle) / 2;
	let position = Vector2D.add(center, new Vector2D(radius, 0).rotate(-actualAngle));
	let overlap = g_WallStyles[style].overlap;
	for (let partIndex = 0; partIndex < numParts; ++partIndex)
		for (let wallEle of wallPart)
		{
			wallEle = getWallElement(wallEle, style);

			// Width correction
			let addAngle = scaleFactor * (wallEle.length - overlap) / radius;
			let target = Vector2D.add(center, new Vector2D(radius, 0).rotate(-actualAngle - addAngle));
			let place = Vector2D.average([position, target]);
			let placeAngle = actualAngle + addAngle / 2;

			// Indent correction
			place.sub(new Vector2D(wallEle.indent, 0).rotate(-placeAngle));

			// Placement
			if (wallEle.templateName)
				g_Map.placeEntityPassable(wallEle.templateName, playerId, place, placeAngle + wallEle.angle);

			// Prepare for the next wall element
			actualAngle += addAngle;
			position = Vector2D.add(center, new Vector2D(radius, 0).rotate(-actualAngle));
		}

	if (endWithFirst)
	{
		let wallEle = getWallElement(wallPart[0], style);
		let addAngle = scaleFactor * wallEle.length / radius;
		let target = Vector2D.add(center, new Vector2D(radius, 0).rotate(-actualAngle - addAngle))
		let place = Vector2D.average([position, target]);
		let placeAngle = actualAngle + addAngle / 2;
		g_Map.placeEntityPassable(wallEle.templateName, playerId, place, placeAngle + wallEle.angle);
	}
}

/**
 * Places a polygonal wall of repeated wall elements around a central
 * point at a given radius.
 *
 * Note: Any "bending" wall pieces passed will be ignored.
 *
 * @param {Vector2D} centerPosition
 * @param {number} radius
 * @param {array} [wallPart]
 * @param {string} [cornerWallElement] - Wall element to be placed at the polygon's corners.
 * @param {string} [style]
 * @param {number} [playerId]
 * @param {number} [orientation] - Direction the first wall piece or opening in the wall faces.
 * @param {number} [numCorners] - How many corners the polygon will have.
 * @param {boolean} [skipFirstWall] - If the first linear wall part will be left opened as entrance.
 */
function placePolygonalWall(centerPosition, radius, wallPart, cornerWallElement = "tower", style, playerId = 0, orientation = 0, numCorners = 8, skipFirstWall = true)
{
	wallPart = wallPart || ["long", "tower"];
	style = validateStyle(style, playerId);

	let angleAdd = Math.PI * 2 / numCorners;
	let angleStart = orientation - angleAdd / 2;
	let corners = new Array(numCorners).fill(0).map((zero, i) =>
		Vector2D.add(centerPosition, new Vector2D(radius, 0).rotate(-angleStart - i * angleAdd)));

	for (let i = 0; i < numCorners; ++i)
	{
		let angleToCorner = getAngle(corners[i].x, corners[i].y, centerPosition.x, centerPosition.y);
		g_Map.placeEntityPassable(getWallElement(cornerWallElement, style).templateName, playerId, corners[i], angleToCorner);

		if (!skipFirstWall || i != 0)
		{
			let cornerLength = getWallElement(cornerWallElement, style).length / 2;
			let cornerAngle = angleToCorner + angleAdd / 2;
			let targetCorner = (i + 1) % numCorners;
			let cornerPosition = new Vector2D(cornerLength, 0).rotate(-cornerAngle).perpendicular();

			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				Vector2D.sub(corners[i], cornerPosition),
				Vector2D.add(corners[targetCorner], cornerPosition),
				wallPart,
				style,
				playerId);
		}
	}
}

/**
 * Places an irregular polygonal wall consisting of parts semi-randomly
 * chosen from a provided assortment, built around a central point at a
 * given radius.
 *
 * Note: Any "bending" wall pieces passed will be ... I'm not sure. TODO: test what happens!
 *
 * Note: The wallPartsAssortment is last because it's the hardest to set.
 *
 * @param {Vector2D} centerPosition
 * @param {number} radius
 * @param {string} [cornerWallElement] - Wall element to be placed at the polygon's corners.
 * @param {string} [style]
 * @param {number} [playerId]
 * @param {number} [orientation] - Direction the first wallpiece or opening in the wall faces.
 * @param {number} [numCorners] - How many corners the polygon will have.
 * @param {number} [irregularity] - How irregular the polygon will be. 0 = regular, 1 = VERY irregular.
 * @param {boolean} [skipFirstWall] - If true, the first linear wall part will be left open as an entrance.
 * @param {array} [wallPartsAssortment] - An array of wall part arrays to choose from for each linear wall connecting the corners.
 */
function placeIrregularPolygonalWall(centerPosition, radius, cornerWallElement = "tower", style, playerId = 0, orientation = 0, numCorners, irregularity = 0.5, skipFirstWall = false, wallPartsAssortment)
{
	style = validateStyle(style, playerId);
	numCorners = numCorners || randIntInclusive(5, 7);

	// Generating a generic wall part assortment with each wall part including 1 gate lengthened by walls and towers
	// NOTE: It might be a good idea to write an own function for that...
	let defaultWallPartsAssortment = [["short"], ["medium"], ["long"], ["gate", "tower", "short"]];
	let centeredWallPart = ["gate"];
	let extendingWallPartAssortment = [["tower", "long"], ["tower", "medium"]];
	defaultWallPartsAssortment.push(centeredWallPart);
	for (let assortment of extendingWallPartAssortment)
	{
		let wallPart = centeredWallPart;
		for (let j = 0; j < radius; ++j)
		{
			if (j % 2 == 0)
				wallPart = wallPart.concat(assortment);
			else
			{
				assortment.reverse();
				wallPart = assortment.concat(wallPart);
				assortment.reverse();
			}
			defaultWallPartsAssortment.push(wallPart);
		}
	}
	// Setup optional arguments to the default
	wallPartsAssortment = wallPartsAssortment || defaultWallPartsAssortment;

	// Setup angles
	let angleToCover = Math.PI * 2;
	let angleAddList = [];
	for (let i = 0; i < numCorners; ++i)
	{
		// Randomize covered angles. Variety scales down with raising angle though...
		angleAddList.push(angleToCover / (numCorners - i) * (1 + randFloat(-irregularity, irregularity)));
		angleToCover -= angleAddList[angleAddList.length - 1];
	}

	// Setup corners
	let corners = [];
	let angleActual = orientation - angleAddList[0] / 2;
	for (let i = 0; i < numCorners; ++i)
	{
		corners.push(Vector2D.add(centerPosition, new Vector2D(radius, 0).rotate(-angleActual)));

		if (i < numCorners - 1)
			angleActual += angleAddList[i + 1];
	}

	// Setup best wall parts for the different walls (a bit confusing naming...)
	let wallPartLengths = [];
	let maxWallPartLength = 0;
	for (let wallPart of wallPartsAssortment)
	{
		let length = getWallLength(style, wallPart);
		wallPartLengths.push(length);
		if (length > maxWallPartLength)
			maxWallPartLength = length;
	}

	let wallPartList = []; // This is the list of the wall parts to use for the walls between the corners, not to confuse with wallPartsAssortment!
	for (let i = 0; i < numCorners; ++i)
	{
		let bestWallPart = []; // This is a simple wall part not a wallPartsAssortment!
		let bestWallLength = Infinity;
		let targetCorner = (i + 1) % numCorners;
		// NOTE: This is not quite the length the wall will be in the end. Has to be tweaked...
		let wallLength = corners[i].distanceTo(corners[targetCorner]);
		let numWallParts = Math.ceil(wallLength / maxWallPartLength);
		for (let partIndex = 0; partIndex < wallPartsAssortment.length; ++partIndex)
		{
			let linearWallLength = numWallParts * wallPartLengths[partIndex];
			if (linearWallLength < bestWallLength && linearWallLength > wallLength)
			{
				bestWallPart = wallPartsAssortment[partIndex];
				bestWallLength = linearWallLength;
			}
		}
		wallPartList.push(bestWallPart);
	}

	// Place Corners and walls
	for (let i = 0; i < numCorners; ++i)
	{
		let angleToCorner = getAngle(corners[i].x, corners[i].y, centerPosition.x, centerPosition.y);
		g_Map.placeEntityPassable(getWallElement(cornerWallElement, style).templateName, playerId, corners[i], angleToCorner);
		if (!skipFirstWall || i != 0)
		{
			let cornerLength = getWallElement(cornerWallElement, style).length / 2;
			let targetCorner = (i + 1) % numCorners;
			let startAngle = angleToCorner + angleAddList[i] / 2;
			let targetAngle = angleToCorner + angleAddList[targetCorner] / 2;


			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				Vector2D.sub(corners[i], new Vector2D(cornerLength, 0).perpendicular().rotate(-startAngle)),
				Vector2D.add(corners[targetCorner], new Vector2D(cornerLength, 0).rotate(-targetAngle - Math.PI / 2)),
				wallPartList[i],
				style,
				playerId,
				false);
		}
	}
}

/**
 * Places a generic fortress with towers at the edges connected with long
 * walls and gates, positioned around a central point at a given radius.
 *
 * The difference between this and the other two Fortress placement functions
 * is that those place a predefined fortress, regardless of terrain type.
 * This function attempts to intelligently place a wall circuit around
 * the central point taking into account terrain and other obstacles.
 *
 * This is the default Iberian civ bonus starting wall.
 *
 * @param {Vector2D} center - The approximate center coordinates of the fortress
 * @param {number} [radius] - The approximate radius of the wall to be placed.
 * @param {number} [playerId]
 * @param {string} [style]
 * @param {number} [irregularity] - 0 = circle, 1 = very spiky
 * @param {number} [gateOccurence] - Integer number, every n-th walls will be a gate instead.
 * @param {number} [maxTries] - How often the function tries to find a better fitting shape.
 */
function placeGenericFortress(center, radius = 20, playerId = 0, style, irregularity = 0.5, gateOccurence = 3, maxTries = 100)
{
	style = validateStyle(style, playerId);

	// Setup some vars
	let startAngle = randomAngle();
	let actualOff = new Vector2D(radius, 0).rotate(-startAngle);
	let actualAngle = startAngle;
	let pointDistance = getWallLength(style, ["long", "tower"]);

	// Searching for a well fitting point derivation
	let tries = 0;
	let bestPointDerivation;
	let minOverlap = 1000;
	let overlap;
	while (tries < maxTries && minOverlap > g_WallStyles[style].overlap)
	{
		let pointDerivation = [];
		let distanceToTarget = 1000;
		while (true)
		{
			let indent = randFloat(-irregularity * pointDistance, irregularity * pointDistance);
			let tmp = new Vector2D(radius + indent, 0).rotate(-actualAngle - pointDistance / radius);
			let tmpAngle = getAngle(actualOff.x, actualOff.y, tmp.x, tmp.y);

			actualOff.add(new Vector2D(pointDistance, 0).rotate(-tmpAngle));
			actualAngle = getAngle(0, 0, actualOff.x, actualOff.y);
			pointDerivation.push(actualOff.clone());
			distanceToTarget = pointDerivation[0].distanceTo(actualOff);

			let numPoints = pointDerivation.length;
			if (numPoints > 3 && distanceToTarget < pointDistance) // Could be done better...
			{
				overlap = pointDistance - pointDerivation[numPoints - 1].distanceTo(pointDerivation[0]);
				if (overlap < minOverlap)
				{
					minOverlap = overlap;
					bestPointDerivation = pointDerivation;
				}
				break;
			}
		}
		++tries;
	}

	log("placeGenericFortress: Reduced overlap to " + minOverlap + " after " + tries + " tries");

	// Place wall
	for (let pointIndex = 0; pointIndex < bestPointDerivation.length; ++pointIndex)
	{
		let start = Vector2D.add(center, bestPointDerivation[pointIndex]);
		let target = Vector2D.add(center, bestPointDerivation[(pointIndex + 1) % bestPointDerivation.length]);
		let angle = getAngle(start.x, start.y, target.x, target.y);

		let element = (pointIndex + 1) % gateOccurence == 0 ? "gate" : "long";
		element = getWallElement(element, style);

		if (element.templateName)
		{
			let pos = Vector2D.add(start, new Vector2D(start.distanceTo(target) / 2, 0).rotate(-angle));
			g_Map.placeEntityPassable(element.templateName, playerId, pos, angle - Math.PI / 2 + element.angle);
		}

		// Place tower
		start = Vector2D.add(center, bestPointDerivation[(pointIndex + bestPointDerivation.length - 1) % bestPointDerivation.length]);
		angle = getAngle(start.x, start.y, target.x, target.y);

		let tower = getWallElement("tower", style);
		g_Map.placeEntityPassable(tower.templateName, playerId, Vector2D.add(center, bestPointDerivation[pointIndex]), angle - Math.PI / 2 + tower.angle);
	}
}
