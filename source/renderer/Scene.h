/**
 * File        : Scene.h
 * Project     : graphics
 * Description : This file contains the interfaces that are used to send a
 *             : scene to the renderer, and for the renderer to query objects
 *             : in that scene.
 *
 * @note This file would fit just as well into the graphics/ subdirectory.
 **/

#ifndef INCLUDED_SCENE
#define INCLUDED_SCENE

class CFrustum;
class CModel;
class CPatch;

class SceneCollector;

/**
 * This interface describes a scene to the renderer.
 *
 * @see CRenderer::RenderScene
 */
class Scene {
public:
	/**
	 * Send all objects that can be seen when rendering the given frustum
	 * to the scene collector.
	 * @param frustum The frustum that will be used for rendering.
	 * @param c The scene collector that should receive objects inside the frustum
	 *  that are visible.
	 */
	virtual void EnumerateObjects(const CFrustum& frustum, SceneCollector* c) = 0;
};


/**
 * This interface accepts renderable objects.
 *
 * @see Scene::EnumerateObjects
 */
class SceneCollector {
public:
	/**
	 * Submit a terrain patch that is part of the scene.
	 */
	virtual void Submit(CPatch* patch) = 0;

	/**
	 * Submit a model that is part of the scene,
	 * without submitting attached models.
	 */
	virtual void SubmitNonRecursive(CModel* model) = 0;

	/**
	 * Submit a model that is part of the scene,
	 * including attached sub-models.
	 *
	 * @note This function is implemented using SubmitNonRecursive,
	 * so you shouldn't have to reimplement it.
	 */
	virtual void SubmitRecursive(CModel* model);
};


#endif // INCLUDED_SCENE
