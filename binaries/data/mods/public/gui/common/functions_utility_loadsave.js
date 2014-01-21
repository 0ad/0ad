function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function twoDigits(n)
{
	return n < 10 ? "0" + n : n;
}

function generateLabel(metadata, engineInfo)
{
	var t = new Date(metadata.time*1000);
	var date = t.getFullYear()+"-"+twoDigits(1+t.getMonth())+"-"+twoDigits(t.getDate());
	var time = twoDigits(t.getHours())+":"+twoDigits(t.getMinutes())+":"+twoDigits(t.getSeconds());
	var label = "["+date+" "+time+"] ";

	if (engineInfo)
	{
		if (!hasSameVersion(metadata, engineInfo))
			label = "[color=\"red\"]" + label + "[/color]";
		else if (!hasSameMods(metadata, engineInfo))
			label = "[color=\"orange\"]" + label + "[/color]";
	}

	label += metadata.initAttributes.map.replace("maps/","")
		+ (metadata.description ? " - "+metadata.description : "");
	return label;
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
