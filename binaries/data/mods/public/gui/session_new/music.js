var g_CurrentMusic = null;
var g_CurrentAmbient = null;

var g_MusicGain = 0.3;

/*
 * At some point, this ought to be extended to do dynamic music selection and
 * crossfading - it at least needs to pick the music track based on the player's
 * civ and peace/battle
 */

function startMusic()
{
	g_CurrentAmbient = new Sound("audio/ambient/dayscape/day_temperate_gen_03.ogg");
	if (g_CurrentAmbient)
	{
		g_CurrentAmbient.loop();
		g_CurrentAmbient.setGain(0.8);
	}

	g_CurrentMusic = new Sound("audio/music/germanic_peace_1.ogg");
	if (g_CurrentMusic)
	{
		g_CurrentMusic.loop();
		g_CurrentMusic.fade(0.0, g_MusicGain, 10.0);
	}
}

function switchMusic(track, fadeInPeriod)
{
	if (g_CurrentMusic)
		g_CurrentMusic.fade(-1, 0.0, 5.0);

	g_CurrentMusic = new Sound("audio/music/" + track + ".ogg");

	if (g_CurrentMusic)
	{
		g_CurrentMusic.loop();
		if (fadeInPeriod)
			g_CurrentMusic.fade(0.0, g_MusicGain, fadeInPeriod);
	}
}

function stopMusic()
{
	if (g_CurrentMusic)
	{
		g_CurrentMusic.fade(-1, 0.0, 5.0);
		g_CurrentMusic = null;
	}

	if (g_CurrentAmbient)
	{
		g_CurrentAmbient.fade(-1, 0.0, 5.0);
		g_CurrentAmbient = null;
	}
}
