/**
 * The class allows the player to position structures so that they are aligned
 * with nearby structures.
 */
class ObstructionSnap
{
	getValidEdges(allEdges, position, maxSide)
	{
		let edges = [];
		let dir1 = new Vector2D();
		let dir2 = new Vector2D();
		for (let edge of allEdges)
		{
			let signedDistance = Vector2D.dot(edge.normal, position) -
			                     Vector2D.dot(edge.normal, edge.begin);
			// Negative signed distance means that the template position
			// lays behind the edge.
			if (signedDistance < -this.MinimalDistanceToSnap - maxSide ||
			    signedDistance > this.MinimalDistanceToSnap + maxSide)
				continue;
			dir1.setFrom(edge.begin).sub(edge.end).normalize();
			dir2.setFrom(dir1).mult(-1);
			let offsetDistance = Math.max(
				Vector2D.dot(dir1, position) - Vector2D.dot(dir1, edge.begin),
				Vector2D.dot(dir2, position) - Vector2D.dot(dir2, edge.end));
			if (offsetDistance > this.MinimalDistanceToSnap + maxSide)
				continue;
			// If a projection of the template position on the edge is
			// lying inside the edge then obviously we don't need to
			// account the offset distance.
			if (offsetDistance < 0)
				offsetDistance = 0;
			edge.signedDistance = signedDistance;
			edge.offsetDistance = offsetDistance;
			edges.push(edge);
		}
		return edges;
	}

	// We need a small padding to avoid unnecessary collisions
	// because of loss of accuracy.
	getPadding(edge)
	{
		const snapPadding = 0.05;
		// We don't need to padding for edges with normals directed inside
		// its entity, as we try to snap from an internal side of the edge.
		return edge.order == "ccw" ? 0 : snapPadding;
	}

	// Pick a base edge, it will be the first axis and fix the angle.
	// We can't just pick an edge by signed distance, because we might have
	// a case when one segment is closer by signed distance than another
	// one but much farther by actual (euclid) distance.
	compareEdges(a, b)
	{
		const behindA = a.signedDistance < -this.EPS;
		const behindB = b.signedDistance < -this.EPS;
		const scoreA = Math.abs(a.signedDistance) + a.offsetDistance;
		const scoreB = Math.abs(b.signedDistance) + b.offsetDistance;
		if (Math.abs(scoreA - scoreB) < this.EPS)
		{
			if (behindA != behindB)
				return behindA - behindB;
			if (!behindA)
				return a.offsetDistance - b.offsetDistance;
			return -a.signedDistance - -b.signedDistance;
		}
		return scoreA - scoreB;
	}

	getNearestSizeAlongNormal(width, depth, angle, normal)
	{
		// Front face direction.
		let direction = new Vector2D(0.0, 1.0);
		direction.rotate(angle);
		let dot = direction.dot(normal);
		const threshold = Math.cos(Math.PI / 4.0);
		if (Math.abs(dot) > threshold)
			return [depth, width];
		return [width, depth];
	}

	getPosition(data, template)
	{
		if (!data.snapToEdges || !template.Obstruction || !template.Obstruction.Static)
			return undefined;

		const width = template.Obstruction.Static["@width"] / 2;
		const depth = template.Obstruction.Static["@depth"] / 2;
		const maxSide = Math.max(width, depth);
		let templatePos = Vector2D.from3D(data);
		let templateAngle = data.angle || 0;

		let edges = this.getValidEdges(data.snapToEdges, templatePos, maxSide);
		if (!edges.length)
			return undefined;

		let baseEdge = edges[0];
		for (let edge of edges)
			if (this.compareEdges(edge, baseEdge) < 0)
				baseEdge = edge;
		// Now we have the normal, we need to determine an angle,
		// which side will be snapped first.
		for (let dir = 0; dir < 4; ++dir)
		{
			const angleCandidate = baseEdge.angle + dir * Math.PI / 2;
			// We need to find a minimal angle difference.
			let difference = Math.abs(angleCandidate - templateAngle);
			difference = Math.min(difference, Math.PI * 2 - difference);
			if (difference < Math.PI / 4 + this.EPS)
			{
				templateAngle = angleCandidate;
				break;
			}
		}
		let [sizeToBaseEdge, sizeToPairedEdge] =
			this.getNearestSizeAlongNormal(width, depth, templateAngle, baseEdge.normal);

		let distance = Vector2D.dot(baseEdge.normal, templatePos) - Vector2D.dot(baseEdge.normal, baseEdge.begin);
		templatePos.sub(Vector2D.mult(baseEdge.normal, distance - sizeToBaseEdge - this.getPadding(baseEdge)));
		edges = this.getValidEdges(data.snapToEdges, templatePos, maxSide);
		if (edges.length > 1)
		{
			let pairedEdges = [];
			for (let edge of edges)
			{
				// We have to place a rectangle, so the angle between
				// edges should be 90 degrees.
				if (Math.abs(Vector2D.dot(baseEdge.normal, edge.normal)) > this.EPS)
					continue;
				let newEdge = {
					"begin": edge.end,
					"end": edge.begin,
					"normal": Vector2D.mult(edge.normal, -1),
					"signedDistance": -edge.signedDistance,
					"offsetDistance": edge.offsetDistance,
					"order": "ccw",
				};
				pairedEdges.push(edge);
				pairedEdges.push(newEdge);
			}
			pairedEdges.sort(this.compareEdges.bind(this));
			if (pairedEdges.length)
			{
				let secondEdge = pairedEdges[0];
				for (let edge of pairedEdges)
					if (this.compareEdges(edge, secondEdge) < 0)
						secondEdge = edge;
				let distance = Vector2D.dot(secondEdge.normal, templatePos) - Vector2D.dot(secondEdge.normal, secondEdge.begin);
				templatePos.sub(Vector2D.mult(secondEdge.normal, distance - sizeToPairedEdge - this.getPadding(secondEdge)));
			}
		}
		return {
			"x": templatePos.x,
			"z": templatePos.y,
			"angle": templateAngle
		};
	}
}

ObstructionSnap.prototype.MinimalDistanceToSnap = 5;

ObstructionSnap.prototype.EPS = 1e-3;

Engine.RegisterGlobal("ObstructionSnap", ObstructionSnap);
