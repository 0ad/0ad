#include "precompiled.h"
#include "ProfileViewer.h"
#include "Profile.h"
#include "Renderer.h"
#include "lib/res/graphics/unifont.h"
#include "Hotkey.h"

bool profileVisible = false;
extern int g_xres, g_yres;

const CProfileNode* currentNode = NULL;

void ResetProfileViewer()
{
	currentNode = NULL;
}

void RenderProfileNode( CProfileNode* node, int currentNodeid )
{
	glPushMatrix();
	if( node->CanExpand() )
	{
		glPushMatrix();
		glTranslatef( -15.0f, 0.0f, 0.0f );
		glwprintf( L"%d", currentNodeid );
		glPopMatrix();
	}

	glPushMatrix();
	glwprintf( L"%hs", node->GetName() );
	glPopMatrix();
	glTranslatef( 230.0f, 0.0f, 0.0f );
	glPushMatrix();
#ifdef PROFILE_AMORTIZE
	glwprintf( L"%.3f", node->GetFrameCalls() );
#else
	glwprintf( L"%d", node->GetFrameCalls() );
#endif
	glPopMatrix();
	glTranslatef( 100.0f, 0.0f, 0.0f );
	glPushMatrix();
	glwprintf( L"%.3f", node->GetFrameTime() * 1000.0f );
	glPopMatrix();
	glTranslatef( 100.0f, 0.0f, 0.0f );
	glPushMatrix();
	
	glwprintf( L"%.1f", node->GetFrameTime() * 100.0 / g_Profiler.GetRoot()->GetFrameTime() );
	glPopMatrix();
	glTranslatef( 100.0f, 0.0f, 0.0f );
	glPushMatrix();
	glwprintf( L"%.1f", node->GetFrameTime() * 100.0 / currentNode->GetFrameTime() );
	glPopMatrix();
	glPopMatrix();

	glTranslatef( 0.0f, 20.0f, 0.0f );
}

void RenderProfile()
{
	if( !profileVisible ) return;
	if( !currentNode ) currentNode = g_Profiler.GetRoot();

	int estimate_height;
	 
	estimate_height = 6 + (int)currentNode->GetChildren()->size() + (int)currentNode->GetScriptChildren()->size();
	estimate_height = 20*estimate_height;
	
	glDisable(GL_TEXTURE_2D);
	glColor4ub(0,0,0,128);
	glBegin(GL_QUADS);
		glVertex2i(0, g_yres);
		glVertex2i(660, g_yres);
		glVertex2i(660, g_yres-estimate_height);
		glVertex2i(0, g_yres-estimate_height);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);

	glTranslatef(2.0f, g_yres - 20.0f, 0.0f );
	glScalef(1.0f, -1.0f, 1.0f);

	glPushMatrix();
	glwprintf( L"Profiling Information for: %hs (Time in node: %.3f msec/frame)", currentNode->GetName(), currentNode->GetFrameTime() * 1000.0f );
	glPopMatrix();
	glTranslatef( 20.0f, 20.0f, 0.0f );
	glPushMatrix();
	  glPushMatrix();
	  glwprintf( L"Name" );
	  glPopMatrix();
	  glTranslatef( 230.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"calls/frame" );
	  glPopMatrix();
	  glTranslatef( 100.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"msec/frame" );
	  glPopMatrix();
	  glTranslatef( 100.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"%%/frame" );
	  glPopMatrix();
	  glTranslatef( 100.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"%%/parent" );
	  glPopMatrix();
	glPopMatrix();


	CProfileNode::const_profile_iterator it;

	glTranslatef( 0.0f, 20.0f, 0.0f );

	float unlogged = (float)currentNode->GetFrameTime();

	int currentNodeid = 1;

	for( it = currentNode->GetChildren()->begin(); it != currentNode->GetChildren()->end(); it++, currentNodeid++ )
	{
		unlogged -= (float)(*it)->GetFrameTime();
		RenderProfileNode( *it, currentNodeid );
	}
	glColor3f( 1.0f, 0.5f, 0.5f );
	for( it = currentNode->GetScriptChildren()->begin(); it != currentNode->GetScriptChildren()->end(); it++, currentNodeid++ )
	{
		unlogged -= (float)(*it)->GetFrameTime();
		RenderProfileNode( *it, currentNodeid );
	}
	glColor3f( 1.0f, 1.0f, 1.0f );

	glTranslatef( 0.0f, 20.0f, 0.0f );
	glPushMatrix();
	  
	  glPushMatrix();
	  glwprintf( L"unlogged" );
	  glPopMatrix();
	  glTranslatef( 330.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"%.3f", unlogged * 1000.0f );
	  glPopMatrix();
	  glTranslatef( 100.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"%.1f", ( unlogged / g_Profiler.GetRoot()->GetFrameTime() ) * 100.0f );
	  glPopMatrix();
	  glTranslatef( 100.0f, 0.0f, 0.0f );
	  glPushMatrix();
	  glwprintf( L"%.1f", ( unlogged / currentNode->GetFrameTime() ) * 100.0f );
	  glPopMatrix();
	glPopMatrix();

	if( currentNode->GetParent() )
	{
		glTranslatef( 0.0f, 20.0f, 0.0f );
		glPushMatrix();
		glPushMatrix();
		glTranslatef( -15.0f, 0.0f, 0.0f );
		glwprintf( L"0" );
		glPopMatrix();
		glwprintf( L"back to parent" );
		glPopMatrix();
	}

	glPopMatrix();
}

InEventReaction profilehandler( const SDL_Event* ev )
{
	switch( ev->type )
	{
	case SDL_KEYDOWN:
	{
		if( profileVisible )
		{
			int k = ev->key.keysym.sym - SDLK_0;
			if( k == 0 )
			{
				if( currentNode->GetParent() )
					currentNode = currentNode->GetParent();
			}
			else if( ( k >= 1 ) && ( k <= 9 ) )
			{
				k--;
				CProfileNode::const_profile_iterator it;
				for( it = currentNode->GetChildren()->begin(); it != currentNode->GetChildren()->end(); it++, k-- )
					if( (*it)->CanExpand() && !k )
					{
						currentNode = (*it);
						return( IN_HANDLED );
					}
				for( it = currentNode->GetScriptChildren()->begin(); it != currentNode->GetScriptChildren()->end(); it++, k-- )
					if( (*it)->CanExpand() && !k )
					{
						currentNode = (*it);
						return( IN_HANDLED );
					}
				return( IN_HANDLED );
			}
		}
		break;
	}
	case SDL_HOTKEYDOWN:
		if( ev->user.code == HOTKEY_PROFILE_TOGGLE )
		{
			profileVisible = !profileVisible;
			return( IN_HANDLED );
		}
		break;
	}
	return( IN_PASS );
}
