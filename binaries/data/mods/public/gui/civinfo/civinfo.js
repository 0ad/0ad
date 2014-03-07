
var g_CivData = {};
var TEXTCOLOR = "white"

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
	return '[font="serif-bold-'+(size+6)+'"]' + str[0] + '[/font]' + '[font="serif-bold-'+size+'"]' + str.substring(1) + '[/font]';
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
			textArray[i] = '[font="serif-bold-'+size+'"]' + wordCaps + '[/font]';	// TODO: Would not be necessary if we could do nested tags
	}
	
	return textArray.join(" ");
}

// Called when user selects civ from dropdown
function selectCiv(code)
{
	var civInfo = g_CivData[code];
	
	if(!civInfo)
		error("Error loading civ data for \""+code+"\"");

	// Update civ gameplay display
	Engine.GetGUIObjectByName("civGameplayHeading").caption = heading(civInfo.Name+" Gameplay", 16);


	// Bonuses
	var bonusCaption = heading("Civilization Bonus"+(civInfo.CivBonuses.length == 1 ? "" : "es"), 12) + '\n';
	
	for(var i = 0; i < civInfo.CivBonuses.length; ++i)
	{
		bonusCaption += '[color="' + TEXTCOLOR + '"][font="serif-bold-14"]' + civInfo.CivBonuses[i].Name + '[/font] [icon="iconInfo" tooltip="'
                    + civInfo.CivBonuses[i].History + '" tooltip_style="civInfoTooltip"]\n     ' + civInfo.CivBonuses[i].Description + '\n[/color]';
	}

	bonusCaption += heading("Team Bonus"+(civInfo.TeamBonuses.length == 1 ? "" : "es"), 12) + '\n';
	
	for(var i = 0; i < civInfo.TeamBonuses.length; ++i)
	{
		bonusCaption += '[color="' + TEXTCOLOR + '"][font="serif-bold-14"]' + civInfo.TeamBonuses[i].Name + '[/font] [icon="iconInfo" tooltip="'
                    + civInfo.TeamBonuses[i].History + '" tooltip_style="civInfoTooltip"]\n     ' + civInfo.TeamBonuses[i].Description + '\n[/color]';
	}
	
	Engine.GetGUIObjectByName("civBonuses").caption = bonusCaption;


	// Special techs / buildings
	var techCaption = heading("Special Technologies", 12) + '\n';
	
	for(var i = 0; i < civInfo.Factions.length; ++i)
	{
		var faction = civInfo.Factions[i];
		for(var j = 0; j < faction.Technologies.length; ++j)
		{
			techCaption += '[color="' + TEXTCOLOR + '"][font="serif-bold-14"]' + faction.Technologies[j].Name + '[/font] [icon="iconInfo" tooltip="'
                            + faction.Technologies[j].History + '" tooltip_style="civInfoTooltip"]\n     ' + faction.Technologies[j].Description + '\n[/color]';
		}
	}

	techCaption += heading("Special Building"+(civInfo.Structures.length == 1 ? "" : "s"), 12) + '\n';
	
	for(var i = 0; i < civInfo.Structures.length; ++i)
	{
		techCaption += '[color="' + TEXTCOLOR + '"][font="serif-bold-14"]' + civInfo.Structures[i].Name + '[/font][/color] [icon="iconInfo" tooltip="'
                    + civInfo.Structures[i].History + '" tooltip_style="civInfoTooltip"]\n';
	}
	
	Engine.GetGUIObjectByName("civTechs").caption = techCaption;


	// Heroes
	var heroCaption = heading("Heroes", 12) + '\n';
	
	for(var i = 0; i < civInfo.Factions.length; ++i)
	{
		var faction = civInfo.Factions[i];
		for(var j = 0; j < faction.Heroes.length; ++j)
		{
			heroCaption += '[color="' + TEXTCOLOR + '"][font="serif-bold-14"]' + faction.Heroes[j].Name + '[/font][/color] [icon="iconInfo" tooltip="'
                            + faction.Heroes[j].History + '" tooltip_style="civInfoTooltip"]\n';
		}
		heroCaption += '\n';
	}
	
	Engine.GetGUIObjectByName("civHeroes").caption = heroCaption;


	// Update civ history display
	Engine.GetGUIObjectByName("civHistoryHeading").caption = heading("History of the " + civInfo.Name, 16);
	Engine.GetGUIObjectByName("civHistoryText").caption = civInfo.History;
}
