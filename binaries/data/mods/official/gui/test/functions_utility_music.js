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

function newRandomSound(soundType, soundSubType)
{
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
		default:
		break;
	}
	
	// Build path to random audio file.
	randomSoundPath += randomFileName;

	console.write("Playing " + randomSoundPath + " ...");

	return new Sound(randomSoundPath);
}