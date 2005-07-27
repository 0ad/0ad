function UpdateFPSCounter(mouse)
{
	g_mouse_x = mouse.x;
	g_mouse_y = mouse.y;
	getGUIObjectByName('FPS_Counter').caption = "FPS: " + getFPS() + " " + g_mouse_x + " " + g_mouse_y;
}

