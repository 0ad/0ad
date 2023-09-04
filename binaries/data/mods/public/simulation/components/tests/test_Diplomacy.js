Engine.LoadComponentScript("interfaces/Diplomacy.js");
Engine.LoadComponentScript("interfaces/PlayerManager.js");
Engine.LoadComponentScript("Diplomacy.js");
Engine.LoadHelperScript("Player.js");

const players = [10, 11];

ConstructComponent(players[0], "Diplomacy", null)
const cmpDiplomacy = ConstructComponent(players[1], "Diplomacy", {})
TS_ASSERT_EQUALS(cmpDiplomacy.GetTeam(), -1);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetAllies(), []);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetEnemies(), []);

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetNumPlayers": () => players.length,
	"GetPlayerByID": (i) => players[i]
});

for (const player in players)
	AddMock(players[player], IID_Player, {
		"GetPlayerID": () => player,
		"IsActive": () => true,
	});

cmpDiplomacy.ChangeTeam(1);
TS_ASSERT_EQUALS(cmpDiplomacy.GetTeam(), 1);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetAllies(), [1]);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetEnemies(), []);

cmpDiplomacy.LockTeam();
cmpDiplomacy.ChangeTeam(2);
TS_ASSERT_EQUALS(cmpDiplomacy.GetTeam(), 1);

cmpDiplomacy.UnLockTeam();
cmpDiplomacy.SetEnemy(0);

TS_ASSERT(!cmpDiplomacy.IsAlly(0));
TS_ASSERT(!cmpDiplomacy.IsNeutral(0));
TS_ASSERT(cmpDiplomacy.IsEnemy(0));
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetAllies(), [1]);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetEnemies(), [0]);

cmpDiplomacy.Ally(0);

TS_ASSERT(cmpDiplomacy.IsAlly(0));
TS_ASSERT(!cmpDiplomacy.IsNeutral(0));
TS_ASSERT(!cmpDiplomacy.IsEnemy(0));
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetAllies(), [0, 1]);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetEnemies(), []);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetMutualAllies(), [1]);

cmpDiplomacy.SetNeutral(0);

TS_ASSERT(!cmpDiplomacy.IsAlly(0));
TS_ASSERT(cmpDiplomacy.IsNeutral(0));
TS_ASSERT(!cmpDiplomacy.IsEnemy(0));

// Mutual worsening of relations.
cmpDiplomacy.OnDiplomacyChanged({
	"player": 0,
	"otherPlayer": 1,
	"value": -1
});
TS_ASSERT(cmpDiplomacy.IsEnemy(0));


cmpDiplomacy.SetDiplomacy([-1, 1, 0, 1, -1]);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetAllies(), [1, 3]);
TS_ASSERT_UNEVAL_EQUALS(cmpDiplomacy.GetEnemies(), [0, 4]);

// Check diplomacy is not editable outside of the component.
var diplo = cmpDiplomacy.GetDiplomacy();
diplo[0] = 1;
TS_ASSERT(cmpDiplomacy.IsEnemy(0));

diplo = [1, 1, 0];
cmpDiplomacy.SetDiplomacy(diplo);
diplo[1] = -1;
TS_ASSERT(cmpDiplomacy.IsAlly(1));


// (De)serialisation preserves relations.
const deserialisedCmp = SerializationCycle(cmpDiplomacy);
TS_ASSERT(deserialisedCmp.IsAlly(1));
