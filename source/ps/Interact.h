// Interact.h
//
// Manages main-screen interaction (screen->worldspace mapping, unit selection, ordering, etc.)
// and the hotkey message processor.
// Does this belong in GUI?

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include <vector>
#include "Singleton.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EntityMessage.h"
#include "Scheduler.h"
#include "Camera.h"

// In it's current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE 

#define SELECTION_CIRCLE_POINTS 25
#define SELECTION_BOX_POINTS 10

// CSelectedEntities: the singleton containing entities currently selected on the local machine.
// (including group allocations on the local machine)

struct CSelectedEntities : public Singleton<CSelectedEntities>
{
	CSelectedEntities() { clearSelection(); m_group = 255; m_group_highlight = 255; m_contextOrder = -1; }
	std::vector<CEntity*> m_selected;
	std::vector<CEntity*> m_groups[10];
	u8 m_group, m_group_highlight;
	int m_contextOrder;

	void addSelection( CEntity* entity );
	void removeSelection( CEntity* entity );
	void setSelection( CEntity* entity );
	void clearSelection();
	void removeAll( CEntity* entity );
	bool isSelected( CEntity* entity );
	CVector3D getSelectionPosition();

	void saveGroup( u8 groupid );
	void loadGroup( u8 groupid );
	void addGroup( u8 groupid );
	void changeGroup( CEntity* entity, u8 groupid );
	void highlightGroup( u8 groupid );
	void highlightNone();
	int getGroupCount( u8 groupid );
	CVector3D getGroupPosition( u8 groupid );

	void update();
	bool isContextValid( int contextOrder );
	void contextOrder( bool pushQueue = false );
	bool nextContext();
	bool previousContext();

	void renderSelectionOutlines();
	void renderOverlays();
};

// CMouseoverEntities: the singleton containing entities the mouse is currently hovering over or bandboxing
// ( for mouseover selection outlines )

struct SMouseoverFader
{
	CEntity* entity;
	float fade;
	bool isActive;
	SMouseoverFader( CEntity* _entity, float _fade = 0.0f, bool _active = true ) : entity( _entity ), fade( _fade ), isActive( _active ) {}
};

struct CMouseoverEntities : public Singleton<CMouseoverEntities>
{
	float m_fadeinrate;
	float m_fadeoutrate;
	float m_fademaximum;
	CEntity* m_target;

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
	bool isBandbox() { return( m_bandbox ); }
	void startBandbox( u16 x, u16 y );
	void stopBandbox();
};

bool isMouseoverType( CEntity* ev );
bool isOnScreen( CEntity* ev );

void pushCameraTarget( const CVector3D& target );
void setCameraTarget( const CVector3D& target );
void popCameraTarget();

int interactInputHandler( const SDL_Event* ev );

extern std::vector<CVector3D> cameraTargets;
extern CVector3D cameraDelta;

#define g_Selection CSelectedEntities::GetSingleton()
#define g_Mouseover CMouseoverEntities::GetSingleton()

