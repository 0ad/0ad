if (!g_VictoryConditions)
	var g_VictoryConditions = {};

var vc = {};

vc.name = translate("Wonder");
vc.description = translate("Build a wonder to win");

/*
NOT SUPPORTED YET
vc.requirementsCheck = function(mapSettings)
{
	// nothing required
	return true;
};
*/

vc.scripts = ["scripts/Conquest.js", "scripts/WonderVictory.js"];

g_VictoryConditions.wonder = vc;
vc = null;
