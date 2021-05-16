Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Promotion.js");
Engine.LoadComponentScript("Timer.js");

let cmpPromotion;
const entity = 60;
let modifier = 0;

let ApplyValueModificationsToEntity = (_, val) => val + modifier;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToEntity);

let QueryOwnerInterface = () => ({ "GetPlayerID": () => 1 });
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);

let entTemplates = {
	"60": "template_b",
	"61": "template_f",
	"62": "end"
};

let promote = {
	"template_b": "template_c",
	"template_c": "template_d",
	"template_d": "template_e",
	"template_e": "template_f"
};

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": (t) => ({
		"Promotion": {
			"Entity": promote[t],
			"RequiredXp": 1000
		},
	}),
});

let ChangeEntityTemplate = function(ent, template)
{
	let newEnt = ent + 1;
	let cmpNewPromotion = ConstructComponent(newEnt, "Promotion", {
		"Entity": entTemplates[newEnt],
		"RequiredXp": 1000
	});
	cmpPromotion.SetPromotedEntity(newEnt);
	cmpNewPromotion.IncreaseXp(cmpPromotion.GetCurrentXp());
	cmpPromotion = cmpNewPromotion;
	return newEnt;
};
Engine.RegisterGlobal("ChangeEntityTemplate", ChangeEntityTemplate);

cmpPromotion = ConstructComponent(entity, "Promotion", {
	"Entity": "template_b",
	"RequiredXp": 1000
});

// Test getters/setters.
TS_ASSERT_EQUALS(cmpPromotion.GetRequiredXp(), 1000);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
TS_ASSERT_EQUALS(cmpPromotion.GetPromotedTemplateName(), "template_b");

modifier = 111;
TS_ASSERT_EQUALS(cmpPromotion.GetRequiredXp(), 1111);
modifier = 0;

cmpPromotion.IncreaseXp(200);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 200);

// Test promotion itself.
cmpPromotion.IncreaseXp(800);
TS_ASSERT_EQUALS(cmpPromotion.entity, 61);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
TS_ASSERT_EQUALS(cmpPromotion.GetRequiredXp(), 1000);
TS_ASSERT_EQUALS(cmpPromotion.GetPromotedTemplateName(), "template_f");

// Test multiple promotions at once.
cmpPromotion.IncreaseXp(4200);
TS_ASSERT_EQUALS(cmpPromotion.entity, 62);
TS_ASSERT_EQUALS(cmpPromotion.template.Entity, "end");
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 200);
TS_ASSERT_EQUALS(cmpPromotion.GetPromotedTemplateName(), "end");

// Test a dead entity can't promote.
cmpPromotion = ConstructComponent(entity, "Promotion", {
	"Entity": "template_b",
	"RequiredXp": 1000
});

let cmpHealth = AddMock(entity, IID_Health, {
	"GetHitpoints": () => 0,
});

cmpPromotion.IncreaseXp(1000);
TS_ASSERT_EQUALS(cmpPromotion.entity, entity);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
DeleteMock(entity, IID_Health);

// Test XP trickle.
let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", {});
cmpPromotion = ConstructComponent(entity, "Promotion", {
	"Entity": "template_b",
	"RequiredXp": "100",
	"TrickleRate": "10"
});
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 10);
cmpTimer.OnUpdate({ "turnLength": 2 });
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 30);

// Test promoted due to trickle.
cmpTimer.OnUpdate({ "turnLength": 8 });
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 10);
TS_ASSERT_EQUALS(cmpPromotion.entity, 61);

// Test valuemodification applies.
modifier = 10;
cmpPromotion.OnValueModification({ "component": "Promotion" });
cmpTimer.OnUpdate({ "turnLength": 4 });
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 90);
