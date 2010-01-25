function init()
{
	updateDebug();
}

function onSimulationUpdate()
{
	updateDebug();

	updateBuildButton();
}

function updateDebug()
{
	var debug = getGUIObjectByName("debug");
	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	var text = "Simulation:\n" + uneval(simState);
	text += "\n\n";
	for (var ent in g_Selection)
	{
		text += "Entity "+ent+":\n" + uneval(Engine.GuiInterfaceCall("GetEntityState", ent)) + "\n";
	}
	debug.caption = text;
}

function updateBuildButton()
{
	var selection = getEntitySelection();
	if (selection.length)
	{
		var entity = Engine.GuiInterfaceCall("GetEntityState", selection[0]);
		if (entity.buildEntities && entity.buildEntities.length)
		{
			var ent = entity.buildEntities[0];
			getGUIObjectByName("testBuild").caption = "Construct "+ent;
			getGUIObjectByName("testBuild").onpress = function() { testBuild(ent) };
			getGUIObjectByName("testBuild").hidden = false;
			return;
		}
	}
	getGUIObjectByName("testBuild").hidden = true;
}
