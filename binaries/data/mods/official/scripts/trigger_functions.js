function trigGetAlwaysTrue()
{
	return true;
}

function trigPlayerResourceCount(player, resource)
{
	return players[player].resources[resource];
}

function trigPlayerUnitCount(player, unit)
{
	var unitCount = getPlayerUnitCount(player, unit);
	return unitCount;
}

//Loop through player's unit list and check for significant entities i.e. units or buildings which can produce units
function trigPlayerSigEntities(player)
{
	var unitNames = new Array(3);
	Array[0] = "Unit";
	Array[1] = "Town";
	Array[2] = "CivilCentre";	//(May need to be expanded)
	var sum = 0;
	for ( var i = 0; i < unitNames.length; ++i )
		sum += getPlayerUnitCount(player, Array[i]);
	return sum;
}

//Effects

function trigObjectTask(subjects, target, task)
{
	for ( var i = 0; i < subjects.length; ++i )
		getEntityByUnitID(subjects[i]).orderFromTriggers(
			ORDER_GENERIC, getEntityByUnitID(target[0]), task);
}

function trigObjectGoto(subjects, destination)
{
	for ( var i = 0; i < subjects.length; ++i )
		getEntityByUnitID(subjects[i]).orderFromTriggers(
			ORDER_GOTO, destination.x, destination.y);
}

function trigEndGame()
{
	console.write("The game has ended...We can pretend, anyway");
}
