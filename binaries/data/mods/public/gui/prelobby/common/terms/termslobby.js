var g_TermsButtonHeight = 40;

function initLobbyTerms()
{
	let terms = {
		"Service": {
			"title": translate("Terms of Service"),
			"instruction": translate("Please read the Terms of Service"),
			"file": "gui/prelobby/common/terms/Terms_of_Service.txt",
			"config": "lobby.terms_of_service",
			"salt": () => Engine.GetGUIObjectByName("username").caption,
			"accepted": false,
			"callback": updateFeedback
		},
		"Use": {
			"title": translate("Terms of Use"),
			"instruction": translate("Please read the Terms of Use"),
			"file": "gui/prelobby/common/terms/Terms_of_Use.txt",
			"config": "lobby.terms_of_use",
			"salt": () => Engine.GetGUIObjectByName("username").caption,
			"accepted": false,
			"callback": updateFeedback
		}
	};

	Object.keys(terms).forEach((page, i) => {

		let button = Engine.GetGUIObjectByName("termsButton[" + i + "]");

		button.caption = terms[page].title;

		button.onPress = () => {
			openTerms(page);
		};

		let size = button.size;
		size.top = i * g_TermsButtonHeight;
		size.bottom = i * g_TermsButtonHeight + 28;
		button.size = size;
	});

	initTerms(terms);
	loadTermsAcceptance();
}
