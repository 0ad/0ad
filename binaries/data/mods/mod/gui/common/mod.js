/**
 * Check the mod compatibility between the saved game to be loaded and the engine
 */
function hasSameMods(modsA, modsB)
{
	if (!modsA || !modsB || modsA.length != modsB.length)
		return false;
	// Mods must be loaded in the same order. 0: modname, 1: modversion
	return modsA.every((mod, index) => [0, 1].every(i => mod[i] == modsB[index][i]));
}

/**
 * Converts a list of mods and their version into a human-readable string.
 */
function modsToString(mods)
{
	return mods.map(mod => sprintf(translateWithContext("Mod comparison", "%(mod)s (%(version)s)"), {
			"mod": mod[0],
			"version": mod[1]
		})).join(translate(", "));
}

/**
 * Convert the required and active mods and their version into a humanreadable translated string.
 */
function comparedModsString(required, active)
{
	return sprintf(translateWithContext("Mod comparison", "Required: %(mods)s"),
			{ "mods": modsToString(required) }) + "\n" +
		sprintf(translateWithContext("Mod comparison", "Active: %(mods)s"),
			{ "mods": modsToString(active) });
}
