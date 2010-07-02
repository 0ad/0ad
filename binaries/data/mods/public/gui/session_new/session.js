const GAIA = "Gaia"
const CART = "Cart";
const CELT = "Celt";
const HELE = "Hele";
const IBER = "Iber";
const PERS = "Pers";
const ROME = "Rome";

const CARTHAGINIANS = "Carthaginians";
const ROMANS = "Romans";
const HELLENES = "Hellenes";
const CELTS = "Celts";
const PERSIANS = "Persians";
const IBERIANS = "Iberians";

// Cache dev-mode settings that are frequently or widely used
var g_DevSettings = {
	controlAll: false
};

function init(initData, hotloadData)
{
	if (hotloadData)
	{
		g_Selection.selected = hotloadData.selection;
	}
	else 
	{
		// Starting for the first time:
		startMusic();
	}

	onSimulationUpdate();
}

function leaveGame()
{
	stopMusic();
	endGame();
	Engine.SwitchGuiPage("page_pregame.xml");
}

// Return some data that we'll use when hotloading this file after changes
function getHotloadData()
{
	return { selection: g_Selection.selected };
}

function handleNetMessage(message)
{
	warn("Net message: "+uneval(message));

	switch (message.type)
	{
	case "netstatus":
		var obj = getGUIObjectByName("netStatus");
		switch (message.status)
		{
		case "waiting_for_players":
			obj.caption = "Waiting for other players to connect";
			obj.hidden = false;
			break;
		case "active":
			obj.caption = "";
			obj.hidden = true;
			break;
		case "disconnected":
			obj.caption = "Connection to the server has been lost";
			obj.hidden = false;
			// TODO: we need to give players some way to exit
			break;
		default:
			error("Unrecognised netstatus type "+message.status);
			break;
		}
		break;
	default:
		error("Unrecognised net message type "+message.type);
	}
}

function onTick()
{
	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;
		handleNetMessage(message);
	}

	g_DevSettings.controlAll = getGUIObjectByName("devControlAll").checked;
	// TODO: at some point this controlAll needs to disable the simulation code's
	// player checks (once it has some player checks)

	updateCursor();

	// If the selection changed, we need to regenerate the sim display
	if (g_Selection.dirty)
		onSimulationUpdate();
}

function onSimulationUpdate()
{
	g_Selection.dirty = false;
	var simState = Engine.GuiInterfaceCall("GetSimulationState");

	// If we're called during init when the game is first loading, there will be no simulation yet, so do nothing
	if (!simState)
		return;

	updateDebug(simState);
	updatePlayerDisplay(simState);
	updateSelectionDetails(simState);
}

function updateDebug(simState)
{
	var debug = getGUIObjectByName("debug");

	if (getGUIObjectByName("devDisplayState").checked)
	{
		debug.hidden = false;
	}
	else
	{
		debug.hidden = true;
		return;
	}

	var text = uneval(simState);
	
	var selection = g_Selection.toList();
	if (selection.length)
	{
		var entState = Engine.GuiInterfaceCall("GetEntityState", selection[g_Selection.getPrimary()]);
		if (entState)
		{
			var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
			text += "\n\n" + uneval(entState) + "\n\n" + uneval(template);
		}
	}

	debug.caption = text;
}

function updatePlayerDisplay(simState)
{
	var playerState = simState.players[Engine.GetPlayerID()];

	if (!playerState)
		return;

	getGUIObjectByName("resourceFood").caption = playerState.resourceCounts.food;
	getGUIObjectByName("resourceWood").caption = playerState.resourceCounts.wood;
	getGUIObjectByName("resourceStone").caption = playerState.resourceCounts.stone;
	getGUIObjectByName("resourceMetal").caption = playerState.resourceCounts.metal;
	getGUIObjectByName("resourcePop").caption = playerState.popCount + "/" + playerState.popLimit;
}

//-------------------------------- -------------------------------- -------------------------------- 
// Utility functions
//-------------------------------- -------------------------------- -------------------------------- 

function toTitleCase(string)
{
	if (string.length > 0)
		string = string.charAt(0).toUpperCase() + string.substring(1, string.length).toLowerCase();
	return string;
}

function getTemplateFirstWord(templateName)
{
	return templateName.substring(0, templateName.search("/"))
}

function damageTypesToTextStacked(dmg)
{
	if (!dmg)
		return "(None)";
	return dmg.hack + " Hack\n" + dmg.pierce + " Pierce\n" + dmg.crush + " Crush";
}

function damageTypesToText(dmg)
{
	if (!dmg)
		return "[font=\"serif-12\"](None)[/font]";
	
	var hackLabel = "[font=\"serif-12\"] Hack[/font]";
	var pierceLabel = "[font=\"serif-12\"] Pierce[/font]";
	var crushLabel = "[font=\"serif-12\"] Crush[/font]";
	var hackDamage = dmg.hack;
	var pierceDamage = dmg.pierce;
	var crushDamage = dmg.crush;
	
	var dmgArray = [];
	if (hackDamage) dmgArray.push(hackDamage + hackLabel);
	if (pierceDamage) dmgArray.push(pierceDamage + pierceLabel);
	if (crushDamage) dmgArray.push(crushDamage + crushLabel);

	return dmgArray.join(", ");
}

function isUnitElite(templateName)
{
	var eliteStatus = false;
	var firstWord = getTemplateFirstWord(templateName);
	var endsWith = templateName.substring(templateName.length-2, templateName.length);

	if (firstWord == "units" && endsWith == "_e")
		eliteStatus = true;

	return eliteStatus;
}

function getFullName(template)
{
		var name;
		
		if ((template.name.specific && template.name.generic) && (template.name.specific != template.name.generic))
			name = template.name.specific + " (" + template.name.generic + ")";
		else
			name = template.name.specific || template.name.generic || "???";
		
		return "[font=\"serif-bold-16\"]" + name + "[/font]";
}

function getPortraitSheetName(playerState, entState)
{
	var portraitSheetName = "snPortraitSheet";
	var firstWord = getTemplateFirstWord(entState.template);

	if (firstWord == "gaia") // Find appropriate Gaia icon sheet
	{
		var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);
		var gaiaType  = template.name.generic;
		
		if ((gaiaType == "Rock") || (gaiaType == "Mineral"))
			portraitSheetName += "RockGaia";
		else if ((gaiaType == "Tree") || (gaiaType == "Bush"))
			portraitSheetName += "TreeGaia";
		else if (gaiaType == "Fauna")
			portraitSheetName += "AnimalGaia";
		else
			portraitSheetName += "SpecialGaia";
	}
	else // Find appropriate Civ icon sheet
	{
		var civName = toTitleCase(playerState.civ);
		portraitSheetName += civName;
	}

	return portraitSheetName;
}

function getFormalCivName(civ)
{
	switch (civ)
	{
	case CART:
		return "Carthaginians";
	case CELT:
		return "Celts";
	case HELE:
		return "Hellenes";
	case IBER:
		return "Iberians";
	case PERS:
		return "Persians";
	case ROME:
		return "Romans";
	default:
		return "Gaia";
	}
}
