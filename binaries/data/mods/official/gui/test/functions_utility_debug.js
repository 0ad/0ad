function UpdateFPSCounter()
{
	getGUIObjectByName('FPS_Counter').caption = "FPS: " + getFPS();
}

