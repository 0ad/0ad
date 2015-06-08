function init(data)
{
	var mbMainObj = Engine.GetGUIObjectByName("mbMain");
	var mbTitleObj = Engine.GetGUIObjectByName("mbTitleBar");
	var mbTextObj = Engine.GetGUIObjectByName("mbText");

	var mbButton1Obj = Engine.GetGUIObjectByName("mbButton1");
	var mbButton2Obj = Engine.GetGUIObjectByName("mbButton2");
	var mbButton3Obj = Engine.GetGUIObjectByName("mbButton3");

	var mbCancelHotkey = Engine.GetGUIObjectByName("mbCancelHotkey");
	// Default behaviour
	mbCancelHotkey.onPress = function()
	{
		Engine.PopGuiPage();
	};

	// Calculate size
	var mbLRDiff = data.width / 2;     // Message box left/right difference from 50% of screen
	var mbUDDiff = data.height / 2;    // Message box up/down difference from 50% of screen

	var mbSizeString = "50%-" + mbLRDiff + " 50%-" + mbUDDiff + " 50%+" + mbLRDiff + " 50%+" + mbUDDiff;

	mbMainObj.size = mbSizeString;

	// Texts
	mbTitleObj.caption	= data.title;
	mbTextObj.caption 	= data.message;

	if (data.font)
		mbTextObj.font = data.font;

	// Message box modes
	// There is a number of standard modes, and if none of these is used (mbMode == 0), the button captions will be
	// taken from the array mbButtonCaptions; there currently is a maximum of three buttons.
	switch (data.mode)
	{
	case 1:
		// Simple Yes/No question box
		data.buttonCaptions = [translate("Yes"), translate("No")];
		break;
	case 2:
		// Okay-only box
		data.buttonCaptions = [translate("OK")];
		break;
	case 3:
		// Retry/Abort/Ignore box (will we ever need this?!)
		data.buttonCaptions = [translate("Retry"), translate("Ignore"), translate("Abort")];
	default:
		break;
	}

	// Buttons
	var codes = data.buttonCode;
	if (data.buttonCaptions.length >= 1)
	{
		var action = function () 
		{ 
			if (data.callback) 
				Engine.PopGuiPageCB(0); 
			else
				Engine.PopGuiPage(); 
		};

		mbButton1Obj.caption = data.buttonCaptions[0];
		mbButton1Obj.onPress = action;
		mbButton1Obj.hidden = false;

		mbCancelHotkey.onPress = action;
	}
	if (data.buttonCaptions.length >= 2)
	{
		var action = function () 
		{ 
			if (data.callback) 
				Engine.PopGuiPageCB(1); 
			else
				Engine.PopGuiPage(); 
		};

		mbButton2Obj.caption = data.buttonCaptions[1];
		mbButton2Obj.onPress = action;
		mbButton2Obj.hidden = false;
	}
	if (data.buttonCaptions.length >= 3)
	{
		var action = function () 
		{ 
			if (data.callback) 
				Engine.PopGuiPageCB(2); 
			else
				Engine.PopGuiPage(); 
		};

		mbButton3Obj.caption = data.buttonCaptions[2];
		mbButton3Obj.onPress = action;
		mbButton3Obj.hidden = false;
	}

	switch (data.buttonCaptions.length)
	{
	case 1:
		mbButton1Obj.size = "18 100%-46 100%-18 100%-18";
		break;
	case 2:
		mbButton1Obj.size = "18 100%-46 50%-5 100%-18";
		mbButton2Obj.size = "50%+5 100%-46 100%-18 100%-18";
		break;
	case 3:
		mbButton1Obj.size = "18 100%-46 33%-5 100%-18";
		mbButton2Obj.size = "33%+5 100%-46 66%-5 100%-18";
		mbButton3Obj.size = "66%+5 100%-46 100%-18 100%-18";
		break;
	}
}
