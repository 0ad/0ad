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

#ifndef INCLUDED_ICAMERACONTROLLER
#define INCLUDED_ICAMERACONTROLLER

#include "graphics/Camera.h"
#include "simulation2/system/Entity.h"

#include "lib/input.h" // InReaction - can't forward-declare enum

/**
 * @interface ICameraController defines a camera controller interface. The camera object
 * is owned by the camera controller's owner. It is therefore guaranteed that the lifetime
 * of the camera is at least the same as the lifetime of the camera controller.
 * The camera object is stored by reference, ensuring that the camera controller has full
 * control of the camera object during its own lifetime.
 */
class ICameraController
{
	NONCOPYABLE(ICameraController);
public:
	explicit ICameraController(CCamera& camera);
	virtual ~ICameraController();

	virtual void LoadConfig() = 0;

	virtual InReaction HandleEvent(const SDL_Event_* ev) = 0;

	virtual CVector3D GetCameraPivot() const = 0;
	virtual CVector3D GetCameraPosition() const = 0;
	virtual CVector3D GetCameraRotation() const = 0;
	virtual float GetCameraZoom() const = 0;

	virtual void SetCamera(const CVector3D& pos, float rotX, float rotY, float zoom) = 0;
	virtual void MoveCameraTarget(const CVector3D& target) = 0;
	virtual void ResetCameraTarget(const CVector3D& target) = 0;
	virtual void FollowEntity(entity_id_t entity, bool firstPerson) = 0;
	virtual entity_id_t GetFollowedEntity() = 0;

	virtual void Update(const float deltaRealTime) = 0;
	virtual void SetViewport(const SViewPort& vp) = 0;

	virtual bool GetConstrainCamera() const = 0;
	virtual void SetConstrainCamera(bool constrain) = 0;

protected:
	CCamera& m_Camera;
};

#endif // INCLUDED_ICAMERACONTROLLER
