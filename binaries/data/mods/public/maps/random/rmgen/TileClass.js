/**
 * Class that can be tagged to any tile. Can be used to constrain placers and entity placement to given areas.
 */
class TileClass {

	constructor(size)
	{
		this.size = size;
		this.width = Math.ceil(size / 16); // Need one entry per 16 tiles as each tile takes a single bit
		this.inclusionGrid = new Uint16Array(this.size * this.width);
	}

	/**
	 * Returns true if the given position is part of the tileclass.
	 */
	has(position)
	{
		if (position.x < 0 || position.x >= this.size || position.y < 0 || position.y >= this.size)
			return 0;
		// x >> 4 == Math.floor(x / 16); used to find the integer this x is included in
		// x & 0xF == x % 16; used to find the bit position of the given x
		return this.inclusionGrid[position.y * this.width + (position.x >> 4)] & (1 << (position.x & 0xF));
	}

	/**
	 * Adds the given position to the tileclass.
	 */
	add(position)
	{
		if (position.x < 0 || position.x >= this.size || position.y < 0 || position.y >= this.size)
			return;
		this.inclusionGrid[position.y * this.width + (position.x >> 4)] |= 1 << (position.x & 0xF);
	}

	/**
	 * Removes the given position to the tileclass.
	 */
	remove(position)
	{
		if (position.x < 0 || position.x >= this.size || position.y < 0 || position.y >= this.size)
			return;
		this.inclusionGrid[position.y * this.width + (position.x >> 4)] &= ~(1 << (position.x & 0xF));
	}

	/**
	 * Count the number of tiles in the tileclass within the given radius of the given position.
	 * Can return either the total number of members or nonmembers.
	 */
	countInRadius(position, radius, returnMembers)
	{
		let members = 0;
		let total = 0;
		const radius2 = radius * radius;
		const [x, y] = [position.x, position.y];

		const yMin = Math.max(Math.ceil(y - radius), 0);
		const yMax = Math.min(Math.floor(y + radius), this.size - 1);
		for (let iy = yMin; iy <= yMax; ++iy)
		{
			const dy = iy - y;
			const dy2 = dy * dy;
			const delta = Math.sqrt(radius2 - dy2);
			const xMin = Math.max(Math.ceil(x - delta), 0);
			const xMax = Math.min(Math.floor(x + delta), this.size - 1);

			const indexXMin = xMin >> 4;
			const indexXMax = xMax >> 4;
			const indexY = iy * this.width;
			for (let indexX = indexXMin; indexX <= indexXMax; ++indexX)
			{
				const imin = indexX == indexXMin ? xMin & 0xF : 0;
				const imax = indexX == indexXMax ? xMax & 0xF : 15;
				total += imax - imin + 1;
				if (this.inclusionGrid[indexY + indexX])
					for (let i = imin; i <= imax; ++i)
						if (this.inclusionGrid[indexY + indexX] & (1 << i))
							++members;
			}
		}

		return returnMembers ? members : total - members;
	}

	/**
	 * Counts the number of tiles marked in the tileclass within the given radius of the given position.
	 */
	countMembersInRadius(position, radius)
	{
		return this.countInRadius(position, radius, true);
	}

	/**
	 * Counts the number of tiles not marked in the tileclass within the given radius of the given position.
	 */
	countNonMembersInRadius(position, radius)
	{
		return this.countInRadius(position, radius, false);
	}
}
