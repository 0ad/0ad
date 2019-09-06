/**
 * IMPORTANT: Remember to update session/top_panel/label.xml in sync with this.
 */
var g_ProjectInformation = {
	"organizationName": {
		"caption": translate("WILDFIRE GAMES")
	},
	"organizationLogo": {
		"sprite": "WildfireGamesLogo"
	},
	"productLogo": {
		"sprite": "0ADLogo"
	},
	"productBuild": {
		"caption": getBuildString()
	},
	"productDescription": {
		"caption": setStringTags(translate("Alpha XXIV"), { "font": "sans-bold-16" }) + "\n\n" +
			translate("Notice: This game is under development and many features have not been added yet.")
	}
};

var g_CommunityButtons = [
	{
		"caption": translate("Website"),
		"tooltip": translate("Click to open play0ad.com in your web browser."),
		"size": "8 100%-180 50%-4 100%-152",
		"onPress": () => {
			openURL("https://play0ad.com/");
		}
	},
	{
		"caption": translate("Chat"),
		"tooltip": translate("Click to open the 0 A.D. IRC chat in your browser. (#0ad on webchat.quakenet.org)"),
		"size": "50%+4 100%-180 100%-8 100%-152",
		"onPress": () => {
			openURL("https://webchat.quakenet.org/?channels=0ad");
		}
	},
	{
		"caption": translate("Report a Bug"),
		"tooltip": translate("Click to visit 0 A.D. Trac to report a bug, crash, or error."),
		"size": "8 100%-144 100%-8 100%-116",
		"onPress": () => {
			openURL("https://trac.wildfiregames.com/wiki/ReportingErrors/");
		}
	},
	{
		"caption": translate("Translate the Game"),
		"tooltip": translate("Click to open the 0 A.D. translate page in your browser."),
		"size": "8 100%-108 100%-8 100%-80",
		"onPress": () => {
			openURL("https://trac.wildfiregames.com/wiki/Localization");
		}
	},
	{
		"caption": translate("Donate"),
		"tooltip": translate("Help with the project expenses by donating."),
		"size": "8 100%-72 100%-8 100%-44",
		"onPress": () => {
			openURL("https://play0ad.com/community/donate/");
		}
	},
	{
		"caption": translate("Credits"),
		"tooltip": translate("Click to see the 0 A.D. credits."),
		"size": "8 100%-36 100%-8 100%-8",
		"onPress": () => {
			Engine.PushGuiPage("page_credits.xml");
		}
	}
];
