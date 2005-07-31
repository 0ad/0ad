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
			randomSoundPath = "audio/music/"
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
	soundArray = buildFileList(randomSoundPath, "*" + soundSubType + "*", false);
	if (soundArray.length == 0)
	{
		console.write ("Failed to find sounds matching '*"+soundSubType+"*'");
		return;
	}
	// Get a random number within the sound's range.
	randomSound = getRandom (0, soundArray.length-1);
	// Set name of track.
	randomFileName = soundArray[randomSound];

	// Build path to random audio file.
	randomSoundPath = randomFileName;

	console.write("Playing " + randomSoundPath + " ...");

	return new Sound(randomSoundPath);
}

// ====================================================================

function fadeOut (soundHandle, Rate)
{
	// Adjust the gain of a sound until it is zero.

	// (This is a horrible hack, and needs to be replaced with a decent engine function.)

	for (fadeLoop = 1; fadeLoop > 0; fadeLoop = fadeLoop - Rate)
	{
		soundHandle.setGain(fadeLoop);
	}
	
	return true;
}

// ====================================================================

function fadeIn (soundHandle, Gain, Rate)
{
	// Adjust the gain of a sound from zero up to the given value.

	// (This is a horrible hack, and needs to be replaced with a decent engine function.)

	for (fadeLoop = 0; fadeLoop < Gain; fadeLoop = fadeLoop + Rate)
	{
		soundHandle.setGain(fadeLoop);
	}
	
	return true;	
}

// ====================================================================

function crossFade (outHandle, inHandle, Rate)
{
	// Accepts two sound handles. Fades out the first and fades in the second at the specified rate.
	// Note that it plays the in and frees the out while it's at it.

	// (This is a horrible hack, and needs to be replaced with a decent engine function.)

	if (outHandle)
		fadeOut(outHandle, Rate);

	if (inHandle)
	{
		inHandle.play();
		fadeIn(inHandle, 1, Rate);
	}

	if (outHandle)
		outHandle.free();

	return true;
}

// ====================================================================