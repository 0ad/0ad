class LoadGameButton
{
    constructor(setupWindow)
    {
        this.setupWindow = setupWindow;
        this.buttonHiddenChangeHandlers = new Set();

        this.loadGameButton = Engine.GetGUIObjectByName("loadGameButton");
        this.loadGameButton.onPress = this.onPress.bind(this);
        this.updateCaption();

        setupWindow.registerLoadHandler(this.onLoad.bind(this));
    }

    registerButtonHiddenChangeHandler(handler)
    {
        this.buttonHiddenChangeHandlers.add(handler);
    }

    onLoad()
    {
        // Only give that option to the player hosting the game
        this.loadGameButton.hidden = !g_IsController;

        for (let handler of this.buttonHiddenChangeHandlers)
            handler();
    }

    updateCaption()
    {
        // Update the caption and tooltip of the button
        let status = g_isSaveLoaded ? "clear" : "load";

        this.loadGameButton.caption = this.Caption[status];
        this.loadGameButton.tooltip = this.Tooltip[status];
    }

    onPress()
    {
        // Load or clear a previously loaded save
        if (g_isSaveLoaded)
            this.onPressClearSave();
        else
            this.onPressLoadSave();   
    }

    onPressClearSave()
    {
        // Pass the global g_isSaveLoaded variable to false, clear the saved
        // game ID and unlock the settings
        g_isSaveLoaded = false;
        g_savedGameId = undefined;

        g_GameSettings.pickRandomItems();
        this.setupWindow.controls.gameSettingsController.setNetworkInitAttributes();
        this.updateCaption();

        // TODO @mbusy: to be removed, here to check the game launching settigns
        print("###########\n");
        print(JSON.stringify(this.setupWindow.controls.gameSettingsController.getSettings().settings.PlayerData, null, 2) + "\n");
        print("###########\n");
    }

    onPressLoadSave()
    {
        Engine.PushGuiPage(
            "page_loadgame.xml",
            {},
            this.parseGameData.bind(this));
    }

    parseGameData(data)
    {
        // If no data is being provided, for instance if the cancel button is
        // pressed
        if (typeof data === 'undefined')
            return;
 

        // WARNING: This line removes the null entry at index 0 of player data,
        // accounting for Gaïa. TODO @mbusy, investigate how this works and if 
        // this can be handled more gracefully
        data.metadata.initAttributes.settings.PlayerData.splice(0, 1);

        // Update the data depending on if extra human players are present. If
        // the loaded data contains AI but more humans are present, the AIs are
        // removed and replaced by the human players.

        //TODO: in practice removing the AI relative values in PlayerData is
        // not enough, if the player was originally a petra bot, both the
        // player and the petra bot will send commands
        let minPlayerNumber = Math.min(
            data.metadata.initAttributes.settings.PlayerData.length,
            g_GameSettings.toInitAttributes().settings.PlayerData.length)

        for (let i = 0; i < minPlayerNumber; ++i)
        {
            let currentIndexAI = g_GameSettings.toInitAttributes().settings.PlayerData[i]["AI"];

            if (data.metadata.initAttributes.settings.PlayerData[i]["AI"] !== currentIndexAI)
            {
                data.metadata.initAttributes.settings.PlayerData[i]["AI"] = currentIndexAI;

                if (currentIndexAI === false && data.metadata.initAttributes.settings.PlayerData[i].hasOwnProperty("AIBehavior"))
                    delete data.metadata.initAttributes.settings.PlayerData[i]["AIBehavior"]
                if (currentIndexAI === false && data.metadata.initAttributes.settings.PlayerData[i].hasOwnProperty("AIDiff"))
                    delete data.metadata.initAttributes.settings.PlayerData[i]["AIDiff"]
            }
        }
        
        // Pass the global g_isSaveLoaded variable to true, set the
        // g_savedGameId global variable and update the settings
        g_isSaveLoaded = true;
        g_savedGameId = data.gameId;

        g_GameSettings.fromInitAttributes(data.metadata.initAttributes);
        this.setupWindow.controls.gameSettingsController.setNetworkInitAttributes();
        this.updateCaption();
    }


}

LoadGameButton.prototype.Caption = 
{
    "load" : translate("Load Game"),
    "clear" : translate("Clear Save")
};

// TODO @mbusy: needs translation
LoadGameButton.prototype.Tooltip = 
{
    "load" : translate("Load a previously created game. You will still have to press start after having loaded the game data"),
    "clear" : translate("Clear the loaded saved data, allowing to update the setting once again and start a new game from scratch")
};