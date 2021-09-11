Resources = {
	"BuildChoicesSchema": () => {
		let schema = "";
		for (let res of ["food", "metal"])
		{
			for (let subtype in ["meat", "grain"])
				schema += "<value>" + res + "." + subtype + "</value>";
			schema += "<value> treasure." + res + "</value>";
		}
		return "<choice>" + schema + "</choice>";
	}
};

Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("ResourceSupply.js");
Engine.LoadComponentScript("Timer.js");

let entity = 60;

AddMock(entity, IID_Fogging, {
	"Activate": () => {}
});

let template = {
	"Max": "1001",
	"Initial": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"MaxGatherers": "2"
};

let cmpResourceSupply = ConstructComponent(entity, "ResourceSupply", template);
cmpResourceSupply.OnOwnershipChanged({ "to": 1 });

TS_ASSERT(!cmpResourceSupply.IsInfinite());

TS_ASSERT(!cmpResourceSupply.GetKillBeforeGather());

TS_ASSERT_EQUALS(cmpResourceSupply.GetMaxAmount(), 1001);

TS_ASSERT_EQUALS(cmpResourceSupply.GetMaxGatherers(), 2);

TS_ASSERT_EQUALS(cmpResourceSupply.GetDiminishingReturns(), null);

TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 0);

TS_ASSERT(cmpResourceSupply.IsAvailableTo(70));
TS_ASSERT(cmpResourceSupply.AddGatherer(70));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT(cmpResourceSupply.AddGatherer(71));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

TS_ASSERT(!cmpResourceSupply.AddGatherer(72));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

TS_ASSERT(cmpResourceSupply.IsAvailableTo(70));
TS_ASSERT(!cmpResourceSupply.IsAvailableTo(73));
TS_ASSERT(!cmpResourceSupply.AddGatherer(73));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

cmpResourceSupply.RemoveGatherer(70);
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

cmpResourceSupply.RemoveGatherer(70);
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 2);

cmpResourceSupply.RemoveGatherer(70);
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 1);

TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.TakeResources(300), { "amount": 300, "exhausted": false });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 700);
TS_ASSERT(cmpResourceSupply.IsAvailableTo(70));

TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.TakeResources(800), { "amount": 700, "exhausted": true });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 0);
// The resource is not available when exhausted
TS_ASSERT(!cmpResourceSupply.IsAvailableTo(70));

cmpResourceSupply.RemoveGatherer(71);
TS_ASSERT_EQUALS(cmpResourceSupply.GetNumGatherers(), 0);

// #6317
const infiniteTemplate = {
	"Max": "Infinity",
	"Type": "food.grain",
	"KillBeforeGather": "false",
	"MaxGatherers": "1"
};

const cmpInfiniteResourceSupply = ConstructComponent(entity, "ResourceSupply", infiniteTemplate);
cmpResourceSupply.OnOwnershipChanged({ "to": 1 });

TS_ASSERT(cmpInfiniteResourceSupply.IsInfinite());
TS_ASSERT_UNEVAL_EQUALS(cmpInfiniteResourceSupply.TakeResources(300), { "amount": 300, "exhausted": false });

cmpInfiniteResourceSupply.OnEntityRenamed({ "newentity": entity });
TS_ASSERT(cmpInfiniteResourceSupply.IsAvailable());


// Test Changes.

let cmpTimer;
function reset(newTemplate)
{
	cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");
	cmpResourceSupply = ConstructComponent(entity, "ResourceSupply", newTemplate);
	cmpResourceSupply.OnOwnershipChanged({ "to": 1 });
}

// Decay.
template = {
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 999);
cmpTimer.OnUpdate({ "turnLength": 5 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 994);

// Decay with minimum.
template = {
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"LowerLimit": "997"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
cmpTimer.OnUpdate({ "turnLength": 3 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 997);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);

// Decay with maximum.
template = {
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"UpperLimit": "995"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);

// Decay with minimum and maximum.
template = {
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"UpperLimit": "995",
			"LowerLimit": "990"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
cmpResourceSupply.TakeResources(6);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 994);
cmpTimer.OnUpdate({ "turnLength": 10 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 989);

// Growth.
template = {
	"Initial": "995",
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);
cmpTimer.OnUpdate({ "turnLength": 5 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);

// Growth with minimum.
template = {
	"Initial": "995",
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"LowerLimit": "997"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);

// Growth with maximum.
template = {
	"Initial": "994",
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"UpperLimit": 995
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 994);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);

// Growth with minimum and maximum.
template = {
	"Initial": "990",
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"UpperLimit": "995",
			"LowerLimit": "990"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 990);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 991);
cmpTimer.OnUpdate({ "turnLength": 8 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);

// Growth when resources are taken again.
template = {
	"Initial": "995",
	"Max": "1000",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 996);
cmpResourceSupply.TakeResources(6);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 990);
cmpTimer.OnUpdate({ "turnLength": 5 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 995);

// Decay when dead.
template = {
	"Max": "10",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"State": "dead"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);

// No growth when dead.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "alive"
		}
	},
	"MaxGatherers": "2"
};
reset(template);

TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);

// Decay when dead or alive.
template = {
	"Max": "10",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"State": "dead alive"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);

AddMock(entity, IID_Health, {});  // Bring the entity to life.

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 8);

// No decay when alive.
template = {
	"Max": "10",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"State": "dead"
		}
	},
	"MaxGatherers": "2"
};
reset(template);

TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);

// Growth when alive.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "alive"
		}
	},
	"MaxGatherers": "2"
};
reset(template);

TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);

// Growth when dead or alive.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "dead alive"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);

DeleteMock(entity, IID_Health); // "Kill" the entity.

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);

// Decay *and* growth.
template = {
	"Max": "10",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000"
		},
		"Growth": {
			"Value": "1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);

// Decay *and* growth with different health states.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Rotting": {
			"Value": "-1",
			"Interval": "1000",
			"State": "dead"
		},
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "alive"
		}
	},
	"MaxGatherers": "2"
};
AddMock(entity, IID_Health, { });  // Bring the entity to life.
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);
DeleteMock(entity, IID_Health); // "Kill" the entity.
// We overshoot one due to lateness.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);

cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);

// Two effects with different limits.
template = {
	"Max": "20",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"SuperGrowth": {
			"Value": "2",
			"Interval": "1000",
			"UpperLimit": "8"
		},
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"UpperLimit": "12"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 8);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 11);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 12);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 13);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 13);

// Two effects with different limits.
// This in an interesting case, where the order of the changes matters.
template = {
	"Max": "20",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"UpperLimit": "12"
		},
		"SuperGrowth": {
			"Value": "2",
			"Interval": "1000",
			"UpperLimit": "8"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 8);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);
cmpTimer.OnUpdate({ "turnLength": 5 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 13);

// Infinity with growth.
template = {
	"Max": "Infinity",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), Infinity);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), Infinity);

// Infinity with decay.
template = {
	"Max": "Infinity",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Decay": {
			"Value": "-1",
			"Interval": "1000"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), Infinity);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), Infinity);

// Decay when not gathered.
template = {
	"Max": "10",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Decay": {
			"Value": "-1",
			"Interval": "1000",
			"State": "notGathered"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 10);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);
TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);
cmpResourceSupply.RemoveGatherer(70);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 8);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);

// Grow when gathered.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "gathered"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);
cmpResourceSupply.RemoveGatherer(70, 1);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);

// Grow when gathered or not.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "notGathered gathered"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);
TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 8);
cmpResourceSupply.RemoveGatherer(70);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 9);

// Grow when gathered and alive.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Growth": {
			"Value": "1",
			"Interval": "1000",
			"State": "alive gathered"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
AddMock(entity, IID_Health, { });  // Bring the entity to life.
cmpResourceSupply.CheckTimers(); // No other way to tell we've come to life.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 6);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);
cmpResourceSupply.RemoveGatherer(70);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);
DeleteMock(entity, IID_Health); // "Kill" the entity.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 7);

// Decay when dead and not gathered.
template = {
	"Max": "10",
	"Initial": "5",
	"Type": "food.meat",
	"KillBeforeGather": "false",
	"Change": {
		"Decay": {
			"Value": "-1",
			"Interval": "1000",
			"State": "dead notGathered"
		}
	},
	"MaxGatherers": "2"
};
reset(template);
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 5);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 4);
TS_ASSERT(cmpResourceSupply.AddActiveGatherer(70));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 4);
AddMock(entity, IID_Health, {});  // Bring the entity to life.
cmpResourceSupply.CheckTimers(); // No other way to tell we've come to life.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 4);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 4);
cmpResourceSupply.RemoveGatherer(70);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 4);
DeleteMock(entity, IID_Health); // "Kill" the entity.
cmpResourceSupply.CheckTimers(); // No other way to tell we've died.
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 3);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpResourceSupply.GetCurrentAmount(), 2);
