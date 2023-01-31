Engine.LoadComponentScript("interfaces/PlayerManager.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Requirements.js");

const playerID = 1;
const playerEnt = 11;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": () => playerEnt
});

// First test no requirements.
let template = {
};

const met = () => TS_ASSERT(RequirementsHelper.AreRequirementsMet(template, playerID));
const notMet = () => TS_ASSERT(!RequirementsHelper.AreRequirementsMet(template, playerID));

met();

// Simple requirements are assumed to be additive.
template = {
	"Techs": { "_string": "phase_city" }
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => false
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city"
});
met();

template = {
	"Techs": { "_string": "cartography phase_city" }
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => false
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "cartography" || tech === "phase_town"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "cartography" || tech === "phase_city"
});
met();


// Additive requirements (all should to be met).
// Entity requirements.
template = {
	"All": {
		"Entities": {
			"class_1": {
				"Count": 1
			}
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {},
	"typeCountsByClass": {}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 0
	},
	"typeCountsByClass": {}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_2": 1
	},
	"typeCountsByClass": {}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 1
	},
	"typeCountsByClass": {}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 1
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1
		}
	}
});
met();


template = {
	"All": {
		"Entities": {
			"class_1": {
				"Variants": 2
			}
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1,
			"template_2": 1
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 1
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1
		}
	}
});
notMet();

template = {
	"All": {
		"Entities": {
			"class_1": {
				"Count": 1,
				"Variants": 2
			}
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 1
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1,
			"template_2": 1
		}
	}
});
met();

template = {
	"All": {
		"Entities": {
			"class_1": {
				"Count": 3,
				"Variants": 2
			}
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1,
			"template_2": 1
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2,
			"template_2": 1
		}
	}
});
met();


// Technology requirements.
template = {
	"All": {
		"Techs": { "_string": "phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => false
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town"
});
met();

template = {
	"All": {
		"Techs": { "_string": "phase_city" }
	}
};
notMet();

template = {
	"All": {
		"Techs": { "_string": "phase_town phase_city" }
	}
};
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city"
});
met();

template = {
	"All": {
		"Techs": { "_string": "!phase_city" }
	}
};
notMet();

template = {
	"All": {
		"Techs": { "_string": "!phase_town phase_city"}
	}
};
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_city"
});
met();


// Combination of Entity and Technology requirements.
template = {
	"All": {
		"Entities": {
			"class_1": {
				"Count": 3,
				"Variants": 2
			}
		},
		"Techs": { "_string": "phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => false,
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2,
			"template_2": 1
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2,
			"template_2": 1
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2,
			"template_2": 1
		}
	}
});
notMet();


// Choice requirements (at least one needs to be met).
// Entity requirements.
template = {
	"Any": {
		"Entities": {
			"class_1": {
				"Count": 1,
			}
		},
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 0
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 1
	}
});
met();

template = {
	"Any": {
		"Entities": {
			"class_1": {
				"Count": 5,
				"Variants": 2
			}
		},
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 3,
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2,
			"template_2": 1
		}
	}
});
met();


// Technology requirements.
template = {
	"Any": {
		"Techs": { "_string": "phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_city"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town"
});
met();

template = {
	"Any": {
		"Techs": { "_string": "phase_town phase_city" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_city"
});
met();

template = {
	"Any": {
		"Techs": { "_string": "!phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town"
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_city"
});
met();

template = {
	"Any": {
		"Techs": { "_string": "!phase_town phase_city" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city"
});
met();


// Combinational requirements of entities and technologies.
template = {
	"Any": {
		"Entities": {
			"class_1": {
				"Count": 3,
				"Variants": 2
			}
		},
		"Techs": { "_string": "!phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 3
		}
	}
});
met();


// Nested requirements.
template = {
	"All": {
		"All": {
			"Techs": { "_string": "!phase_town" }
		},
		"Any": {
			"Entities": {
				"class_1": {
					"Count": 3,
					"Variants": 2
				}
			},
			"Techs": { "_string": "phase_city" }
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 3
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 3
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => false,
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1,
			"template_2": 1
		}
	}
});
met();


template = {
	"Any": {
		"All": {
			"Techs": { "_string": "!phase_town" }
		},
		"Any": {
			"Entities": {
				"class_1": {
					"Count": 3,
					"Variants": 2
				}
			},
			"Techs": { "_string": "phase_city" }
		}
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 3
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town" || tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 3
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => false,
	"typeCountsByClass": {
		"class_1": {
			"template_1": 1,
			"template_2": 1
		}
	}
});
met();


// Two levels deep nested.
template = {
	"All": {
		"Any": {
			"All": {
				"Techs": { "_string": "cartography phase_imperial" }
			},
			"Entities": {
				"class_1": {
					"Count": 3,
					"Variants": 2
				}
			},
			"Techs": { "_string": "phase_city" }
		},
		"Techs": { "_string": "!phase_town" }
	}
};

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_town",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_city",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
met();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "phase_imperial",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
notMet();

AddMock(playerEnt, IID_TechnologyManager, {
	"classCounts": {
		"class_1": 2
	},
	"IsTechnologyResearched": (tech) => tech === "cartography" || tech === "phase_imperial",
	"typeCountsByClass": {
		"class_1": {
			"template_1": 2
		}
	}
});
met();
