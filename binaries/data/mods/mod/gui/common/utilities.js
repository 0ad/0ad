function distributeButtonsHorizontally(button, captions)
{
	const y1 = "100%-46";
	const y2 = "100%-18";
	switch (captions.length)
	{
	case 1:
		button[0].size = "18 " + y1 + " 100%-18 " + y2;
		break;
	case 2:
		button[0].size = "18 " + y1 + " 50%-5 " + y2;
		button[1].size = "50%+5 " + y1 + " 100%-18 " + y2;
		break;
	case 3:
		button[0].size = "18 " + y1 + " 33%-5 " + y2;
		button[1].size = "33%+5 " + y1 + " 66%-5 " + y2;
		button[2].size = "66%+5 " + y1 + " 100%-18 " + y2;
		break;
	}
}

function setButtonCaptionsAndVisibitily(button, captions, cancelHotkey, name)
{
	captions.forEach((caption, i) => {
		button[i] = Engine.GetGUIObjectByName(name + (i + 1));
		button[i].caption = caption;
		button[i].hidden = false;
		button[i].onPress = () => {
			Engine.PopGuiPage(i);
		};

		if (i == 0)
			cancelHotkey.onPress = button[i].onPress;
	});
}
