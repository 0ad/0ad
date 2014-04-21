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
		DEFEAT: "defeat"
	};

	this.resetTracks();

	this.states = {
		OFF : 0,
		MENU : 1,
		PEACE : 2,
		BATTLE : 3,
		VICTORY :4,
		DEFEAT : 5
	};

	this.musicGain = 0.3;

	this.currentState = 0;
	this.oldState = 0;

	// timer for delay between tracks
	this.timer = [];
	this.time = Date.now();
}

Music.prototype.resetTracks = function()
{
	this.tracks = {
		MENU: ["Honor_Bound.ogg"],
		PEACE: [],
		BATTLE: ["Taiko_1.ogg", "Taiko_2.ogg"],
		VICTORY : ["You_are_Victorious!.ogg"],
		DEFEAT : ["Dried_Tears.ogg"]
	};
};

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
			Engine.StopMusic();
			break;

		case this.states.MENU:
			this.switchMusic(this.getRandomTrack(this.tracks.MENU), 0.0, true);
			break;

		case this.states.PEACE:
			this.startPlayList(this.tracks.PEACE, 3.0, true);
			break;

		case this.states.BATTLE:
			this.startPlayList(this.tracks.BATTLE, 2.0, true);
			break;

		case this.states.VICTORY:
			this.startPlayList(this.tracks.VICTORY, 2.0, true);
			break;

		case this.states.DEFEAT:
			this.startPlayList(this.tracks.DEFEAT, 2.0, true);
			break;

		default:
			warn(sprintf("%(functionName)s: Unknown music state: %(state)s", { functionName: "Music.updateState()", state: this.currentState }));
			break;
		}
	}
};

Music.prototype.storeTracks = function(civMusic)
{
	this.resetTracks();
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
			warn(sprintf("%(functionName)s: Unrecognized music type: %(musicType)s", { functionName: "Music.storeTracks()", musicType: music.Type }));
			continue;
		}

		this.tracks[type].push(music.File);
	}
};

Music.prototype.getRandomTrack = function(tracks)
{
	return tracks[getRandom(0, tracks.length-1)];
};

Music.prototype.startPlayList = function(tracks, fadeInPeriod, isLooping)
{
	Engine.ClearPlaylist();
	for (var i in tracks)
	{
		Engine.AddPlaylistItem( this.RELATIVE_MUSIC_PATH + tracks[i] );
	}

	Engine.StartPlaylist(isLooping);
};

Music.prototype.switchMusic = function(track, fadeInPeriod, isLooping)
{
  Engine.ClearPlaylist();
	Engine.AddPlaylistItem( this.RELATIVE_MUSIC_PATH + track );
	Engine.StartPlaylist(isLooping);
};

Music.prototype.isPlaying = function()
{
	return Engine.MusicPlaying();
};

Music.prototype.start = function()
{
	Engine.StartMusic();
	this.setState(this.states.PEACE);
};

Music.prototype.stop = function()
{
	this.setState(this.states.OFF);
};

