function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.ReadFile("gui/splashscreen/" + data.page + ".txt");

}

