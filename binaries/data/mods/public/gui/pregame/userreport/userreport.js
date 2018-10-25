var g_TermsUserReport = {
	"TermsAndConditions": {
		"title": translateWithContext("UserReporter Terms and Conditions", "Terms"),
		"instruction": translate("Please read and accept the UserReporter Terms and Conditions."),
		"file": "gui/userreport/Terms_and_Conditions.txt",
		"termsURL": Engine.ConfigDB_GetValue("user", "userreport.url_terms"),
		"sprintf": {
			"logPath": setStringTags(Engine.GetUserReportLogPath(), { "font": "sans-bold-12" }),
			"configPath": setStringTags(Engine.GetUserReportConfigPath(), { "font": "sans-bold-12" })
		},
		"config": "userreport.terms",
		"callback": data => {
			setUserReportEnabled(data.accepted);
		},
		"accepted": false,
		"urlButtons": [
			{
				"caption": translate("Publications"),
				"url": Engine.ConfigDB_GetValue("user", "userreport.url_publication")
			}
		]
	}
};

var g_UserReportStatusFormat = {
	"disabled": data => translate("disabled"),
	"proxy": data => translate("connecting to server"),
	"waiting": data => translate("connecting to server"),
	"connecting": data => translate("connecting to server"),
	"sending": data => sprintf(translate("uploading (%f%%)"), Math.floor(100 * data[1])),
	"completed": data =>
		data[1] == 200 ?
			translate("upload succeeded") :
			sprintf(translate("upload failed (%(errorCode)s)"), {
				"errorCode": data[1]
			}),
	"failed": data => sprintf(translate("upload failed (%(errorMessage)s)"), {
		"errorMessage": data.slice(2).join(":")
	})
};

function initUserReport()
{
	initTerms(g_TermsUserReport);
	loadTermsAcceptance();

	setUserReportEnabled(!checkTerms() && Engine.IsUserReportEnabled());
}

function setUserReportEnabled(enabled)
{
	Engine.SetUserReportEnabled(enabled);
	updateUserReportButtons();
}

function updateUserReportButtons()
{
	let termsFeedback = checkTerms();

	let userReportEnableButton = Engine.GetGUIObjectByName("userReportEnableButton");
	userReportEnableButton.caption = Engine.IsUserReportEnabled() ? translate("Disable Feedback") : translate("Enable Feedback");
	userReportEnableButton.enabled = !termsFeedback;
	userReportEnableButton.tooltip = termsFeedback;
	userReportEnableButton.onPress = () => {
		setUserReportEnabled(!Engine.IsUserReportEnabled());
	};

	let userReportTermsButton = Engine.GetGUIObjectByName("userReportTermsButton");
	userReportTermsButton.caption = g_TermsUserReport.TermsAndConditions.title;
	userReportTermsButton.onPress = () => {
		openTerms("TermsAndConditions");
	};
}

function updateUserReportStatus()
{
	let statusData = Engine.GetUserReportStatus().split(":");

	Engine.GetGUIObjectByName("userReportText").caption =
		Engine.IsUserReportEnabled() ?
			setStringTags(translate("Thank you for helping improve 0 A.D.!"), { "font": "sans-bold-16" }) + "\n\n" +
			translate("Feedback is currently enabled.") + "\n" +
			sprintf(translate("Status: %(status)s."), {
				"status": g_UserReportStatusFormat[statusData[0]] ? g_UserReportStatusFormat[statusData[0]](statusData) : translate("unknown")
			}) :
			setStringTags(translate("Help improve 0 A.D.!"), { "font": "sans-bold-16" }) + "\n\n" +
			translate("You can automatically send us feedback that can help us fix bugs, and improve performance and compatibility.");
}
