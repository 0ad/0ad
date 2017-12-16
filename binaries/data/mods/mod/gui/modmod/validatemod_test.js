const g_ValidTestMods = {
	"public": {
		"name": "0ad",
		"version": "0.0.23",
		"label": "0 A.D. Empires Ascendant",
		"url": "play0ad.com",
		"description": "A free, open-source, historical RTS game.",
		"dependencies": []
	},
	"tm": {
		"name": "Terra_Magna",
		"version": "0.0.22",
		"label": "0 A.D. Terra Magna",
		"url": "forum.wildfiregames.com",
		"description": "Adds various civilizations to 0 A.D.",
		"dependencies": ["0ad"]
	},
	"mil": {
		"name": "millenniumad",
		"version": "0.0.22",
		"label": "0 A.D. Medieval Extension",
		"url": "forum.wildfiregames.com",
		"description": "Adds medieval content like civilizations + maps.",
		"dependencies": ["0ad=0.0.23"]
	}
};

const g_TestModsInvalid = {
	"broken1": {
		"name": "name may not contain whitespace",
		"version": "1",
		"label": "1",
		"description": "",
		"dependencies": []
	},
	"broken2": {
		"name": "broken2",
		"version": "0.0.2.1",
		"label": "2",
		"description": "it has too many dots in the version",
		"dependencies": []
	},
	"broken3": {
		"name": "broken3",
		"version": "broken3",
		"label": "3",
		"description": "version numbers must be numeric",
		"dependencies": []
	},
	"broken4": {
		"name": "broken4",
		"version": "4",
		"label": "4",
		"description": "dependencies must be mod names or valid comparisons",
		"dependencies": ["mod version=3"]
	},
	"broken5": {
		"name": "broken5",
		"version": "5",
		"label": "5",
		"description": "names in mod dependencies may not contain whitespace either",
		"dependencies": ["mod version"]
	},
	"broken6": {
		"name": "broken6",
		"version": "6",
		"label": "6",
		"description": "should have used =",
		"dependencies": ["mod==3"]
	},
	"broken7": {
		"name": "broken7",
		"version": "7",
		"label": "",
		"description": "label may not be empty",
		"dependencies": []
	},
	"broken8": {
		"name": "broken8",
		"version": "8",
		"label": "8",
		"description": "a version is an invalid dependency",
		"dependencies": ["0.0.23"]
	}
};

for (let folder in g_ValidTestMods)
	if (!validateMod(folder, g_ValidTestMods[folder], false))
		throw new Error("Valid mod '" + folder + "' should have passed the test.");

for (let folder in g_TestModsInvalid)
	if (validateMod(folder, g_TestModsInvalid[folder], false))
		throw new Error("Invalid mod '" + folder + "' should not have passed the test.");
