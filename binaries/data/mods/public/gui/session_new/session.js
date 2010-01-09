function init()
{
	updateDebug();
}

function onSimulationUpdate()
{
	updateDebug();
}

function updateDebug()
{
	var debug = getGUIObjectByName("debug");
	var simState = Engine.GetSimulationState();
	var text = "Simulation:\n" + uneval(simState);
	text += "\n\n";
	for (var ent in g_Selection)
	{
		text += "Entity "+ent+":\n" + uneval(Engine.GetEntityState(ent)) + "\n";
	}
	debug.caption = text;
}
