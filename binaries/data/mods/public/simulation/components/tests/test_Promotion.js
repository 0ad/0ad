Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Promotion.js");

(function testMultipleXPIncrease()
{
let ApplyValueModificationsToEntity = (_, val) => val;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToEntity);

let QueryOwnerInterface = () => ({ "GetPlayerID": () => 2 });
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);

const ENT_ID = 60;

let entTemplates = {
	"60": "template_b",
	"61": "template_f",
	"62": "end",
};

let promote = {
	"template_b": "template_c",
	"template_c": "template_d",
	"template_d": "template_e",
	"template_e": "template_f",
};

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": (t) => ({
		"Promotion": {
			"Entity": promote[t],
			"RequiredXp": 1000
		},
	}),
});

let cmpPromotion = ConstructComponent(ENT_ID, "Promotion", {
	"Entity": "template_b",
	"RequiredXp": 1000
});

let ChangeEntityTemplate = function(ent, template)
{
	cmpPromotion = ConstructComponent(ent + 1, "Promotion", {
		"Entity": entTemplates[ent + 1],
		"RequiredXp": 1000
	});
	return ent + 1;
};
Engine.RegisterGlobal("ChangeEntityTemplate", ChangeEntityTemplate);

TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
cmpPromotion.IncreaseXp(200);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 200);
cmpPromotion.IncreaseXp(800);

TS_ASSERT_EQUALS(cmpPromotion.entity, 61);
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 0);
TS_ASSERT_EQUALS(cmpPromotion.GetRequiredXp(), 1000);
cmpPromotion.IncreaseXp(4200);
TS_ASSERT_EQUALS(cmpPromotion.entity, 62);
TS_ASSERT_EQUALS(cmpPromotion.template.Entity, "end");
TS_ASSERT_EQUALS(cmpPromotion.GetCurrentXp(), 200);

cmpPromotion = ConstructComponent(ENT_ID, "Promotion", {
	"Entity": "template_b",
	"RequiredXp": 1000
});

let cmpHealth = AddMock(ENT_ID, IID_Health, {
	"GetHitpoints": () => 0,
});

cmpPromotion.IncreaseXp(1000);
})();
