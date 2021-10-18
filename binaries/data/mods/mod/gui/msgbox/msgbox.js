/**
 * Currently limited to at most 3 buttons per message box.
 * The convention is to have "cancel" appear first.
 */
function init(data)
{
	// Set title
	Engine.GetGUIObjectByName("mbTitleBar").caption = data.title;

	// Set subject
	let mbTextObj = Engine.GetGUIObjectByName("mbText");
	mbTextObj.caption = data.message;
	if (data.font)
		mbTextObj.font = data.font;

	// Default behaviour
	let mbCancelHotkey = Engine.GetGUIObjectByName("mbCancelHotkey");
	mbCancelHotkey.onPress = Engine.PopGuiPage;

	// Calculate size
	let mbLRDiff = data.width / 2;
	let mbUDDiff = data.height / 2;
	Engine.GetGUIObjectByName("mbMain").size = "50%-" + mbLRDiff + " 50%-" + mbUDDiff + " 50%+" + mbLRDiff + " 50%+" + mbUDDiff;

	let captions = data.buttonCaptions || [translate("OK")];

	let mbButton = [];
	setButtonCaptionsAndVisibitily(mbButton, captions, mbCancelHotkey, "mbButton");
	distributeButtonsHorizontally(mbButton, captions);
}
