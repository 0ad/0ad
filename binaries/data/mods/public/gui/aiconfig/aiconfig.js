var g_AIs; // [ {"id": ..., "data": {"name": ..., "description": ..., ...} }, ... ]
var g_PlayerSlot;

function init(settings)
{
	g_PlayerSlot = settings.playerSlot;

	translateObjectKeys(settings.ais, ["name", "description"]);
	g_AIs = [
		{id: "", data: {name: translateWithContext("ai", "None"), description: translate("AI will be disabled for this player.")}}
	].concat(settings.ais);

	var aiSelection = Engine.GetGUIObjectByName("aiSelection");
	aiSelection.list = [ translate(ai.data.name) for each (ai in g_AIs) ];

	var selected = 0;
	for (var i = 0; i < g_AIs.length; ++i)
	{
		if (g_AIs[i].id == settings.id)
		{
			selected = i;
			break;
		}
	}
	aiSelection.selected = selected;
	
	var aiDiff = Engine.GetGUIObjectByName("aiDifficulty");
    // Translation: AI difficulty level.
	aiDiff.list = [translateWithContext("aiDiff", "Sandbox"), translateWithContext("aiDiff", "Easy"), translateWithContext("aiDiff", "Medium"), translateWithContext("aiDiff", "Hard"), translateWithContext("aiDiff", "Very Hard")];
	aiDiff.selected = settings.difficulty;
}

function selectAI(idx)
{
	var id = g_AIs[idx].id;
	var name = g_AIs[idx].data.name;
	var description = g_AIs[idx].data.description;

	Engine.GetGUIObjectByName("aiDescription").caption = description;
}

function returnAI()
{
	var aiSelection = Engine.GetGUIObjectByName("aiSelection");
	var idx = aiSelection.selected;
	var id = g_AIs[idx].id;
	var name = g_AIs[idx].data.name;

	var difficulty = Engine.GetGUIObjectByName("aiDifficulty").selected;
	
	// Pop the page before calling the callback, so the callback runs
	// in the parent GUI page's context
	Engine.PopGuiPageCB({"id": id, "name": name, "difficulty" : difficulty, "playerSlot" : g_PlayerSlot });
}
