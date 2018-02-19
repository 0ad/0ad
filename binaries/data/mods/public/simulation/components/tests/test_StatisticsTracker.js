Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("StatisticsTracker.js");

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetInterval": () => true
});

Resources = {
	"GetCodes": () => ["food", "metal", "stone", "wood"]
};

let cmpStatisticsTracker = ConstructComponent(SYSTEM_ENTITY, "StatisticsTracker");
let obj1 = {
	"successfulBribes": 3,
	"unitsTrained": {
		"Infantry": 5,
		"Worker": 7
	}
};
let obj2 = {
	"successfulBribes": [11, 13, 17],
	"unitsTrained": {
		"Infantry": [19, 23],
		"Worker": 29
	}
};

cmpStatisticsTracker.PushValue(obj1, obj2);
TS_ASSERT_UNEVAL_EQUALS(obj2, {
	"successfulBribes": [11, 13, 17, 3],
	"unitsTrained": {
		"Infantry": [19, 23, 5],
		"Worker": [7]
	}
});
