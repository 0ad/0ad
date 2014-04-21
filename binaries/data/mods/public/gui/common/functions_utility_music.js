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
	var soundArray = Engine.BuildDirEntList(randomSoundPath, "*" + soundSubType + "*", false);
	if (soundArray.length == 0)
	{
		Engine.Console_Write (sprintf("Failed to find sounds matching '*%(subtype)s*'", { soundSubType: subtype }));
		return undefined;
	}
	// Get a random number within the sound's range.
	var randomSound = getRandom (0, soundArray.length-1);
	// Set name of track.
	var randomFileName = soundArray[randomSound];

	// Build path to random audio file.
	randomSoundPath = randomFileName;

	//Engine.Console_Write("Playing " + randomSoundPath + " ...");

	switch (soundType)
	{
		case "music":
			return new MusicSound(randomSoundPath);
		break;
		case "ambient":
			return new AmbientSound(randomSoundPath);
		break;
		case "effect":
			Engine.Console_Write(sprintf("am loading effect '*%(path)s*'", { path: randomSoundPath }));
		break;
		default:
		break;
	}
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
		fadeIn(inHandle, Engine.ConfigDB_GetValue("user", "sound.mastergain"), fadeDuration);
	}

	return true;
}

// ====================================================================


//const AMBIENT_SOUND = "audio/ambient/dayscape/day_temperate_gen_03.ogg";


//const AMBIENT_TEMPERATE = "temperate";
//var currentAmbient;

//function playRandomAmbient(type)
//{
//	switch (type)
//	{
//		case AMBIENT_TEMPERATE:
//			// Seem to need the underscore at the end of "temperate" to avoid crash
//			// (Might be caused by trying to randomly load day_temperate.xml)
//			currentAmbient = newRandomSound("ambient", "temperate_", "dayscape");
//			if (currentAmbient)
//			{
//				currentAmbient.loop();
//				currentAmbient.setGain(0.8);
//			}
//			break;
//
//		default:
//			Engine.Console_Write("Unrecognized ambient type: " + type);
//			break;
//	}
//}
//
//function stopAmbient()
//{
//	if (currentAmbient)
//	{
//		currentAmbient.fade(-1, 0.0, 5.0);
//		currentAmbient = null;
//	}
//}

//const BUTTON_SOUND = "audio/interface/ui/ui_button_longclick.ogg";
//function playButtonSound()
//{
//    var buttonSound = new Sound(BUTTON_SOUND);
//    buttonSound.play();
//}
