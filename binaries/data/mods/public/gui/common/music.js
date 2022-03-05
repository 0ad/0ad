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
		"MENU": "menu",
		"PEACE": "peace",
		"BATTLE": "battle",
		"VICTORY": "victory",
		"DEFEAT": "defeat",
		"CUSTOM": "custom"
	};

	this.resetTracks();

	this.states = {
		"OFF": 0,
		"MENU": 1,
		"PEACE": 2,
		"BATTLE": 3,
		"VICTORY": 4,
		"DEFEAT": 5,
		"CUSTOM": 6
	};

	this.musicGain = 0.3;

	this.locked = false;
	this.currentState = 0;
	this.oldState = 0;

	// timer for delay between tracks
	this.timer = [];
	this.time = Date.now();
}

Music.prototype.DEFAULT_MENU_TRACKS = ["Honor_Bound.ogg"].concat(shuffleArray([
	"An_old_Warhorse_goes_to_Pasture.ogg",
	"Calm_Before_the_Storm.ogg",
	"Juno_Protect_You.ogg"
]));

Music.prototype.DEFAULT_PEACE_TRACKS = [
	"Tale_of_Warriors.ogg",
	"Tavern_in_the_Mist.ogg",
	"The_Road_Ahead.ogg"
];

Music.prototype.DEFAULT_BATTLE_TRACKS = [
	"Taiko_1.ogg",
	"Taiko_2.ogg"
];

Music.prototype.DEFAULT_VICTORY_TRACKS = [
	"You_are_Victorious!.ogg"
];

Music.prototype.DEFAULT_DEFEAT_TRACKS = [
	"Dried_Tears.ogg"
];

Music.prototype.DEFAULT_CUSTOM_TRACKS = [];

Music.prototype.completeTracks = function()
{
	for (const musicType of Object.keys(this.MUSIC))
		if (!this.tracks[musicType] || this.tracks[musicType].length === 0)
			this.tracks[musicType] = this["DEFAULT_" + musicType + "_TRACKS"];
};

Music.prototype.resetTracks = function()
{
	this.tracks = {};
	for (const musicType of Object.keys(this.MUSIC))
		this.tracks[musicType] = this["DEFAULT_" + musicType + "_TRACKS"];
};

// "reference" refers to this instance of Music (needed if called from the timer)
Music.prototype.setState = function(state)
{
	if (this.locked)
		return;

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
			this.startPlayList(this.tracks.MENU, 0, true);
			break;

		case this.states.PEACE:
			this.startPlayList(shuffleArray(this.tracks.PEACE), 3.0, true);
			break;

		case this.states.BATTLE:
			this.startPlayList(shuffleArray(this.tracks.BATTLE), 2.0, true);
			break;

		case this.states.VICTORY:
			this.startPlayList(shuffleArray(this.tracks.VICTORY), 2.0, true);
			break;

		case this.states.DEFEAT:
			this.startPlayList(shuffleArray(this.tracks.DEFEAT), 2.0, true);
			break;

		case this.states.CUSTOM:
			this.startPlayList(shuffleArray(this.tracks.CUSTOM), 2.0, true);
			break;

		default:
			warn(sprintf("%(functionName)s: Unknown music state: %(state)s", { "functionName": "Music.updateState()", "state": this.currentState }));
			break;
		}
	}
};

Music.prototype.storeTracks = function(civMusic)
{
	for (const musicType of Object.keys(this.MUSIC))
		this.tracks[musicType] = [];

	for (let music of civMusic)
	{
		let type;
		for (let i in this.MUSIC)
			if (music.Type == this.MUSIC[i])
			{
				type = i;
				break;
			}

		if (type === undefined)
		{
			warn(sprintf("%(functionName)s: Unrecognized music type: %(musicType)s", { "functionName": "Music.storeTracks()", "musicType": music.Type }));
			continue;
		}

		this.tracks[type].push(music.File);
	}

	this.completeTracks();
};

Music.prototype.startPlayList = function(tracks, fadeInPeriod, isLooping)
{
	Engine.ClearPlaylist();
	for (let i in tracks)
		Engine.AddPlaylistItem(this.RELATIVE_MUSIC_PATH + tracks[i]);

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
/**
* Play the custom playlist when locked, otherwise plays the civ music according to the battle state.
*/
Music.prototype.setLocked = function(locked)
{
	this.locked = locked;
};
