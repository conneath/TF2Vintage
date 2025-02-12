//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "tf_weaponbase.h"
#include "tf_projectile_arrow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudBowChargeMeter : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudBowChargeMeter, EditablePanel );

public:
	CHudBowChargeMeter( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );
	virtual void	FireGameEvent( IGameEvent *event );

private:
	vgui::ContinuousProgressBar *m_pChargeMeter;
};

DECLARE_HUDELEMENT( CHudBowChargeMeter );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBowChargeMeter::CHudBowChargeMeter( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudBowCharge" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ChargeMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "arrow_impact" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudDemomanCharge.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBowChargeMeter::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) || !pPlayer->IsAlive() )
	{
		return false;
	}

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();

	if ( !pWpn )
	{
		return false;
	}

	int iWeaponID = pWpn->GetWeaponID();

	if ( iWeaponID != TF_WEAPON_COMPOUND_BOW )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBowChargeMeter::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	ITFChargeUpWeapon *pChargeupWeapon = dynamic_cast< ITFChargeUpWeapon *>( pWpn );

	if ( !pWpn || !pChargeupWeapon )
		return;

	if ( m_pChargeMeter )
	{
		float flChargeMaxTime = pChargeupWeapon->GetChargeMaxTime();

		if ( flChargeMaxTime != 0 )
		{
			float flChargeBeginTime = pChargeupWeapon->GetChargeBeginTime();

			if ( flChargeBeginTime > 0 )
			{
				float flTimeCharged = max( 0, gpGlobals->curtime - flChargeBeginTime );
				float flPercentCharged = min( 1.0, flTimeCharged / flChargeMaxTime );

				m_pChargeMeter->SetProgress( flPercentCharged );
			}
			else
			{
				m_pChargeMeter->SetProgress( 0.0f );
			}
		}
	}
}

void CHudBowChargeMeter::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( Q_strcasecmp( eventName, "arrow_impact" ) == 0 )
	{
		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( cl_entitylist->GetBaseEntity( event->GetInt( "attachedEntity" ) ) );
		if ( pPlayer )
		{
			C_TFProjectile_Arrow *pArrow = new C_TFProjectile_Arrow();
			if ( pArrow )
			{
				int bone = event->GetInt( "boneIndexAttached" );
				Vector vecPosition( event->GetFloat( "bonePositionX" ), event->GetFloat( "bonePositionY" ), event->GetFloat( "bonePositionZ" ) );
				QAngle vecAngles( event->GetFloat( "boneAnglesX" ), event->GetFloat( "boneAnglesY" ), event->GetFloat( "boneAnglesZ" ) );

				extern const char *g_pszArrowModels[];

				const char *szModel; float flModelScale;
				switch ( event->GetInt( "projectileType" ) )
				{
					case TF_PROJECTILE_ARROW:
					{
						szModel = g_pszArrowModels[0];
						flModelScale = 1.0f;
						break;
					}
					case TF_PROJECTILE_HEALING_BOLT:
					{
						szModel = g_pszArrowModels[1];
						flModelScale = 1.6f;

						Vector vecFwd;
						AngleVectors( vecAngles, &vecFwd );

						vecPosition -= vecFwd * 12;

						break;
					}
					case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
					{
						szModel = g_pszArrowModels[2];
						flModelScale = 1.0f;
						break;
					}
					case TF_PROJECTILE_FESTIVE_ARROW:
					{
						szModel = g_pszArrowModels[3];
						flModelScale = 1.0f;
						break;
					}
					case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
					{
						szModel = g_pszArrowModels[4];
						flModelScale = 1.4f;

						Vector vecFwd;
						AngleVectors( vecAngles, &vecFwd );

						vecPosition -= vecFwd * 12;

						break;
					}
					case TF_PROJECTILE_BREAD_MONSTER:
					case TF_PROJECTILE_BREADMONSTER_JARATE:
					case TF_PROJECTILE_BREADMONSTER_MADMILK:
					{
						szModel = g_pszArrowModels[5];
						// pull the syringe back slightly
						Vector vecFwd;
						AngleVectors( vecAngles, &vecFwd );
						vecPosition -= ( vecFwd * 1.0f );
						flModelScale = 2.5f;
						if ( event->GetBool( "critical" ) )
						{
							flModelScale = RandomFloat( 3.0f, 5.0f );
						}
						break;
					}
					case TF_PROJECTILE_CLEAVER:
					{
						szModel = g_pszArrowModels[6];
						flModelScale = 1.0f;
						break;
					}
					default:
					{
						Warning( " Unsupported Projectile type on event arrow_impact - %d\n\n", event->GetInt( "projectileType" ) );
						return;
					}
				}

				pArrow->InitializeAsClientEntity( szModel, RENDER_GROUP_OPAQUE_ENTITY );

				pArrow->SetModelScale( flModelScale );

				pArrow->AttachEntityToBone( pPlayer, bone, vecPosition, vecAngles );

				if 	( event->GetInt( "projectileType" ) == TF_PROJECTILE_BREAD_MONSTER ||
					event->GetInt( "projectileType" ) == TF_PROJECTILE_BREADMONSTER_JARATE || 
					event->GetInt( "projectileType" ) == TF_PROJECTILE_BREADMONSTER_MADMILK )
					pArrow->SetDieTime( gpGlobals->curtime + 10.0f );		// Bread monsters only survive for 10 seconds.
				else
					pArrow->SetDieTime( gpGlobals->curtime + 60.0f );		// Everything else hangs around for 60 seconds.
			}
		}
	}
}