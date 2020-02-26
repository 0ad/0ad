Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/AutoBuildable.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("AutoBuildable.js");

const cmpBuildableAuto = ConstructComponent(10, "AutoBuildable", {
	"Rate": "1.0"
});

TS_ASSERT_EQUALS(cmpBuildableAuto.GetRate(), 1);

const cmpBuildableNoRate = ConstructComponent(12, "AutoBuildable", {
	"Rate": "0"
});
TS_ASSERT_EQUALS(cmpBuildableNoRate.GetRate(), 0);
