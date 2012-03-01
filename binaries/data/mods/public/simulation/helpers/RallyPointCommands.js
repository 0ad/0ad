// Returns an command suitable for ProcessCommand() based on the rally point data.
// This assumes that the rally point has a valid position.
function GetRallyPointCommand(cmpRallyPoint, spawnedEnts)
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

	// If a target was set and the target no longer exists, or no longer
	// has a valid position, then just walk to the rally point.
	if (data && data.target)
	{
		var cmpPosition = Engine.QueryInterface(data.target, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
		{
			command = "walk";
		}
	}

	switch (command)
	{
	case "gather":
		return {
			"type": "gather-near-position",
			"entities": spawnedEnts,
			"x": rallyPos.x,
			"z": rallyPos.z,
			"resourceType": data.resourceType,
			"queued": false
		};
	case "repair": 
	case "build":
		return {
			"type": "repair",
			"entities": spawnedEnts,
			"target": data.target,
			"queued": false,
			"autocontinue": true
		};
	case "garrison": 
		return {
			"type": "garrison",
			"entities": spawnedEnts,
			"target": data.target,
			"queued": false
		};
	default:
		return {
			"type": "walk",
			"entities": spawnedEnts,
			"x": rallyPos.x,
			"z": rallyPos.z,
			"queued": false
		};
	}
}

Engine.RegisterGlobal("GetRallyPointCommand", GetRallyPointCommand);
