// Audio functions (eg "pick a random sound from a list", "build a playlist") will go here.
// In the meantime, a quick run-down of the basic commands:

// Save the specified audio file to handle "s".
// s = new Sound( "audio/music/menu_track.ogg" );

// Play the sound stored at handle "s" one time (it'll be automatically freed when the playback ends):
// s.play();

// Play the sound stored at handle "s" continuously:
// s.loop();

// Close "s" and free it from memory (use in conjunction with loop()):
// s.free();

// Adjust the gain (volume) of a sound (floating point range between 0 (silent) and 1 (max volume)).
// s.SetGain(value);

// ====================================================================

function newRandomSound(soundType, soundSubType, soundPrePath)
{
	// Return a random audio file by category, to be assigned to a handle.

	switch (soundType)
	{
		case "music":
			randomSoundPath = "audio/music/"
			switch (soundSubType)
			{
				case "peace":
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 4);

					switch (randomSound)
					{
						case 1:
							randomFileName = "germanic_peace_1.ogg"
						break;
						case 2:
							randomFileName = "germanic_peace_2.ogg"
						break;
						case 3:
							randomFileName = "germanic_peace_3.ogg"
						break;
						case 4:
							randomFileName = "roman_peace_1.ogg"
						break;
					}
				break;
				case "theme":
					randomFileName = "menu_track.ogg"
				break;
				default:
				break;
			}
		break;
		case "voice":
			randomSoundPath = soundPrePath + "/";
			switch (soundSubType)
			{
				case "attack": // Unit given an attack order
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 6);

					switch (randomSound)
					{
						case 1:
							randomFileName = "Attack-Attackx.ogg"
						break;
						case 2:
							randomFileName = "Attack-Chargex.ogg"
						break;
						case 3:
							randomFileName = "Attack-Engagex.ogg"
						break;
						case 4:
							randomFileName = "Attack-ForMyFamily.ogg"
						break;
						case 5:
							randomFileName = "Attack-Warcry.ogg"
						break;
						case 6:
							randomFileName = "Attack-ZeusSaviourandVictory.ogg"
						break;
					}
				break;
				case "command": // Unit given a generic order
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 8);

					switch (randomSound)
					{
						case 1:
							randomFileName = "Command-AsYouWish.ogg"
						break;
						case 2:
							randomFileName = "Command-ByYourCommand.ogg"
						break;
						case 3:
							randomFileName = "Command-ImComing.ogg"
						break;
						case 4:
							randomFileName = "Command-IObey.ogg"
						break;
						case 5:
							randomFileName = "Command-MyLiege.ogg"
						break;
						case 6:
							randomFileName = "Command-OnMyWay.ogg"
						break;
						case 7:
							randomFileName = "Command-WithMyHonour.ogg"
						break;
						case 8:
							randomFileName = "Command-YesMyLord.ogg"
						break;
					}
				break;
				case "select": // Unit clicked by player
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 10);

					switch (randomSound)
					{
						case 1:
							randomFileName = "Select-AtYourService.ogg"
						break;
						case 2:
							randomFileName = "Select-HowMayIServeYouq.ogg"
						break;
						case 3:
							randomFileName = "Select-MyLiegeq.ogg"
						break;
						case 4:
							randomFileName = "Select-MyLordq.ogg"
						break;
						case 5:
							randomFileName = "Select-OrdersSireq.ogg"
						break;
						case 6:
							randomFileName = "Select-Ready.ogg"
						break;
						case 7:
							randomFileName = "Select-ReadySire.ogg"
						break;
						case 8:
							randomFileName = "Select-Yesq.ogg"
						break;
						case 9:
							randomFileName = "Select-YourOrdersq.ogg"
						break;
						case 10:
							randomFileName = "Select-YourWishq.ogg"
						break;
					}
				break;
				case "hit": // Unit attacks with a grunt
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 6);

					switch (randomSound)
					{
						case 1:
							randomFileName = "Hit1.ogg"
						break;
						case 2:
							randomFileName = "Hit2.ogg"
						break;
						case 3:
							randomFileName = "Hit3.ogg"
						break;
						case 4:
							randomFileName = "Hit4.ogg"
						break;
						case 5:
							randomFileName = "Hit5.ogg"
						break;
						case 6:
							randomFileName = "Hit6.ogg"
						break;
					}
				break;
				case "pain": // Unit is hurt, possibly death blow
					// Get a random number within the sound's range.
					// (Later we'll need to change this to an array selection of filenames.)
					randomSound = getRandom(1, 9);

					switch (randomSound)
					{
						case 1:
							randomFileName = "Pain1.ogg"
						break;
						case 2:
							randomFileName = "Pain2.ogg"
						break;
						case 3:
							randomFileName = "Pain3.ogg"
						break;
						case 4:
							randomFileName = "Pain4.ogg"
						break;
						case 5:
							randomFileName = "Pain5.ogg"
						break;
						case 6:
							randomFileName = "Pain6.ogg"
						break;
						case 7:
							randomFileName = "Pain7.ogg"
						break;
						case 8:
							randomFileName = "Pain8.ogg"
						break;
						case 9:
							randomFileName = "Pain9.ogg"
						break;
					}
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
	
	// Build path to random audio file.
	randomSoundPath += randomFileName;

	console.write("Playing " + randomSoundPath + " ...");

	return new Sound(randomSoundPath);
}

// ====================================================================

function FadeOut (soundHandle, Rate)
{
	// Adjust the gain of a sound until it is zero.

	for (fadeLoop = 1; fadeLoop > 0; fadeLoop = fadeLoop - Rate)
	{
		soundHandle.setGain(fadeLoop);
	}
	
	return true;
}

// ====================================================================

function FadeIn (soundHandle, Gain, Rate)
{
	// Adjust the gain of a sound from zero up to the given value.

	for (fadeLoop = 0; fadeLoop < Gain; fadeLoop = fadeLoop + Rate)
	{
		soundHandle.setGain(fadeLoop);
	}
	
	return true;	
}

// ====================================================================

function CrossFade (outHandle, inHandle, Rate)
{
	// Accepts two sound handles. Fades out the first and fades in the second at the specified rate.
	// Note that it plays the in and frees the out while it's at it.

	FadeOut(outHandle, Rate);
	inHandle.play();
	FadeIn(inHandle, 1, Rate);
	outHandle.free();

	return true;
}

// ====================================================================