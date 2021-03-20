Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("Timer.js");

Engine.RegisterInterface("Test");

var cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");

var fired = [];

AddMock(10, IID_Test, {
	"Callback": function(data, lateness) {
		fired.push([data, lateness]);
	}
});

var cancelId;
AddMock(20, IID_Test, {
	"Callback": function(data, lateness) {
		fired.push([data, lateness]);
		cmpTimer.CancelTimer(cancelId);
	}
});

TS_ASSERT_EQUALS(cmpTimer.GetTime(), 0);

cmpTimer.OnUpdate({ "turnLength": 1/3 });

TS_ASSERT_EQUALS(cmpTimer.GetTime(), 333);

cmpTimer.SetTimeout(10, IID_Test, "Callback", 1000, "a");
cmpTimer.SetTimeout(10, IID_Test, "Callback", 1200, "b");

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, []);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["a", 0]]);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["a", 0], ["b", 300]]);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["a", 0], ["b", 300]]);

fired = [];

var c = cmpTimer.SetTimeout(10, IID_Test, "Callback", 1000, "c");
var d = cmpTimer.SetTimeout(10, IID_Test, "Callback", 1000, "d");
var e = cmpTimer.SetTimeout(10, IID_Test, "Callback", 1000, "e");
cmpTimer.CancelTimer(d);
cmpTimer.OnUpdate({ "turnLength": 1.0 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["c", 0], ["e", 0]]);

fired = [];

var r = cmpTimer.SetInterval(10, IID_Test, "Callback", 500, 1000, "r");

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["r", 0]]);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["r", 0]]);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["r", 0], ["r", 0]]);

cmpTimer.OnUpdate({ "turnLength": 3.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["r", 0], ["r", 0], ["r", 2500], ["r", 1500], ["r", 500]]);

cmpTimer.CancelTimer(r);

cmpTimer.OnUpdate({ "turnLength": 3.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["r", 0], ["r", 0], ["r", 2500], ["r", 1500], ["r", 500]]);

fired = [];

cancelId = cmpTimer.SetInterval(20, IID_Test, "Callback", 500, 1000, "s");

cmpTimer.OnUpdate({ "turnLength": 3.0 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["s", 2500]]);

fired = [];
let f = cmpTimer.SetInterval(10, IID_Test, "Callback", 1000, 1000, "f");

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["f", 0]]);

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["f", 0], ["f", 0]]);

cmpTimer.UpdateRepeatTime(f, 500);
cmpTimer.OnUpdate({ "turnLength": 1.5 });
// Interval updated at next updated, so expecting latency here.
TS_ASSERT_UNEVAL_EQUALS(fired, [["f", 0], ["f", 0], ["f", 500], ["f", 0]]);

cmpTimer.OnUpdate({ "turnLength": 0.5 });
TS_ASSERT_UNEVAL_EQUALS(fired, [["f", 0], ["f", 0], ["f", 500], ["f", 0], ["f", 0]]);
