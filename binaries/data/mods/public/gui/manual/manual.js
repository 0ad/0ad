function init()
{
	let mainText = Engine.GetGUIObjectByName("mainText");
	let text = Engine.TranslateLines(Engine.ReadFile("gui/manual/intro.txt"));

	let hotkeys = Engine.GetHotkeyMap();

	// Replace anything starting with 'hotkey.' with its hotkey.
	mainText.caption = text.replace(/hotkey\.([a-z0-9_\.]+)/g, (_, k) => formatHotkeyCombinations(hotkeys[k]));
}
