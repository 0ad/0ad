
function initMusic()
{
	// Probably will need to put this in a place where it won't get
	// reinitialized after every match. Otherwise, it will not remember
	// the current track

	// Might need to use pregame for that sort of setup and move all the
	// menu stuff to a main_menu page

	if (!global.music)
		global.music = new Music();
}


// =============================================================================
// Music class for handling music states (requires onTick)
// =============================================================================
function Music()
{
	this.reference = this;

	this.RELATIVE_MUSIC_PATH = "audio/music/";
	this.MUSIC = {
		PEACE: "peace",
		BATTLE: "battle",
		VICTORY: "victory",
		DEFEAT: "defeat",
		DEFEAT_CUE: "defeat_cue"
	};

	this.tracks = {
		MENU: ["main_menu.ogg"],
		PEACE: [],
		BATTLE: [],
		VICTORY : ["Victory_Track.ogg"],
		DEFEAT : ["gen_loss_track.ogg"],
		DEFEAT_CUE : ["gen_loss_cue.ogg"]
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

	this.currentState = 0;
	this.oldState = 0;

	this.currentMusic = null;

	// timer for delay between tracks
	this.timer = [];
	this.time = Date.now();
}

// "reference" refers to this instance of Music (needed if called from the timer)
Music.prototype.setState = function(state)
{
	this.reference.currentState = state;
	this.updateState();
};

Music.prototype.updateState = function()
{
	if (this.currentState != this.oldState)
	{
		this.oldState = this.currentState;

		switch (this.currentState)
		{
		case this.states.OFF:
			if (this.isPlaying())
			{
				this.currentMusic.fade(-1, 0.0, 3.0);
				this.currentMusic = null;
			}
			break;

		case this.states.MENU:
			this.switchMusic(this.getRandomTrack(this.tracks.MENU), 0.0, true);
			break;

		case this.states.PEACE:
			this.switchMusic(this.getRandomTrack(this.tracks.PEACE), 3.0, true);
			break;

		case this.states.BATTLE:
			this.switchMusic(this.getRandomTrack(this.tracks.BATTLE), 2.0, true);
			break;

		case this.states.VICTORY:
			this.switchMusic(this.getRandomTrack(this.tracks.VICTORY), 2.0, true);
			break;

		case this.states.DEFEAT:
			this.switchMusic(this.getRandomTrack(this.tracks.DEFEAT), 2.0, true);
			break;

		case this.states.DEFEAT_CUE:
			this.switchMusic(this.getRandomTrack(this.tracks.DEFEAT_CUE), 2.0, false);
			this.setDelay(this.states.DEFEAT, 7000);
			break;

		default:
			warn("Music.updateState(): Unknown music state: " + this.currentState);
			break;
		}
	}
};

Music.prototype.storeTracks = function(civMusic)
{
	for each (var music in civMusic)
	{
		var type = undefined;
		for (var i in this.MUSIC)
		{
			if (music.Type == this.MUSIC[i])
			{
				type = i;
				break;
			}
		}

		if (type === undefined)
		{
			warn("Music.storeTracks(): Unrecognized music type: " + music.Type);
			continue;
		}

		this.tracks[type].push(music.File);
	}
};

Music.prototype.getRandomTrack = function(tracks)
{
	return tracks[getRandom(0, tracks.length-1)];
};

Music.prototype.switchMusic = function(track, fadeInPeriod, isLooping)
{
	if (this.currentMusic)
	{
		this.currentMusic.fade(-1, 0.0, 5.0);
		this.currentMusic = null;
	}

	this.currentMusic = new Sound(this.RELATIVE_MUSIC_PATH + track);

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

Music.prototype.isPlaying = function()
{
	if (!this.currentMusic)
		return false;

	// should return whether there is a valid handle; gain and fade do this also
	// However, if looping is not set, then it always returns false because the
	// handle is immediately cleared out
//	return this.currentMusic.isPlaying();
	return true;
};

Music.prototype.start = function()
{
	this.setState(this.states.PEACE);
};

Music.prototype.stop = function()
{
	this.setState(this.states.OFF);
};

// =============================================================================
// This allows for delays between tracks
// =============================================================================
Music.prototype.setDelay = function(state, delay)
{
	this.timer = [this.time + delay, state];
};

Music.prototype.stopTimer = function()
{
	this.timer = null;
};

// Needs to be called in onTick() to work
Music.prototype.updateTimer = function()
{
	this.time = Date.now();

	if (this.timer && (this.timer[0] <= this.time))
	{
		// Setting to OFF first guarantees that a state
		// change will take place even if the current
		// state is the same as the new state
		this.reference.setState(this.states.OFF);
		this.reference.setState(this.timer[1]);
		this.stopTimer();
	}
};