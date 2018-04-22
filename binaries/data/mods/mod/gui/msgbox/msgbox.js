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

	// Set button captions and visibility
	let mbButton = [];
	captions.forEach((caption, i) => {
		mbButton[i] = Engine.GetGUIObjectByName("mbButton" + (i + 1));

		let action = function()
		{
			if (data.callback)
				Engine.PopGuiPageCB(i);
			else
				Engine.PopGuiPage();
		};

		mbButton[i].caption = caption;
		mbButton[i].onPress = action;
		mbButton[i].hidden = false;

		// Convention: Cancel is the first button
		if (i == 0)
			mbCancelHotkey.onPress = action;
	});

	// Distribute buttons horizontally
	let y1 = "100%-46";
	let y2 = "100%-18";
	switch (captions.length)
	{
	case 1:
		mbButton[0].size = "18 " + y1 + " 100%-18 " + y2;
		break;
	case 2:
		mbButton[0].size = "18 " + y1 + " 50%-5 " + y2;
		mbButton[1].size = "50%+5 " + y1 + " 100%-18 " + y2;
		break;
	case 3:
		mbButton[0].size = "18 " + y1 + " 33%-5 " + y2;
		mbButton[1].size = "33%+5 " + y1 + " 66%-5 " + y2;
		mbButton[2].size = "66%+5 " + y1 + " 100%-18 " + y2;
		break;
	}
}
