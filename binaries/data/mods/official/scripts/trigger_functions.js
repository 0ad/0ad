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

//Effects

function trigObjectTask(subjects, target, task)
{
	for ( var i = 0; i < subjects.length; ++i )
		getEntityByHandle(subjects[i]).orderFromTriggers(
			ORDER_GENERIC, getEntityByHandle(target[0]), task);
}

function trigObjectGoto(subjects, destination)
{
	for ( var i = 0; i < subjects.length; ++i )
		getEntityByHandle(subjects[i]).orderFromTriggers(
			ORDER_GOTO, destination.x, destination.y);
}
