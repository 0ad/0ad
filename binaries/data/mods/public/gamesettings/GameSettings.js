/**
 * Data store for game settings.
 *
 * This is intended as a helper to create the settings object for a game.
 * This object is referred to as:
 *  - g_InitAttributes in the GUI session context
 *  - InitAttributes in the JS simulation context
 *  - Either InitAttributes or MapSettings in the C++ simulation.
 * Settings can depend on each other, and the map provides many.
 * This class's job is thus to provide a simpler interface around that.
 */
class GameSettings
{
	init(mapCache)
	{
		if (!mapCache)
			mapCache = new MapCache();
		Object.defineProperty(this, "mapCache", {
			"value": mapCache,
		});

		// Load all possible civ data - don't presume that some will be available.
		Object.defineProperty(this, "civData", {
			"value": loadCivData(false, false),
		});

		Object.defineProperty(this, "isNetworked", {
			"value": Engine.HasNetClient(),
		});

		// Load attributes as regular enumerable (i.e. iterable) properties.
		for (let comp in GameSettings.prototype.Attributes)
		{
			let name = comp[0].toLowerCase() + comp.substr(1);
			if (name in this)
				error("Game Settings attribute '" + name + "' is already used.");
			this[name] = new GameSettings.prototype.Attributes[comp](this);
		}
		for (let comp in this)
			if (this[comp].init)
				this[comp].init();

		return this;
	}

	/**
	 * 'Serialize' the settings into the InitAttributes format,
	 * which can then be saved as JSON.
	 * Used to set the InitAttributes, for network synching, for hotloading & for persistence.
	 * TODO: it would probably be better to have different paths for at least a few of these.
	 */
	toInitAttributes()
	{
		let attribs = {
			"settings": {}
		};
		for (let comp in this)
			if (this[comp].toInitAttributes)
				this[comp].toInitAttributes(attribs);

		return attribs;
	}

	/**
	 * Deserialize from a the InitAttributes format (i.e. parsed JSON).
	 * TODO: this could/should maybe support partial deserialization,
	 * which means MP might actually send only the bits that change.
	 */
	fromInitAttributes(attribs)
	{
		// Settings may depend on eachother. Some selections of settings
		// might be illegal. So keep looping through all settings until
		// we find something stable.
		const components = Object.keys(this);

		// To check in the loop below if something change we just compare
		// the entire component. However, we must ignore the "settings"
		// keyword to avoid cyclic objects.
		const getComponentData = comp => Object.keys(this[comp]).map(key => key == "settings" ? undefined : this[comp][key]);

		// When we have looped components.length + 1 times, we are considered stuck.
		for (let i = 0; i <= components.length; ++i)
		{
			// Re-init if any setting was changed, to make sure dependencies are cleared.
			let reInit = false;
			for (const comp in this)
			{
				const oldSettings = clone(getComponentData(comp));
				if (this[comp].fromInitAttributes)
					this[comp].fromInitAttributes(attribs);
				reInit = reInit || !deepCompare(oldSettings, getComponentData(comp));
			}
			if (!reInit)
				return;
		}

		throw new Error("Infinite loop initializing attributes detected, components: " + uneval(components));
	}

	/**
	 * Change "random" settings into their proper settings.
	 */
	pickRandomItems()
	{
		const components = Object.keys(this);

		// When we have looped components.length + 1 times, we are considered stuck.
		for (let i = 0; i <= components.length; ++i)
		{
			// Re-pick if any random setting was unrandomised, to make sure dependencies are cleared.
			let rePick = false;
			for (const comp in this)
				if (this[comp].pickRandomItems)
					rePick = this[comp].pickRandomItems() || rePick;
			if (!rePick)
				return;
		}

		throw new Error("Infinite loop picking random items detected, components: " + uneval(components));
	}

	/**
	 * Start the game & switch to the loading page.
	 * This is here because there's limited value in having a separate folder/file for it,
	 * since you'll need a GameSettings object anyways.
	 * @param playerAssignments - A dict of 'local'/GUID per player and their name/slot.
	 */
	launchGame(playerAssignments, storeReplay)
	{
		this.pickRandomItems();

		// Let the settings finalize themselves. Let them do anything they need to do before the
		// game starts and set any value in the attributes which mustn't be persisted.
		const attribs = this.toInitAttributes();
		for (const comp in this)
			if (this[comp].onFinalizeAttributes)
				this[comp].onFinalizeAttributes(attribs, playerAssignments);

		Object.defineProperty(this, "finalizedAttributes", {
			"value": deepfreeze(attribs)
		});

		// NB: for multiplayer support, the clients must be listening to "start" net messages.
		if (this.isNetworked)
			if (g_isSaveLoaded && g_savedGameId !== undefined)
				Engine.StartNetworkSavedGame(g_savedGameId, this.finalizedAttributes, storeReplay);
			else
				Engine.StartNetworkGame(this.finalizedAttributes, storeReplay);
		else
			Engine.StartGame(this.finalizedAttributes, playerAssignments.local.player, storeReplay);
	}
}

Object.defineProperty(GameSettings.prototype, "Attributes", {
	"value": {},
	"enumerable": false,
	"writable": true,
});
