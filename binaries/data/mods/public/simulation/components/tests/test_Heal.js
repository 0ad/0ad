Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Heal.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Loot.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Heal.js");

const entity= 60;

let template = {
	"Range": 20,
	"RangeOverlay" : {
		"LineTexture": "heal_overlay_range.png",
		"LineTextureMask": "heal_overlay_range_mask.png",
		"LineThickness": 0.35
	},
	"HP": 5,
	"Rate": 2000,
	"UnhealableClasses": { "_string": "Cavalry" },
	"HealableClasses": { "_string": "Support Infantry" },
};

ApplyValueModificationsToEntity = function(value, stat, ent)
{
	if (ent != entity)
		return stat;
	switch (value)
	{
	case "Heal/HP":
		return stat + 100;
	case "Heal/Rate":
		return stat + 200;
	case "Heal/Range":
		return stat + 300;
	default:
		return stat;
	}
};

let cmpHeal = ConstructComponent(60, "Heal", template);

// Test Getters
TS_ASSERT_EQUALS(cmpHeal.GetRate(), 2000 + 200);

TS_ASSERT_UNEVAL_EQUALS(cmpHeal.GetTimers(), { "prepare": 1000, "repeat": 2000 + 200 });

TS_ASSERT_EQUALS(cmpHeal.GetHP(), 5 + 100);

TS_ASSERT_UNEVAL_EQUALS(cmpHeal.GetRange(), {"min":0, "max": 20 + 300 });

TS_ASSERT_EQUALS(cmpHeal.GetHealableClasses(), "Support Infantry");

TS_ASSERT_EQUALS(cmpHeal.GetUnhealableClasses(), "Cavalry");

TS_ASSERT_UNEVAL_EQUALS(cmpHeal.GetRangeOverlays(), [{
	"radius": 20 + 300,
	"texture": "heal_overlay_range.png",
	"textureMask": "heal_overlay_range_mask.png",
	"thickness": 0.35
}]);

// Test PerformHeal
let target = 70;

let increased;
AddMock(target, IID_Health, {
	"GetMaxHitpoints": () => 700,
	"Increase": amount => {
		increased = true;
		TS_ASSERT_EQUALS(amount, 5 + 100);
		return { "old": 600, "new": 600 + 5 + 100 };
	}
});

cmpHeal.PerformHeal(target);
TS_ASSERT(increased);

let looted;
AddMock(target, IID_Loot, {
	"GetXp": () => {
		 looted = true; return 80;
	}
});

let promoted;
AddMock(entity, IID_Promotion, {
	"IncreaseXp": amount => {
		promoted = true;
		TS_ASSERT_EQUALS(amount, (5 + 100) * 80 / 700);
	}
});

increased = false;
cmpHeal.PerformHeal(target);
TS_ASSERT(increased && looted && promoted);

// Test OnValueModification
let updated;
AddMock(entity, IID_UnitAI, {
	"UpdateRangeQueries": () => {
		updated = true;
	}
});

cmpHeal.OnValueModification({ "component": "Heal", "valueNames": ["Heal/HP"] });
TS_ASSERT(!updated);

cmpHeal.OnValueModification({ "component": "Heal", "valueNames": ["Heal/Range"] });
TS_ASSERT(updated);
