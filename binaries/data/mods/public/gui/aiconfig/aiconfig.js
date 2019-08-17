/**
 * Is this user in control of game settings (i.e. is a network server, or offline player).
 */
const g_IsController = !Engine.HasNetClient() || Engine.HasNetServer();

var g_PlayerSlot;

var g_AIDescriptions = [{
	"id": "",
	"data": {
		"name": translateWithContext("ai", "None"),
		"description": translate("AI will be disabled for this player.")
	}
}].concat(g_Settings.AIDescriptions);

var g_AIControls = {
	"aiSelection": {
		"labels": g_AIDescriptions.map(ai => ai.data.name),
		"selected": settings => g_AIDescriptions.findIndex(ai => ai.id == settings.id)
	},
	"aiDifficulty": {
		"labels": prepareForDropdown(g_Settings.AIDifficulties).Title,
		"selected": settings => settings.difficulty
	},
	"aiBehavior": {
		"labels": prepareForDropdown(g_Settings.AIBehaviors).Title,
		"selected": settings => g_Settings.AIBehaviors.findIndex(b => b.Name == settings.behavior)
	}
};

function init(settings)
{
	// Remember the player ID that we change the AI settings for
	g_PlayerSlot = settings.playerSlot;

	for (let name in g_AIControls)
	{
		let control = Engine.GetGUIObjectByName(name);
		control.list = g_AIControls[name].labels;
		control.selected = g_AIControls[name].selected(settings);
		control.hidden = !g_IsController;

		let label = Engine.GetGUIObjectByName(name + "Text");
		label.caption = control.list[control.selected];
		label.hidden = g_IsController;
	}

	checkBehavior();
}

function selectAI(idx)
{
	Engine.GetGUIObjectByName("aiDescription").caption = g_AIDescriptions[idx].data.description;
}

/** Behavior choice does not apply for Sandbox level */
function checkBehavior()
{
	if (g_Settings.AIDifficulties[Engine.GetGUIObjectByName("aiDifficulty").selected].Name != "sandbox")
	{
		Engine.GetGUIObjectByName("aiBehavior").enabled = true;
		return;
	}
	let aiBehavior = Engine.GetGUIObjectByName("aiBehavior");
	aiBehavior.enabled = false;
	aiBehavior.selected = g_Settings.AIBehaviors.findIndex(b => b.Name == "balanced");
}

function returnAI(save = true)
{
	let idx = Engine.GetGUIObjectByName("aiSelection").selected;
	Engine.PopGuiPage({
		"save": save,
		"id": g_AIDescriptions[idx].id,
		"name": g_AIDescriptions[idx].data.name,
		"difficulty": Engine.GetGUIObjectByName("aiDifficulty").selected,
		"behavior": g_Settings.AIBehaviors[Engine.GetGUIObjectByName("aiBehavior").selected].Name,
		"playerSlot": g_PlayerSlot
	});
}
