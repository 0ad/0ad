let identityTemplate = {
	"Classes": { "@datatype": "tokens", "_string": "b a" },
	"VisibleClasses": { "@datatype": "tokens", "_string": "c ß" }
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), ["b", "a", "c", "ß"]);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), ["c", "ß"]);

identityTemplate = {
	"Classes": { "@datatype": "tokens", "_string": "" },
	"VisibleClasses": { "@datatype": "tokens", "_string": "c ß" }
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), ["c", "ß"]);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), ["c", "ß"]);

identityTemplate = {
	"Classes": { "@datatype": "tokens", "_string": "classA" },
	"VisibleClasses": { "@datatype": "tokens", "_string": "classC classF" },
	"Rank": "testRank"
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), ["classA", "classC", "classF", "testRank"]);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), ["classC", "classF"]);

identityTemplate = {
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), []);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), []);

identityTemplate = {
	"Classes": { "@datatype": "tokens", "_string": "" },
	"VisibleClasses": { "@datatype": "tokens", "_string": "" },
	"Rank": ""
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), []);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), []);

identityTemplate = {
	"Classes": {},
	"VisibleClasses": {},
	"Rank": ""
};

TS_ASSERT_UNEVAL_EQUALS(GetIdentityClasses(identityTemplate), []);
TS_ASSERT_UNEVAL_EQUALS(GetVisibleIdentityClasses(identityTemplate), []);
