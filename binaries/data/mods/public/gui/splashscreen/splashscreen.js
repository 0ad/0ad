function init(data)
{
	getGUIObjectByName("mainText").caption = readFile("gui/splashscreen/" + data.page + ".txt");
}
