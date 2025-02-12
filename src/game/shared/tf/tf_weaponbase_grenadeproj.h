//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF basic grenade projectile functionality.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_GRENADEPROJ_H
#define TF_WEAPONBASE_GRENADEPROJ_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#define CTFWeaponBaseMerasmusGrenade C_TFWeaponBaseMerasmusGrenade
#endif

//=============================================================================
//
// TF base grenade projectile class.
//
class CTFWeaponBaseGrenadeProj : public CBaseGrenade
{
public:

	DECLARE_CLASS( CTFWeaponBaseGrenadeProj, CBaseGrenade );
	DECLARE_NETWORKCLASS();

							CTFWeaponBaseGrenadeProj();
	virtual					~CTFWeaponBaseGrenadeProj();
	virtual void			Spawn();
	virtual void			Precache();

	// Unique identifier.
	virtual int GetWeaponID( void ) const				{ return TF_WEAPON_NONE; }
	
	virtual int GetCustomParticleIndex( void ) const;

	// This gets sent to the client and placed in the client's interpolation history
	// so the projectile starts out moving right off the bat.
	CNetworkVector( m_vInitialVelocity );

	virtual float		GetShakeAmplitude( void )		{ return 10.0; }
	virtual float		GetShakeRadius( void )			{ return 300.0; }

	void				SetCritical( bool bCritical )	{ m_bCritical = bCritical; }
	virtual int			GetDamageType();
	virtual float		GetDamageRadius( void );
	virtual int			GetDamageCustom( void )			{ return TF_DMG_CUSTOM_NONE; }
	bool				Touched( void ) const			{ return m_bTouched; }

	CNetworkVar( int, m_iDeflected );
	CNetworkHandle( CBaseEntity, m_hLauncher );
	CNetworkHandle( CBaseEntity, m_hDeflectOwner );

protected:

	// Touch fix
	CNetworkVar( bool,		m_bTouched );

private:

	CTFWeaponBaseGrenadeProj( const CTFWeaponBaseGrenadeProj & );

	// Client specific.
#ifdef CLIENT_DLL

public:

	virtual void			OnPreDataChanged( DataUpdateType_t updateType );
	virtual void			OnDataChanged( DataUpdateType_t type );

	float					m_flSpawnTime;
	bool					m_bCritical;
	int						m_iOldTeamNum;

	// Server specific.
#else

public:

	DECLARE_DATADESC();

	static CTFWeaponBaseGrenadeProj *Create( const char *szName, const Vector &position, const QAngle &angles, 
				const Vector &velocity, const AngularImpulse &angVelocity, 
				CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, float timer, int iFlags = 0 );


	void					InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );
	virtual void			InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, int nDamage, float flRadius );

	int						OnTakeDamage( const CTakeDamageInfo &info );

	virtual void			DetonateThink( void );
	void					Detonate( void );

	void					SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	bool					ShouldNotDetonate( void );
	void					RemoveGrenade( bool bBlinkOut = true );

	void					SetTimer( float time ){ m_flDetonateTime = time; }
	virtual float			GetDetonateTime( void ){ return m_flDetonateTime; }

	void					SetDetonateTimerLength( float timer );

	void					VPhysicsUpdate( IPhysicsObject *pPhysics );

	virtual void			Explode( trace_t *pTrace, int bitsDamageType );

	bool					UseImpactNormal()							{ return m_bUseImpactNormal; }
	const Vector			&GetImpactNormal( void ) const				{ return m_vecImpactNormal; }

	virtual void			SetLauncher( CBaseEntity *pLauncher );

	virtual bool			IsDeflectable();
	virtual void			Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );
	virtual void			IncremenentDeflected( void );

protected:

	void					DrawRadius( float flRadius );

	bool					m_bUseImpactNormal;
	Vector					m_vecImpactNormal;

private:

	// Custom collision to allow for constant elasticity on hit surfaces.
	virtual void			ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity );

	float					m_flDetonateTime;

	bool					m_bInSolid;

	CNetworkVar( bool,		m_bCritical );

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;

#endif
};


class CTFWeaponBaseMerasmusGrenade : public CTFWeaponBaseGrenadeProj
{
public:

	DECLARE_CLASS( CTFWeaponBaseMerasmusGrenade, CTFWeaponBaseGrenadeProj );
	DECLARE_NETWORKCLASS();

	virtual					~CTFWeaponBaseMerasmusGrenade() {}

	virtual int				GetDamageCustom( void ) const { return TF_DMG_CUSTOM_MERASMUS_GRENADE; }
	virtual int				GetCustomParticleIndex( void );

#ifdef CLIENT_DLL
	virtual int				DrawModel( int flags );
#endif
};

#endif // TF_WEAPONBASE_GRENADEPROJ_H
