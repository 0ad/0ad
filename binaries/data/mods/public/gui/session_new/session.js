function init()
{
	updateDebug();
}

function onTick()
{
	updateCursor();
}

function onSimulationUpdate()
{
	updateDebug();

	updateUnitDisplay();
}

function updateDebug()
{
	var debug = getGUIObjectByName("debug");
	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	var text = uneval(simState);
	debug.caption = text;
}

function updateUnitDisplay()
{
	var detailsPanel = getGUIObjectByName("unitDetails");
	var commandsPanel = getGUIObjectByName("unitCommands");

	var selection = getEntitySelection();
	if (selection.length == 0)
	{
		detailsPanel.hidden = true;
		commandsPanel.hidden = true;
		return;
	}

	var entState = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
	var template = Engine.GuiInterfaceCall("GetTemplateData", entState.template);

	detailsPanel.hidden = false;
	detailsPanel.caption = uneval(entState)+"\n\n"+uneval(template);

	commandsPanel.hidden = false;

	var i = 0;
	for each (var build in entState.buildEntities)
	{
		var button = getGUIObjectByName("cButton"+i);
		var icon = getGUIObjectByName("cIcon"+i);

		var template = Engine.GuiInterfaceCall("GetTemplateData", build);

		var name;
		if (template.name.specific && template.name.generic)
			name = template.name.specific + " (" + template.name.generic + ")";
		else
			name = template.name.specific || template.name.generic || "???";

		button.hidden = false;
		button.tooltip = "Construct " + name;
		button.onpress = (function(b) { return function() { testBuild(b) } })(build);
			// (need nested functions to get the closure right)

		icon.sprite = "snPortraitSheetHele";
		icon.cell_id = template.name.icon_cell;
		++i;
	}
	for (; i < 8; ++i)
		getGUIObjectByName("cButton"+i).hidden = true;
}
