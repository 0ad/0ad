/*
	DESCRIPTION	: Audio functions (eg "pick a random sound from a list", "build a playlist") go here.
	NOTES		:
*/


/*
 * These constants are used in session and summary
 */
var g_MusicGain = 0.3;

const RELATIVE_MUSIC_PATH = "audio/music/";
var g_PeaceTracks = [];
var g_BattleTracks = [];

const MAIN_MENU = "main_menu";
const DEFEAT_CUE = "gen_loss_cue";
const DEFEAT_MUSIC = "gen_loss_track";
const VICTORY_MUSIC = "win_1";
//const AMBIENT_SOUND = "audio/ambient/dayscape/day_temperate_gen_03.ogg";
const BUTTON_SOUND = "audio/interface/ui/ui_button_longclick.ogg";

// ====================================================================

// Quick run-down of the basic audio commands:

// Save the specified audio file to handle "s".
// s = new Sound( "audio/music/menu_track.ogg" );

// Play the sound stored at handle "s" one time (it'll be automatically freed when the playback ends):
// s.play();

// Play the sound stored at handle "s" continuously:
// s.loop();

// Close "s" and free it from memory (use in conjunction with loop()):
// s.free();

// Adjust the gain (volume) of a sound (floating point range between 0 (silent) and 1 (max volume)).
// s.setGain(value);

// ====================================================================

function newRandomSound(soundType, soundSubType, soundPrePath)
{
	// Return a random audio file by category, to be assigned to a handle.
	var randomSoundPath;

	switch (soundType)
	{
		case "music":
			randomSoundPath = "audio/music/"
		break;
		case "ambient":
			randomSoundPath = "audio/ambient/" + soundPrePath + "/";
		break;
		case "effect":
			randomSoundPath = soundPrePath + "/";
		break;
		default:
		break;
	}

	// Get names of sounds (attack, command, select, hit, pain).
	// or
	// Get names of "peace", "menu" (theme) and "battle" tracks.
	var soundArray = buildDirEntList(randomSoundPath, "*" + soundSubType + "*", false);
	if (soundArray.length == 0)
	{
		console.write ("Failed to find sounds matching '*"+soundSubType+"*'");
		return undefined;
	}
	// Get a random number within the sound's range.
	var randomSound = getRandom (0, soundArray.length-1);
	// Set name of track.
	var randomFileName = soundArray[randomSound];

	// Build path to random audio file.
	randomSoundPath = randomFileName;

	//console.write("Playing " + randomSoundPath + " ...");

	return new Sound(randomSoundPath);
}

// ====================================================================

function fadeOut (soundHandle, fadeDuration)
{
	// Adjust the gain of a sound until it is zero.
	// The sound is automatically freed when finished fading.
	soundHandle.fade(-1, 0, fadeDuration)

	return true;
}

// ====================================================================

function fadeIn (soundHandle, finalGain, fadeDuration)
{
	// Adjust the gain of a sound from zero up to the given value.
	soundHandle.fade(0, finalGain, fadeDuration)

	return true;
}

// ====================================================================

function crossFade (outHandle, inHandle, fadeDuration)
{
	// Accepts two sound handles. Over the given duration,
	// fades out the first while fading in the second.
	// Note that it plays the in and frees the out while it's at it.

	if (outHandle)
		fadeOut(outHandle, fadeDuration);

	if (inHandle)
	{
		inHandle.play();
		fadeIn(inHandle, g_ConfigDB.system["sound.mastergain"], fadeDuration);
	}

	return true;
}

// ====================================================================




/*
 * At some point, this ought to be extended to do dynamic music selection and
 * crossfading - it at least needs to pick the music track based on the player's
 * civ and peace/battle
 */

/*
 * These functions are used in session and summary
 *
 */

