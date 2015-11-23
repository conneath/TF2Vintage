//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawning entity.
//
//=============================================================================//
#include "cbase.h"
#include "glow_outline_effect.h"
#include "c_tf_player.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_WeaponSpawner : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_WeaponSpawner, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_WeaponSpawner();

	virtual void OnDataChanged( DataUpdateType_t type );

	void	Spawn();
	void	ClientThink();
	void	UpdateGlowEffect();

private:
	QAngle				m_qAngle;
	CGlowObject		   *m_pGlowEffect;
	bool				m_bDisabled;
	bool				m_bRespawning;
	bool				m_bShouldGlow;
	bool				m_bTouchingPlayer;
};

LINK_ENTITY_TO_CLASS(tf_weaponspawner, C_WeaponSpawner);

IMPLEMENT_CLIENTCLASS_DT( C_WeaponSpawner, DT_WeaponSpawner, CWeaponSpawner )
	RecvPropBool( RECVINFO( m_bDisabled ) ),
	RecvPropBool( RECVINFO( m_bRespawning ) ),
END_RECV_TABLE()

C_WeaponSpawner::C_WeaponSpawner()
{
	m_bShouldGlow = false;
	m_bTouchingPlayer = false;
}

void C_WeaponSpawner::Spawn( void )
{
	BaseClass::Spawn();
	m_qAngle = GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Start thinking
//-----------------------------------------------------------------------------
void C_WeaponSpawner::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}

void C_WeaponSpawner::ClientThink()
{
	m_qAngle.y += 90 * gpGlobals->frametime;
	if ( m_qAngle.y >= 360 )
		m_qAngle.y -= 360;

	SetAbsAngles( m_qAngle );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	bool bShouldGlow = false;
	bool bTouchingPlayer = false;

	if ( pPlayer )
	{
		Vector vecPlayerOrigin = pPlayer->GetAbsOrigin();
		Vector vecPlayerMins = vecPlayerOrigin + pPlayer->GetPlayerMins();
		Vector vecPlayerMaxs = vecPlayerOrigin + pPlayer->GetPlayerMaxs();

		bTouchingPlayer = IsBoxIntersectingBox( GetAbsOrigin() + WorldAlignMins(), GetAbsOrigin() + WorldAlignMaxs(), vecPlayerMins, vecPlayerMaxs );

		// Disable the outline if the weapon has been picked up.
		if ( !m_bRespawning && !m_bDisabled )
		{
			// Temp crutch for Occluded\Unoccluded glow parameters not working.
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), pPlayer->EyePosition(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction == 1.0f )
			{
				bShouldGlow = true;
			}
		}
	}

	if ( m_bShouldGlow != bShouldGlow || m_bTouchingPlayer != bTouchingPlayer )
	{
		m_bShouldGlow = bShouldGlow;
		m_bTouchingPlayer = bTouchingPlayer;
		UpdateGlowEffect();
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_WeaponSpawner::UpdateGlowEffect()
{
	// Destroy effect if we don't want it.
	if ( !m_bShouldGlow )
	{
		if ( m_pGlowEffect )
		{
			delete m_pGlowEffect;
			m_pGlowEffect = NULL;
		}
	}
	else
	{
		Vector vecColor;

		if ( m_bTouchingPlayer )
		{
			// White glow.
			vecColor.Init( 0.76f, 0.76f, 0.76f );
		}
		else
		{
			// Blue glow.
			vecColor.Init( 0.6f, 0.6f, 1.0f );
		}

		if ( !m_pGlowEffect )
		{
			m_pGlowEffect = new CGlowObject( this, vecColor, 1.0, true, true );
		}
		else
		{
			m_pGlowEffect->SetColor( vecColor );
		}
	}
}



