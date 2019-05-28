/**
 * Also called from the C++ side when ending the game.
 * The current page can be the summary screen or a message box, so it can't be moved to session/.
 */
function getReplayMetadata()
{
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	let extendedSimState = cmpGuiInterface.GetExtendedSimulationState();
	return {
		"timeElapsed": extendedSimState.timeElapsed,
		"playerStates": extendedSimState.players,
		"mapSettings": Engine.GetInitAttributes().settings
	};
}

Engine.RegisterGlobal("getReplayMetadata", getReplayMetadata);
