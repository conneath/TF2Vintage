//========= Copyright � 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_CLIENTMODE_H
#define TF_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tf_viewport.h"
#include "GameUI/IGameUI.h"

class CHudMenuEngyBuild;
class CHudMenuEngyDestroy;
class CHudMenuSpyDisguise;
class CHudInspectPanel;
class CTFFreezePanel;

#if defined( _X360 )
class CTFClientScoreBoardDialog;
#endif

// Hud Styles 
enum ETFHudStyle
{
	TF_HUDSTYLE_DEFAULT = 0,	// Default Hud
	TF_HUDSTYLE_MINMODE,
	TF_HUDSTYLE_XBOX,
	TF_HUDSTYLE_VR,				// UNTESTED
};

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	OverrideView( CViewSetup *pSetup );

//	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual bool	DoPostScreenSpaceEffects( const CViewSetup *pSetup );

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();

	virtual bool	ShouldDrawCrosshair( void );

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui();

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );

	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void	ComputeVguiResConditions( KeyValues *pkvConditions ) OVERRIDE;

	virtual int		GetHUDStyle() { return m_pHudStyle; }
	virtual void	SetHUDStyle( int HudStyle );
	virtual bool	IsHUDMinMode() { if ( GetHUDStyle() == TF_HUDSTYLE_MINMODE ) return true; return false; };
	
private:
	
	void			MessageHooks( void );
	//void			UpdateSpectatorMode( void );

	void			PrintTextToChat( const char *msg );

private:

	ETFHudStyle m_pHudStyle;

	CHudMenuEngyBuild *m_pMenuEngyBuild;
	CHudMenuEngyDestroy *m_pMenuEngyDestroy;
	CHudMenuSpyDisguise *m_pMenuSpyDisguise;
	CTFFreezePanel		*m_pFreezePanel;
	CHudInspectPanel	*m_pInspectPanel;
	IGameUI			*m_pGameUI;

#if defined( _X360 )
	CTFClientScoreBoardDialog	*m_pScoreboard;
#endif
};


extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

void HandleBreakModel( bf_read &msg, bool bNoAngles );

#endif // TF_CLIENTMODE_H