function storeTracks(civMusic)
{
	for each (var music in civMusic)
	{
		if ("peace" == music["Type"])
		{
			g_PeaceTracks.push(music["File"]);
		}
		else if ("battle" == music["Type"])
		{
			g_BattleTracks.push(music["File"]);
		}
	}
}

function getRandomPeaceTrack()
{
	return RELATIVE_MUSIC_PATH + g_PeaceTracks[getRandom(0, g_PeaceTracks.length-1)];
}

function getRandomBattleTrack()
{
	return RELATIVE_MUSIC_PATH + g_BattleTracks[getRandom(0, g_BattleTracks.length-1)];
}

function playMainMenuMusic()
{
	if (global.curr_music)
		switchMusic(MAIN_MENU, 0.0, true);
	else
		playMusic(MAIN_MENU, 0.0, true);
}

function playButtonSound()
{
    var buttonSound = new Sound(BUTTON_SOUND);
    buttonSound.play();
}

function stopMainMenuMusic()
{
	if (global.main_menu_music)
		global.main_menu_music.fade(-1, 0.0, 5.0);
}


function playDefeatMusic()
{
	switchMusic(DEFEAT_CUE, 0.0, false);
	switchMusic(DEFEAT_MUSIC, 10.0, true);
}

function playVictoryMusic()
{
	switchMusic(VICTORY_MUSIC, 0.0, true);
}

function startSessionSounds(civMusic)
{
	storeTracks(civMusic);
	playAmbientSounds();
	playRandomCivMusic();
}

function startMusic()
{
    console.write(getRandomPeaceTrack());


    console.write(global.curr_music);
    playRandomCivMusic();
    console.write(global.curr_music);
}

function playRandomCivMusic()
{
	global.curr_music = new Sound(getRandomPeaceTrack());
	if (global.curr_music)
	{
		global.curr_music.loop();
		global.curr_music.fade(0.0, g_MusicGain, 10.0);
	}
}

function playAmbientSounds()
{
	// Seem to need the underscore at the end of "temperate" to avoid crash
	// (Might be caused by trying to randomly load day_temperate.xml)
	global.curr_ambient = newRandomSound("ambient", "temperate_", "dayscape");
	console.write(global.curr_ambient);
	if (global.curr_ambient)
	{
		global.curr_ambient.loop();
		global.curr_ambient.setGain(0.8);
	}
}

function playMusic(track, fadeInPeriod, isLooping)
{
	global.curr_music = new Sound(RELATIVE_MUSIC_PATH + track + ".ogg");

	if (global.curr_music)
	{
		if (isLooping)
			global.curr_music.loop();
		else
			global.curr_music.play();

		if (fadeInPeriod)
			global.curr_music.fade(0.0, g_MusicGain, fadeInPeriod);
	}
}

function switchMusic(track, fadeInPeriod, isLooping)
{
	if (global.curr_music)
		global.curr_music.fade(-1, 0.0, 5.0);

	playMusic(track, fadeInPeriod, isLooping);
}

function stopSound()
{
	stopMusic();
	stopAmbient();
}

function stopMusic()
{
	if (global.curr_music)
	{
		global.curr_music.fade(-1, 0.0, 5.0);
		global.curr_music = null;
	}
}

function stopAmbientSounds()
{
	if (global.curr_ambient)
	{
		global.curr_ambient.fade(-1, 0.0, 5.0);
		global.curr_ambient = null;
	}
}

function isMusicPlaying()
{
	if (global.curr_music)
		return true;

	return false;
}

//function isEndingMusicPlaying()
//{
//	if (global.curr_music)
//	{
//		if (global.curr_music[DEFEAT_CUE] ||
//		    global.curr_music[DEFEAT_MUSIC] ||
//		    global.curr_music[VICTORY_MUSIC])
//		{
//			return true;
//		}
//	}
//
//	return false;
//}
//
//
//function isMusicPlaying()
//{
//	if (global.curr_music)
//		return global.curr_music.isPlaying();
//
//	return false;
//}
