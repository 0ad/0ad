/**
 * Check the mod compatibility between the saved game to be loaded and the engine.
 * This is a wrapper around an engine function to allow mods to to fancier or specific things.
 */
function hasSameMods(modsA, modsB)
{
	if (!modsA || !modsB)
		return false;
	return Engine.AreModsPlayCompatible(modsA, modsB);
}

/**
 * Print the shorthand identifier of a mod.
 */
function modToString(mod)
{
	// Skip version for play-compatible mods.
	if (mod.ignoreInCompatibilityChecks)
		return mod.name;
	return sprintf(translateWithContext("Mod comparison", "%(mod)s (%(version)s)"), {
		"mod": mod.name,
		"version": mod.version
	});
}

/**
 * Converts a list of mods and their version into a human-readable string.
 */
function modsToString(mods)
{
	return mods.map(mod => modToString(mod)).join(translate(", "));
}

/**
 * Convert the required and active mods and their version into a humanreadable translated string.
 */
function comparedModsString(required, active)
{
	return sprintf(translateWithContext("Mod comparison", "Required: %(mods)s"),
		{ "mods": modsToString(required) }
	) + "\n" + sprintf(translateWithContext("Mod comparison", "Active: %(mods)s"),
		{ "mods": modsToString(active) });
}
