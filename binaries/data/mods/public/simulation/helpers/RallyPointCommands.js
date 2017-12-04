// Returns an array of commands suitable for ProcessCommand() based on the rally point data.
// This assumes that the rally point has a valid position.
function GetRallyPointCommands(cmpRallyPoint, spawnedEnts)
{
	let data = cmpRallyPoint.GetData();
	let rallyPos = cmpRallyPoint.GetPositions();
	let ret = [];
	for (let i = 0; i < rallyPos.length; ++i)
	{
		// Look and see if there is a command in the rally point data, otherwise just walk there.
		let command = data[i] && data[i].command ? data[i].command : "walk";

		// If a target was set and the target no longer exists, or no longer
		// has a valid position, then just walk to the rally point.
		if (data[i] && data[i].target)
		{
			let cmpPosition = Engine.QueryInterface(data[i].target, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld())
				command = command == "gather" ? "gather-near-position" : "walk";
		}

		switch (command)
		{
		case "gather":
			ret.push({
				"type": "gather",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true
			});
			break;
		case "gather-near-position":
			ret.push({
				"type": "gather-near-position",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"resourceType": data[i].resourceType,
				"resourceTemplate": data[i].resourceTemplate,
				"queued": true
			});
			break;
		case "repair":
		case "build":
			ret.push({
				"type": "repair",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true,
				"autocontinue": i == rallyPos.length - 1
			});
			break;
		case "garrison":
			ret.push({
				"type": "garrison",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true
			});
			break;
		case "attack-walk":
			ret.push({
				"type": "attack-walk",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"targetClasses": data[i].targetClasses,
				"queued": true
			});
			break;
		case "patrol":
			ret.push({
				"type": "patrol",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"target": data[i].target,
				"targetClasses": data[i].targetClasses,
				"queued": true
			});
			break;
		case "attack":
			ret.push({
				"type": "attack",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true,
			});
			break;
		case "trade":
			ret.push({
				"type": "setup-trade-route",
				"entities": spawnedEnts,
				"source": data[i].source,
				"target": data[i].target,
				"route": undefined,
				"queued": true
			});
			break;
		default:
			ret.push({
				"type": "walk",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"queued": true
			});
			break;
		}
	}

	// special case: trade route with waypoints
	// (we do not modify the RallyPoint before, as we want it to be displayed with all way-points)
	if (ret.length > 1 && ret[ret.length-1].type == "setup-trade-route")
	{
		let route = [];
		let waypoints = ret.length - 1;

		for (let i = 0; i < waypoints; ++i)
		{
			if (ret[i].type != "walk")
			{
				route = undefined;
				break;
			}
			route.push({ "x": ret[i].x, "z": ret[i].z });
		}

		if (route && route.length > 0)
		{
			ret.splice(0, waypoints);
			ret[0].route = route;
		}
	}

	return ret;
}

Engine.RegisterGlobal("GetRallyPointCommands", GetRallyPointCommands);
