function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function generateLabel(metadata, engineInfo)
{
	var dateTimeString = Engine.FormatMillisecondsIntoDateString(metadata.time*1000, translate("yyyy-MM-dd HH:mm:ss"));
	var dateString = sprintf(translate("[%(date)s]"), { date: dateTimeString });
	if (engineInfo)
	{
		if (!hasSameVersion(metadata, engineInfo))
			dateString = "[color=\"red\"]" + dateString + "[/color]";
		else if (!hasSameMods(metadata, engineInfo))
			dateString = "[color=\"orange\"]" + dateString + "[/color]";
	}
	if (metadata.description)
		return sprintf(translate("%(dateString)s %(map)s - %(description)s"), { dateString: dateString, map: metadata.initAttributes.map, description: metadata.description });
	else
		return sprintf(translate("%(dateString)s %(map)s"), { dateString: dateString, map: metadata.initAttributes.map });
}

/**
 * Check the version compatibility between the saved game to be loaded and the engine
 */
function hasSameVersion(metadata, engineInfo)
{
	return (metadata.version_major == engineInfo.version_major);
}

/**
 * Check the mod compatibility between the saved game to be loaded and the engine
 */
function hasSameMods(metadata, engineInfo)
{
	if (!metadata.mods)         // only here for backwards compatibility with previous saved games
		var gameMods = [];
	else
		var gameMods = metadata.mods;

	if (gameMods.length != engineInfo.mods.length)
		return false;
	for (var i = 0; i < gameMods.length; ++i)
		if (gameMods[i] != engineInfo.mods[i])
			return false;
	return true;
}
