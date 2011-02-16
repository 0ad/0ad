function init(data)
{
	getGUIObjectByName("mainText").caption = readFile("gui/manual/" + data.page + ".txt");
}
