/* Copyright (C) 2019 Wildfire Games.
* This file is part of 0 A.D.
*
* 0 A.D. is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* 0 A.D. is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INCLUDED_CAMERACONTROLLER
#define INCLUDED_CAMERACONTROLLER

#include "graphics/Camera.h"
#include "graphics/SmoothedValue.h"
#include "simulation2/system/Entity.h"

#include "lib/input.h" // InReaction - can't forward-declare enum

class CCameraController
{
	NONCOPYABLE(CCameraController);
public:
	CCameraController(CCamera& camera);

	void Initialize();

	InReaction HandleEvent(const SDL_Event_* ev);

	CVector3D GetCameraPivot() const;
	CVector3D GetCameraPosition() const;
	CVector3D GetCameraRotation() const;
	float GetCameraZoom() const;

	void SetCamera(const CVector3D& pos, float rotX, float rotY, float zoom);
	void MoveCameraTarget(const CVector3D& target);
	void ResetCameraTarget(const CVector3D& target);
	void CameraFollow(entity_id_t entity, bool firstPerson);
	entity_id_t GetFollowedEntity();

	// Set projection of current camera using near, far, and FOV values
	void SetCameraProjection();

	void Update(const float deltaRealTime);
	void SetViewport(const SViewPort& vp);

	bool GetConstrainCamera() const
	{
		return m_ConstrainCamera;
	}

	void SetConstrainCamera(bool constrain)
	{
		m_ConstrainCamera = constrain;
	}

private:
	CVector3D GetSmoothPivot(CCamera &camera) const;
	void ResetCameraAngleZoom();
	void SetupCameraMatrixSmooth(CMatrix3D* orientation);
	void SetupCameraMatrixSmoothRot(CMatrix3D* orientation);
	void SetupCameraMatrixNonSmooth(CMatrix3D* orientation);
	void FocusHeight(bool smooth);

	CCamera& m_Camera;

	/**
	* Whether the camera movement should be constrained by min/max limits
	* and terrain avoidance.
	*/
	bool m_ConstrainCamera;

	/**
	* Entity for the camera to follow, or INVALID_ENTITY if none.
	*/
	entity_id_t m_FollowEntity;

	/**
	* Whether to follow FollowEntity in first-person mode.
	*/
	bool m_FollowFirstPerson;

	// Settings
	float m_ViewScrollSpeed;
	float m_ViewScrollSpeedModifier;
	float m_ViewRotateXSpeed;
	float m_ViewRotateXMin;
	float m_ViewRotateXMax;
	float m_ViewRotateXDefault;
	float m_ViewRotateYSpeed;
	float m_ViewRotateYSpeedWheel;
	float m_ViewRotateYDefault;
	float m_ViewRotateSpeedModifier;
	float m_ViewDragSpeed;
	float m_ViewZoomSpeed;
	float m_ViewZoomSpeedWheel;
	float m_ViewZoomMin;
	float m_ViewZoomMax;
	float m_ViewZoomDefault;
	float m_ViewZoomSpeedModifier;
	float m_ViewFOV;
	float m_ViewNear;
	float m_ViewFar;
	int m_JoystickPanX;
	int m_JoystickPanY;
	int m_JoystickRotateX;
	int m_JoystickRotateY;
	int m_JoystickZoomIn;
	int m_JoystickZoomOut;
	float m_HeightSmoothness;
	float m_HeightMin;

	// Camera Controls State
	CSmoothedValue m_PosX;
	CSmoothedValue m_PosY;
	CSmoothedValue m_PosZ;
	CSmoothedValue m_Zoom;
	CSmoothedValue m_RotateX; // inclination around x axis (relative to camera)
	CSmoothedValue m_RotateY; // rotation around y (vertical) axis
};

#endif // INCLUDED_CAMERACONTROLLER
