#ifndef LIGHTCONTROL_H__
#define LIGHTCONTROL_H__

#include "GameInterface/Messages.h"
#include "GameInterface/Shareable.h"
#include "General/Observable.h"

class LightSphere;

class LightControl : public wxPanel
{
public:
	LightControl(wxWindow* parent, Observable<AtlasMessage::sEnvironmentSettings>& environment);
	~LightControl();

	void OnSettingsChange(const AtlasMessage::sEnvironmentSettings& settings);

	void NotifyOtherObservers();

private:
	Observable<AtlasMessage::sEnvironmentSettings>& m_Environment;
	ObservableConnection m_Conn;
	LightSphere* m_Sphere;
};

#endif // LIGHTCONTROL_H__
