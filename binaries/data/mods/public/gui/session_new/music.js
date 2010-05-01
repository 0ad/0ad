var g_CurrentMusic = null;

/*
 * At some point, this ought to be extended to do dynamic music selection and
 * crossfading - it at least needs to pick the music track based on the player's
 * civ and peace/battle
 */

function startMusic()
{
	var gain = 0.5;
	g_CurrentMusic = new Sound("audio/music/germanic_peace_1.ogg");
	if (g_CurrentMusic)
	{
		g_CurrentMusic.loop();
		g_CurrentMusic.fade(0.0, gain, 10.0);
	}
}

function stopMusic()
{
	if (g_CurrentMusic)
	{
		g_CurrentMusic.fade(-1, 0.0, 5.0);
		g_CurrentMusic = null;
	}
}
