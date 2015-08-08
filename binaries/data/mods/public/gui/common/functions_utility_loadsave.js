function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function generateLabel(metadata, engineInfo)
{
	var dateTimeString = Engine.FormatMillisecondsIntoDateString(metadata.time*1000, translate("yyyy-MM-dd HH:mm:ss"));
	var dateString = sprintf(translate("\\[%(date)s]"), { date: dateTimeString });
	if (engineInfo)
	{
		if (!hasSameSavegameVersion(metadata, engineInfo) || !hasSameEngineVersion(metadata, engineInfo))
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
function hasSameSavegameVersion(metadata, engineInfo)
{
	return metadata.version_major == engineInfo.version_major;
}

/**
 * Check the version compatibility between the saved game to be loaded and the engine
 */
function hasSameEngineVersion(metadata, engineInfo)
{
	return metadata.engine_version && metadata.engine_version == engineInfo.engine_version;
}

/**
 * Check the mod compatibility between the saved game to be loaded and the engine
 */
function hasSameMods(metadata, engineInfo)
{
	if (!metadata.mods || metadata.mods.length != engineInfo.mods.length)
		return false;
	
	return metadata.mods.every((mod, index) => mod == engineInfo.mods[index]);
}
