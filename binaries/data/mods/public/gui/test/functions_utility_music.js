/*
	DESCRIPTION	: Audio functions (eg "pick a random sound from a list", "build a playlist") go here.
	NOTES		: 
*/

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

	switch (soundType)
	{
		case "music":
			var randomSoundPath = "audio/music/"
		break;
		case "voice":
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
		return;
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