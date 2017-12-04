/**
 * Display selectable civs only.
 */
const g_CivData = loadCivData(true, false);

/**
 * Initialize the dropdown containing all the available civs.
 */
function init(settings)
{
	var civList = Object.keys(g_CivData).map(civ => ({ "name": g_CivData[civ].Name, "code": civ })).sort(sortNameIgnoreCase);
	var civSelection = Engine.GetGUIObjectByName("civSelection");

	civSelection.list = civList.map(civ => civ.name);
	civSelection.list_data = civList.map(civ => civ.code);
	civSelection.selected = 0;
}

/**
 * Give the first character a larger font.
 */
function bigFirstLetter(str, size)
{
	return '[font="sans-bold-' + (size + 6) + '"]' + str[0] + '[/font]' + '[font="sans-bold-' + size + '"]' + str.substring(1) + '[/font]';
}

/**
 * Set heading font - bold and mixed caps
 *
 * @param string {string}
 * @param size {number} - Font size
 * @returns {string}
 */
function heading(string, size)
{
	var textArray = string.split(" ");

	for (let i in textArray)
	{
		var word = textArray[i];
		var wordCaps = word.toUpperCase();

		// Check if word is capitalized, if so assume it needs a big first letter
		// Check if toLowerCase changes the character to avoid false positives from special signs
		if (word.length && word[0].toLowerCase() != word[0])
			textArray[i] = bigFirstLetter(wordCaps, size);
		else
			textArray[i] = '[font="sans-bold-' + size + '"]' + wordCaps + '[/font]';	// TODO: Would not be necessary if we could do nested tags
	}

	return textArray.join(" ");
}

/**
 * Prepends a backslash to all quotation marks.
 * @param str {string}
 * @returns {string}
 */
function escapeQuotation(str)
{
	return str.replace(/"/g, "\\\"");
}

/**
 * Returns a styled concatenation of Name, History and Description of the given object.
 *
 * @param obj {Object}
 * @returns {string}
 */
function subHeading(obj)
{
	if (!obj.Name)
		return "";
	let string = '[font="sans-bold-14"]' + obj.Name + '[/font] ';
	if (obj.History)
		string += '[icon="iconInfo" tooltip="' + escapeQuotation(obj.History) + '" tooltip_style="civInfoTooltip"]';
	if (obj.Description)
		string += '\n     ' + obj.Description;
	return coloredText(string + "\n", "white");
}

/**
 * Updates the GUI after the user selected a civ from dropdown.
 *
 * @param code {string}
 */
function selectCiv(code)
{
	var civInfo = g_CivData[code];

	if(!civInfo)
		error(sprintf("Error loading civ data for \"%(code)s\"", { "code": code }));

	// Update civ gameplay display
	Engine.GetGUIObjectByName("civGameplayHeading").caption = heading(sprintf(translate("%(civilization)s Gameplay"), { "civilization": civInfo.Name }), 16);

	// Bonuses
	var bonusCaption = heading(translatePlural("Civilization Bonus", "Civilization Bonuses", civInfo.CivBonuses.length), 12) + '\n';
	for (let bonus of civInfo.CivBonuses)
		bonusCaption += subHeading(bonus);

	// Team Bonuses
	bonusCaption += heading(translatePlural("Team Bonus", "Team Bonuses", civInfo.TeamBonuses.length), 12) + '\n';
	for (let bonus of civInfo.TeamBonuses)
		bonusCaption += subHeading(bonus);

	Engine.GetGUIObjectByName("civBonuses").caption = bonusCaption;

	// Special techs
	var techCaption = heading(translate("Special Technologies"), 12) + '\n';
	for (let faction of civInfo.Factions)
		for (let technology of faction.Technologies)
			techCaption += subHeading(technology);

	// Special buildings
	techCaption += heading(translatePlural("Special Building", "Special Buildings", civInfo.Structures.length), 12) + '\n';
	for (let structure of civInfo.Structures)
		techCaption += subHeading(structure);

	Engine.GetGUIObjectByName("civTechs").caption = techCaption;

	// Heroes
	var heroCaption = heading(translate("Heroes"), 12) + '\n';
	for (let faction of civInfo.Factions)
	{
		for (let hero of faction.Heroes)
			heroCaption += subHeading(hero);
		heroCaption += '\n';
	}
	Engine.GetGUIObjectByName("civHeroes").caption = heroCaption;

	// Update civ history display
	Engine.GetGUIObjectByName("civHistoryHeading").caption = heading(sprintf(translate("History of the %(civilization)s"), { "civilization": civInfo.Name }), 16);
	Engine.GetGUIObjectByName("civHistoryText").caption = civInfo.History;
}
