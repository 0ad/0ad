/*
	DESCRIPTION	: Audio functions (eg "pick a random sound from a list", "build a playlist") go here.
	NOTES		:
*/


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
