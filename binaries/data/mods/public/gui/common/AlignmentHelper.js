/**
 * This is a helper class to align edges of a set of GUIObjects.
 * The class is designed to either vertically or horizontally align the GUIObjects.
 */
class AlignmentHelper
{
	/**
	 * @param {string} direction - Either min or max. Deciding whether we should move all objects to the minimal (left or top most) or maximal (right or bottom most) position.
	 */
	constructor(direction)
	{
		if (direction != "max" && direction != "min")
			error("Invalid alignment direction.");

		this.direction = direction;
		// An Object of Objects containing the GUIObjects and their requested alignment details.
		this.objectData = {};

		this.defaultValue = this.direction == "max" ? -Infinity : Infinity;
	}

	/**
	 * @param {Object} GUIObject - A GUIObject to be aligned.
	 * @param {string} edge - One of left, right, top and bottom. Determining the edge to change position for this object.
	 * @param {number} wantedPosition - The requested position of the edge.
	 */
	setObject(GUIObject, edge, wantedPosition = this.defaultValue)
	{
		this.objectData[GUIObject.name] = {
			"GUIObject": GUIObject,
			"edge": edge,
			"wantedPosition": wantedPosition
		};

		this.align();
	}

	align()
	{
		let value = this.defaultValue;
		for (const objectName in this.objectData)
			value = Math[this.direction](value, this.objectData[objectName].wantedPosition);

		for (const objectName in this.objectData)
		{
			const objectSize = this.objectData[objectName].GUIObject.size;
			objectSize[this.objectData[objectName].edge] = value;
			this.objectData[objectName].GUIObject.size = objectSize;
		}
	}
}
