
function initMusic()
{
	// Probably will need to put this in a place where it won't get
	// reinitialized after every match. Otherwise, it will not remember
	// the current track

	// Might need to use pregame for that sort of setup and move all the
	// menu stuff to a main_menu page

	global.music = new Music();
}

function Music()
{
	this.RELATIVE_MUSIC_PATH = "audio/music/";
	this.MUSIC_PEACE = "peace";
	this.MUSIC_BATTLE = "battle";

	this.tracks = {
		MAIN_MENU_TRACK : "main_menu",
		VICTORY_TRACK : "win_1",
		DEFEAT_TRACK : "gen_loss_track",
		DEFEAT_CUE_TRACK : "gen_loss_cue"
	};

	this.states = {
		OFF : 0,
		MENU : 1,
		PEACE : 2,
		BATTLE : 3,
		VICTORY :4,
		DEFEAT : 5,
		DEFEAT_CUE : 6
	};

	this.musicGain = 0.3;

	this.peaceTracks = [];
	this.battleTracks = [];

	this.currentState = 0;
	this.oldState = 0;

	this.currentMusic = null;
}

// Needs to be called in onTick() to work
Music.prototype.update = function()
{
	if (this.currentState != this.oldState)
	{
		this.oldState = this.currentState;

		switch (this.currentState)
		{
			case this.states.OFF:
			    	if (this.currentMusic)
				{
					this.currentMusic.fade(-1, 0.0, 5.0);
					this.currentMusic = null;
				}
				break;

			case this.states.MENU:
				this.switchMusic(this.tracks.MAIN_MENU_TRACK, 0.0, true);
				break;

			case this.states.PEACE:
				this.switchMusic(this.getRandomPeaceTrack(), 0.0, true);
				break;

			case this.states.BATTLE:
				this.switchMusic(this.getRandomBattleTrack(), 0.0, true);
				break;

			case this.states.VICTORY:
				this.switchMusic(this.tracks.VICTORY_TRACK, 0.0, true);
				break;

			case this.states.DEFEAT:
				this.switchMusic(this.tracks.DEFEAT_TRACK, 0.0, true);
				break;

			case this.states.DEFEAT_CUE:
				this.switchMusic(this.tracks.DEFEAT_CUE_TRACK, 0.0, false);
				break;

			default:
				console.write("Unknown music state: " + this.currentState);
				break;
		}
	}
	else if (!this.isMusicPlaying())
	{
		console.write("not playing " + this.currentState);

		if (this.currentState == this.states.DEFEAT_CUE)
			this.setState(this.states.DEFEAT);
	}
};

Music.prototype.setState = function(state)
{
	this.currentState = state;
};

Music.prototype.storeTracks = function(civMusic)
{
	for each (var music in civMusic)
	{
		var type = music["Type"];

		switch (type)
		{
			case this.MUSIC_PEACE:
				this.peaceTracks.push(music["File"]);
				break;

			case this.MUSIC_BATTLE:
				this.battleTracks.push(music["File"]);
				break;

			default:
				console.write("Unrecognized music type: " + type);
				break;
		}
	}
};

Music.prototype.getRandomPeaceTrack = function()
{
	return this.peaceTracks[getRandom(0, this.peaceTracks.length-1)];
};

Music.prototype.getRandomBattleTrack = function()
{
	return this.battleTracks[getRandom(0, this.battleTracks.length-1)];
};

Music.prototype.switchMusic = function(track, fadeInPeriod, isLooping)
{
	if (this.currentMusic)
	{
		this.currentMusic.fade(-1, 0.0, 5.0);
		this.currentMusic = null;
	}


console.write(this.RELATIVE_MUSIC_PATH + track + ".ogg");

	this.currentMusic = new Sound(this.RELATIVE_MUSIC_PATH + track + ".ogg");

console.write(this.currentMusic);


	if (this.currentMusic)
	{
		if (isLooping)
			this.currentMusic.loop();
		else
			this.currentMusic.play();

		if (fadeInPeriod)
			this.currentMusic.fade(0.0, this.musicGain, fadeInPeriod);
	}
};

Music.prototype.isMusicPlaying = function()
{
	if (this.currentMusic)
		return true;

	return false;
};

Music.prototype.startMusic = function()
{
	this.setState(this.states.PEACE);
};

Music.prototype.stopMusic = function()
{
	this.setState(this.states.OFF);
};