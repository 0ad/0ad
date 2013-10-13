// Returns an array of commands suitable for ProcessCommand() based on the rally point data.
// This assumes that the rally point has a valid position.
function GetRallyPointCommands(cmpRallyPoint, spawnedEnts)
{
	var data = cmpRallyPoint.GetData();
	var rallyPos = cmpRallyPoint.GetPositions();
	var ret = [];
	for(var i = 0; i < rallyPos.length; ++i)
	{
		// Look and see if there is a command in the rally point data, otherwise just walk there.
		var command = undefined;
		if (data[i] && data[i].command)
		{
			command = data[i].command;
		}
		else
		{
			command = "walk";
		}

		// If a target was set and the target no longer exists, or no longer
		// has a valid position, then just walk to the rally point.
		if (data[i] && data[i].target)
		{
			var cmpPosition = Engine.QueryInterface(data[i].target, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld())
			{
				command = "walk";
			}
		}

		switch (command)
		{
		case "gather":
			ret.push( {
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
			ret.push( {
				"type": "repair",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true,
				"autocontinue": (i == rallyPos.length-1)
			});
			break;
		case "garrison": 
			ret.push( {
				"type": "garrison",
				"entities": spawnedEnts,
				"target": data[i].target,
				"queued": true
			});
			break;
		case "attack-walk":
			ret.push( {
				"type": "attack-walk",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"queued": true
			});
			break;
		case "trade":
			ret.push( {
				"type": "setup-trade-route",
				"entities": spawnedEnts,
				"source": data[i].source,
				"target": data[i].target,
				"queued": true
			});
			break;
		default:
			ret.push( {
				"type": "walk",
				"entities": spawnedEnts,
				"x": rallyPos[i].x,
				"z": rallyPos[i].z,
				"queued": true
			});
			break;
		}
	}
	return ret;
}

Engine.RegisterGlobal("GetRallyPointCommands", GetRallyPointCommands);
