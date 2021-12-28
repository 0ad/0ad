Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/Researcher.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Trainer.js");
Engine.LoadComponentScript("interfaces/Upgrade.js");
Engine.LoadComponentScript("Timer.js");

Engine.LoadComponentScript("ProductionQueue.js");

const playerEnt = 2;
const playerID = 1;
const testEntity = 3;

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"CancelTimer": (id) => {},
	"SetInterval": (ent, iid, func) => 1
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => playerEnt
});

AddMock(playerEnt, IID_Player, {
	"GetPlayerID": () => playerID
});

AddMock(testEntity, IID_Ownership, {
	"GetOwner": () => playerID
});

AddMock(testEntity, IID_Trainer, {
	"GetBatch": (id) => ({}),
	"HasBatch": (id) => false, // Assume we've finished.
	"Progress": (time) => time,
	"QueueBatch": () => 1,
	"StopBatch": (id) => {}
});

const cmpProdQueue = ConstructComponent(testEntity, "ProductionQueue", null);


// Test autoqueue.
cmpProdQueue.EnableAutoQueue();

cmpProdQueue.AddItem("some_template", "unit", 3);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
cmpProdQueue.ProgressTimeout(null, 0);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);

cmpProdQueue.RemoveItem(cmpProdQueue.nextID -1);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);

cmpProdQueue.DisableAutoQueue();


// Test items which don't use all the time.
AddMock(testEntity, IID_Trainer, {
	"GetBatch": (id) => ({}),
	"HasBatch": (id) => false, // Assume we've finished.
	"PauseBatch": (id) => {},
	"Progress": (time) => time - 250,
	"QueueBatch": () => 1,
	"StopBatch": (id) => {},
	"UnpauseBatch": (id) => {}
});

cmpProdQueue.AddItem("some_template", "unit", 2);
cmpProdQueue.AddItem("some_template", "unit", 3);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 2);
cmpProdQueue.ProgressTimeout(null, 0);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);


// Test pushing an item to the front.
cmpProdQueue.AddItem("some_template", "unit", 2);
cmpProdQueue.AddItem("some_template", "unit", 3, null, true);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 2);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue()[0].id, cmpProdQueue.nextID - 1);
TS_ASSERT(cmpProdQueue.GetQueue()[1].paused);

cmpProdQueue.ProgressTimeout(null, 0);
TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);


// Simple deserialisation test.
cmpProdQueue.AddItem("some_template", "unit", 2);
const deserialisedCmp = SerializationCycle(cmpProdQueue);
TS_ASSERT_EQUALS(deserialisedCmp.GetQueue().length, 1);
deserialisedCmp.ProgressTimeout(null, 0);
TS_ASSERT_EQUALS(deserialisedCmp.GetQueue().length, 0);
