var state_before;
var state_after;

function handleInputBeforeGui(ev) {
	if ((ev.type == "hotkeydown" || ev.type == "hotkeyup") && ev.hotkey == "test")
		state_before = Engine.HotkeyIsPressed("test");
	return false;
}

function handleInputAfterGui(ev) {
	if ((ev.type == "hotkeydown" || ev.type == "hotkeyup") && ev.hotkey == "test")
		state_after = Engine.HotkeyIsPressed("test");
	return false;
}
