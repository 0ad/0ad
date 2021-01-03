let effects = {
	"eff_A": {
		"code": "a",
		"name": "A",
		"order": "2",
		"IID": "IID_A",
		"method": "doA"
	},
	"eff_B": {
		"code": "b",
		"name": "B",
		"order": "1",
		"IID": "IID_B",
		"method": "doB"
	}
};

Engine.ListDirectoryFiles = () => Object.keys(effects);
Engine.ReadJSONFile = (file) => effects[file];

let attackEffects = new AttackEffects();

TS_ASSERT_UNEVAL_EQUALS(attackEffects.Receivers(), [{
	"type": "b",
	"IID": "IID_B",
	"method": "doB"
}, {
	"type": "a",
	"IID": "IID_A",
	"method": "doA"
}]);
