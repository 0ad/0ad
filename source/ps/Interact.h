// Interact.h
//
// Manages main-screen interaction (screen->worldspace mapping, unit selection, ordering, etc.)
// and the hotkey message processor.
// Does this belong in GUI?

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include <vector>

#include "Singleton.h"
#include "simulation/EntityHandles.h"
#include "ps/Vector2D.h"
#include "lib/input.h"
#include "lib/res/handle.h"

class CVector3D;
class CUnit;
class CBoundingObject;

#define MAX_BOOKMARKS 10
#define MAX_GROUPS    20

// CSelectedEntities: the singleton containing entities currently selected on the local machine.
// (including group allocations on the local machine)

struct CSelectedEntities : public Singleton<CSelectedEntities>
{
	CSelectedEntities()
	{
		clearSelection();
		m_group = -1;
		m_group_highlight = -1;
		m_defaultCommand = -1;
		m_defaultAction = -1;
		m_secondaryCommand = -1;
		m_secondaryAction = -1;
		m_selectionChanged = true;
		m_mouseOverMM = false;

		loadUnitUITextures();
	}
	~CSelectedEntities()
	{
		destroyUnitUITextures();
	}
	std::vector<HEntity> m_selected;
	std::vector<HEntity> m_groups[MAX_GROUPS];
	i8 m_group, m_group_highlight;
	bool m_selectionChanged;
	bool m_mouseOverMM;
	int m_defaultCommand;
	int m_defaultAction;
	int m_secondaryCommand;
	int m_secondaryAction;

	void addSelection( HEntity entity );
	void removeSelection( HEntity entity );
	void setSelection( HEntity entity );
	void clearSelection();
	void removeAll( HEntity entity );
	bool isSelected( HEntity entity );
	CVector3D getSelectionPosition();

	void addToGroup( i8 groupid, HEntity entity );
	void saveGroup( i8 groupid );
	void loadGroup( i8 groupid );
	void addGroup( i8 groupid );
	void changeGroup( HEntity entity, i8 groupid );
	void highlightGroup( i8 groupid );
	void highlightNone();
	int getGroupCount( i8 groupid );
	CVector3D getGroupPosition( i8 groupid );

	void update();

	void renderSelectionOutlines();
	void renderOverlays();
	void renderHealthBars();
	void renderStaminaBars();
	void renderRanks();
	void renderBarBorders();
	void renderRallyPoints();
	
	void destroyUnitUITextures();
	int loadUnitUITextures();
	std::map<CStr, Handle> m_unitUITextures;
};

// CMouseoverEntities: the singleton containing entities the mouse is currently hovering over or bandboxing
// ( for mouseover selection outlines )

struct SMouseoverFader
{
	HEntity entity;
	float fade;
	bool isActive;
	SMouseoverFader( HEntity _entity, float _fade = 0.0f, bool _active = true ) : entity( _entity ), fade( _fade ), isActive( _active ) {}
};

struct CMouseoverEntities : public Singleton<CMouseoverEntities>
{
	float m_fadeinrate;
	float m_fadeoutrate;
	float m_fademaximum;
	CVector2D m_worldposition;
	HEntity m_target;
	HEntity m_lastTarget;
	bool m_targetChanged;

	bool m_bandbox, m_viewall;
	u16 m_x1, m_y1, m_x2, m_y2;

	CMouseoverEntities()
	{
		m_bandbox = false;
		m_viewall = false;
		m_fadeinrate = 1.0f;
		m_fadeoutrate = 2.0f;
		m_fademaximum = 0.5f;
		m_mouseover.clear();
		m_targetChanged = true;
	}
	std::vector<SMouseoverFader> m_mouseover;
	void update( float timestep );

	void addSelection();
	void removeSelection();
	void setSelection();

	void expandAcrossScreen();
	void expandAcrossWorld();

	void renderSelectionOutlines();
	void renderOverlays();
	void renderHealthBars();
	void renderStaminaBars();
	void renderRanks();
	void renderBarBorders();
	void renderRallyPoints();

	bool isBandbox() { return( m_bandbox ); }
	void startBandbox( u16 x, u16 y );
	void stopBandbox();
};

struct CBuildingPlacer : public Singleton<CBuildingPlacer>
{
	CBuildingPlacer()
	{
		m_active = false;
		m_actor = 0;
		m_bounds = 0;
	}

	CStrW m_templateName;
	bool m_active;
	bool m_clicked;
	bool m_dragged;
	float m_angle;
	bool m_valid;
	float m_timeSinceClick;
	float m_totalTime;
	CVector3D clickPos;
	CUnit* m_actor;
	CBoundingObject* m_bounds;

	bool activate( CStrW& templateName );
	void deactivate();
	void mousePressed();
	void mouseReleased();
	void update( float timeStep );
	void render();
};

bool isMouseoverType( CEntity* ev, void* userdata );
bool isOnScreen( CEntity* ev, void* userdata );

void StartCustomSelection();
void ResetInteraction();

InReaction interactInputHandler( const SDL_Event* ev );

#define g_Selection CSelectedEntities::GetSingleton()
#define g_Mouseover CMouseoverEntities::GetSingleton()
#define g_BuildingPlacer CBuildingPlacer::GetSingleton()

