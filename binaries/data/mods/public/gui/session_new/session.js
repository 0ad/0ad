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

function changeNetStatus(status)
{
	var obj = getGUIObjectByName("netStatus");
	switch (status)
	{
	case "normal":
		obj.caption = "";
		obj.hidden = true;
		break;
	case "waiting_for_connect":
		obj.caption = "Waiting for other players to connect";
		obj.hidden = false;
		break;
	default:
		error("Unexpected net status "+status);
	}
}

function onTick()
{
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
	updateSelectionDetails();
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
	(hackDamage?  dmgArray.push(hackDamage +  hackLabel) : "");
	(pierceDamage?  dmgArray.push(pierceDamage +  pierceLabel) : "");
	(crushDamage?  dmgArray.push(crushDamage  +  crushLabel) : "");

	return dmgArray.join(", ");
}

function isUnitElite(templateName)
{
	var eliteStatus = false;
	var firstWord = templateName.substring(0, templateName.search("/"));
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
