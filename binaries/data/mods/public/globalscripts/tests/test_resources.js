let resources = {
	"res_A": {
		"code": "a",
		"name": "A",
		"subtypes": {
			"aa": "AA",
			"aaa": "AAA"
		},
		"order": 2,
		"properties": ["barterable", "tributable"]
	},
	"res_B": {
		"code": "b",
		"name": "B",
		"subtypes": {
			"bb": "BB",
			"bbb": "BBB"
		},
		"order": 1,
		"properties": ["tributable"]
	}
};

Engine.ListDirectoryFiles = () => Object.keys(resources);
Engine.ReadJSONFile = (file) => resources[file];

let res = new Resources();

TS_ASSERT_EQUALS(res.GetResources().length, 2);
TS_ASSERT_EQUALS(res.GetResources()[0].code, "b");

TS_ASSERT_EQUALS(res.GetResource("b").order, 1);

TS_ASSERT_UNEVAL_EQUALS(res.GetCodes(), ["b", "a"]);
TS_ASSERT_UNEVAL_EQUALS(res.GetTributableCodes(), ["b", "a"]);
TS_ASSERT_UNEVAL_EQUALS(res.GetBarterableCodes(), ["a"]);
TS_ASSERT_UNEVAL_EQUALS(res.GetTradableCodes(), []);

TS_ASSERT_UNEVAL_EQUALS(res.GetNames(), {
	"a": "A",
	"aa": "AA",
	"aaa": "AAA",
	"b": "B",
	"bb": "BB",
	"bbb": "BBB"
});
