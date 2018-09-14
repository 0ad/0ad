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
		"errorMessage": uneval(data[2])
	})
};

function initUserReport()
{
	setUserReportEnabled(Engine.IsUserReportEnabled());
}

function setUserReportEnabled(enabled)
{
	Engine.SetUserReportEnabled(enabled);
	updateUserReportButtons();
}

function updateUserReportButtons()
{
	let userReportEnableButton = Engine.GetGUIObjectByName("userReportEnableButton");
	userReportEnableButton.caption = Engine.IsUserReportEnabled() ? translate("Disable Feedback") : translate("Enable Feedback");
	userReportEnableButton.onPress = () => {
		setUserReportEnabled(!Engine.IsUserReportEnabled());
	};

	let userReportTermsButton = Engine.GetGUIObjectByName("userReportTermsButton");
	userReportTermsButton.onPress = () => {
		Engine.PushGuiPage("page_manual.xml", {
			"page": "manual/userreport",
			"title": translate("Technical Details")
		});
	};
}

function updateUserReportStatus()
{
	let statusData = Engine.GetUserReportStatus().split(/:/, 3);

	Engine.GetGUIObjectByName("userReportText").caption =
		Engine.IsUserReportEnabled() ?
			setStringTags(translate("Thank you for helping improve 0 A.D.!"), { "font": "sans-bold-16" }) + "\n\n" +
			translate("Anonymous feedback is currently enabled.") + "\n" +
			sprintf(translate("Status: %(status)s."), {
				"status": g_UserReportStatusFormat[statusData[0]] ? g_UserReportStatusFormat[statusData[0]](statusData) : translate("unknown")
			}) :
			setStringTags(translate("Help improve 0 A.D.!"), { "font": "sans-bold-16" }) + "\n\n" +
			translate("You can automatically send us anonymous feedback that will help us fix bugs, and improve performance and compatibility.");
}
