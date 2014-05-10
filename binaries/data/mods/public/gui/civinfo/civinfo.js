
var g_CivData = {};

function init(settings)
{
	// Initialize civ list
	initCivNameList();
	// TODO: Separate control for factions?
}

// Sort by culture, then by code equals culture and then by name ignoring case
function sortByCultureAndName(a, b)
{
	if (a.culture < b.culture)
		return -1;
	if (a.culture > b.culture)
		return 1;

	// Same culture
	// First code == culture
	if (a.code == a.culture)
		return -1;
	if (b.code == b.culture)
		return 1;

	// Then alphabetically by name (ignoring case)
	return sortNameIgnoreCase(a, b);
}

// Initialize the dropdown containing all the available civs
function initCivNameList()
{
	// Cache map data
	g_CivData = loadCivData();

	var civList = [ { "name": civ.Name, "culture": civ.Culture, "code": civ.Code } for each (civ in g_CivData) ];

	// Alphabetically sort the list, ignoring case
	civList.sort(sortByCultureAndName);

	// Indent sub-factions
	var civListNames = [ ((civ.code == civ.culture)?"":"   ")+civ.name for each (civ in civList) ];
	var civListCodes = [ civ.code for each (civ in civList) ];

	// Set civ control
	var civSelection = Engine.GetGUIObjectByName("civSelection");
	civSelection.list = civListNames;
	civSelection.list_data = civListCodes;
	civSelection.selected = 0;
}

// Function to make first char of string big
function bigFirstLetter(str, size)
{
	return '[font="sans-bold-'+(size+6)+'"]' + str[0] + '[/font]' + '[font="sans-bold-'+size+'"]' + str.substring(1) + '[/font]';
}

// Heading font - bold and mixed caps
function heading(string, size)
{
	var textArray = string.split(" ");
	
	for(var i = 0; i < textArray.length; ++i)
	{
		var word = textArray[i];
		var wordCaps = word.toUpperCase();
		
		// Check if word is capitalized, if so assume it needs a big first letter
		// Check if toLowerCase changes the character to avoid false positives from special signs
		if (word.length && word[0].toLowerCase() != word[0])
			textArray[i] = bigFirstLetter(wordCaps, size);
		else
			textArray[i] = '[font="sans-bold-'+size+'"]' + wordCaps + '[/font]';	// TODO: Would not be necessary if we could do nested tags
	}
	
	return textArray.join(" ");
}

function escapeChars(str)
{
	return str.replace(/\[/g, "&#91;").replace(/\]/g, "&#93;").replace(/"/g, "&#34;");
};

function subHeading(obj)
{
	var string = "";
	if (!obj.Name)
		return string;
	string += '[color="white"][font="sans-bold-14"]' + obj.Name + '[/font] ';
	if (obj.History)
		string += '[icon="iconInfo" tooltip="' + escapeChars(obj.History) + '" tooltip_style="civInfoTooltip"]';
	if (obj.Description)
		string += '\n     ' + obj.Description;
	string += '\n[/color]';
	return string;
}

// Called when user selects civ from dropdown
function selectCiv(code)
{

	var civInfo = g_CivData[code];
	
	if(!civInfo)
		error(sprintf("Error loading civ data for \"%(code)s\"", { code: code }));

	// Update civ gameplay display
	Engine.GetGUIObjectByName("civGameplayHeading").caption = heading(sprintf(translate("%(civilization)s Gameplay"), { civilization: civInfo.Name }), 16);

	// Bonuses
	var bonusCaption = heading(translatePlural("Civilization Bonus", "Civilization Bonuses", civInfo.CivBonuses.length), 12) + '\n';
	
	for(var i = 0; i < civInfo.CivBonuses.length; ++i)
		bonusCaption = subHeading(civInfo.CivBonuses[i]);

	bonusCaption += heading(translatePlural("Team Bonus", "Team Bonuses", civInfo.TeamBonuses.length), 12) + '\n';
	
	for(var i = 0; i < civInfo.TeamBonuses.length; ++i)
		bonusCaption += subHeading(civInfo.TeamBonuses[i]);

	Engine.GetGUIObjectByName("civBonuses").caption = bonusCaption;


	// Special techs / buildings
	var techCaption = heading(translate("Special Technologies"), 12) + '\n';
	
	for(var i = 0; i < civInfo.Factions.length; ++i)
	{
		var faction = civInfo.Factions[i];
		for(var j = 0; j < faction.Technologies.length; ++j)
			techCaption += subHeading(faction.Technologies[j]);
	}

	techCaption += heading(translatePlural("Special Building", "Special Buildings", civInfo.Structures.length), 12) + '\n';
	
	for(var i = 0; i < civInfo.Structures.length; ++i)
		techCaption += subHeading(civInfo.Structures[i]);
	
	Engine.GetGUIObjectByName("civTechs").caption = techCaption;


	// Heroes
	var heroCaption = heading(translate("Heroes"), 12) + '\n';
	
	for(var i = 0; i < civInfo.Factions.length; ++i)
	{
		var faction = civInfo.Factions[i];
		for(var j = 0; j < faction.Heroes.length; ++j)
			heroCaption += subHeading(faction.Heroes[j]);
		heroCaption += '\n';
	}
	
	Engine.GetGUIObjectByName("civHeroes").caption = heroCaption;


	// Update civ history display
	Engine.GetGUIObjectByName("civHistoryHeading").caption = heading(sprintf(translate("History of the %(civilization)s"), { civilization: civInfo.Name }), 16);
	Engine.GetGUIObjectByName("civHistoryText").caption = civInfo.History;
}
