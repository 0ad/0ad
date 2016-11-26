Engine.LoadComponentScript("interfaces/EndGameManager.js");
Engine.LoadComponentScript("EndGameManager.js");

let cmpEndGameManager = ConstructComponent(SYSTEM_ENTITY, "EndGameManager");

let playerEnt1 = 1;
let wonderDuration = 2 * 60 * 1000;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetNumPlayers": () => 4
});

AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
	"DeleteTimeNotification": () => null,
	"AddTimeNotification": () => 1
});

AddMock(playerEnt1, IID_Player, {
	"GetName": () => "Player 1",
	"GetState": () => "active",
});

TS_ASSERT_EQUALS(cmpEndGameManager.skipAlliedVictoryCheck, true);
cmpEndGameManager.SetAlliedVictory(true);
TS_ASSERT_EQUALS(cmpEndGameManager.GetAlliedVictory(), true);
cmpEndGameManager.SetGameType("wonder", { "wonderDuration": wonderDuration });
TS_ASSERT_EQUALS(cmpEndGameManager.skipAlliedVictoryCheck, false);
TS_ASSERT(cmpEndGameManager.GetGameType() == "wonder");
TS_ASSERT_EQUALS(cmpEndGameManager.GetGameTypeSettings().wonderDuration, wonderDuration);
