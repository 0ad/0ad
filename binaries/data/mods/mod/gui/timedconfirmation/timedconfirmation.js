/**
 * Currently limited to at most 3 buttons per message box.
 * The convention is to have "cancel" appear first.
 */
function init(data)
{
	Engine.GetGUIObjectByName("tmcTitleBar").caption = data.title;

	const textObj = Engine.GetGUIObjectByName("tmcText");
	textObj.caption = data.message;

	updateDisplayedTimer(data.timeout);

	Engine.GetGUIObjectByName("tmcTimer").caption = data.timeout;
	if (data.font)
		textObj.font = data.font;

	const cancelHotkey = Engine.GetGUIObjectByName("tmcCancelHotkey");
	cancelHotkey.onPress = Engine.PopGuiPage;

	const lRDiff = data.width / 2;
	const uDDiff = data.height / 2;
	Engine.GetGUIObjectByName("tmcMain").size = "50%-" + lRDiff + " 50%-" + uDDiff + " 50%+" + lRDiff + " 50%+" + uDDiff;

	const captions = data.buttonCaptions || [translate("OK")];

	// Set button captions and visibility
	const button = [];
	setButtonCaptionsAndVisibitily(button, captions, cancelHotkey, "tmcButton");
	distributeButtonsHorizontally(button, captions);
}

function onTick()
{
	const timerObj = Engine.GetGUIObjectByName("tmcTimer");
	let time = +timerObj.caption;
	--time;
	if (time < 1)
		Engine.GetGUIObjectByName("tmcButton1").onPress();

	timerObj.caption = time;
	updateDisplayedTimer(time);
}

function updateDisplayedTimer(time)
{
	Engine.GetGUIObjectByName("tmcTimerDisplay").caption = Math.ceil(time / 100);
}
