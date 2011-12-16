// Returns an command suitable for ProcessCommand() based on the rally point data.
// This assumes that the rally point has a valid position.
function getRallyPointCommand(cmpRallyPoint, spawnedEnts)
{
	// Look and see if there is a command in the rally point data, otherwise just walk there.
	var data = cmpRallyPoint.GetData();
	var rallyPos = cmpRallyPoint.GetPosition();
	var command = undefined;
	if (data && data.command)
	{
		command = data.command;
	}
	else
	{
		command = "walk";
	}

	// If a target was set and the target no longer exists them just walk to the rally point.
	if (data && data.target)
	{
		if (! Engine.QueryInterface(data.target, IID_Position))
		{
			command = "walk";
		}
	}

	switch (command)
	{
	case "gather":
		return {
			"type": "gatherNearPosition",
			"entities": spawnedEnts,
			"x": rallyPos.x,
			"z": rallyPos.z,
			"resourceType": data.resourceType,
			"queued": false
		};
		break;
	case "repair": 
	case "build":
		return {
			"type": "repair",
			"entities": spawnedEnts,
			"target": data.target,
			"queued": false,
			"autocontinue": true
		};
		break;
	case "garrison": 
		return {
			"type": "garrison",
			"entities": spawnedEnts,
			"target": data.target,
			"queued": false
		};
	}
	
	// default return value
	return {
		"type": "walk",
		"entities": spawnedEnts,
		"x": rallyPos.x,
		"z": rallyPos.z,
		"queued": false
	};
}

Engine.RegisterGlobal("getRallyPointCommand", getRallyPointCommand);
