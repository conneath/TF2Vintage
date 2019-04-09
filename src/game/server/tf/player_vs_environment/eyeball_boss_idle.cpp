#include "cbase.h"
#include "tf_player.h"
#include "eyeball_boss_idle.h"
#include "eyeball_boss_escape.h"
#include "eyeball_boss_teleport.h"
#include "eyeball_boss_emote.h"
#include "eyeball_boss_notice.h"

extern ConVar tf_eyeball_boss_lifetime;
extern ConVar tf_eyeball_boss_lifetime_spell;
extern ConVar tf_eyeball_boss_hover_height;
extern ConVar tf_eyeball_boss_debug;


CEyeBallBossIdle::CEyeBallBossIdle()
{
}


const char *CEyeBallBossIdle::GetName( void ) const
{
	return "Idle";
}


ActionResult<CEyeBallBoss> CEyeBallBossIdle::OnStart( CEyeBallBoss *me, Action<CEyeBallBoss> *priorAction )
{
	float flRandom = RandomFloat( 3.0f, 5.0f );
	me->m_idleTimer.Start( flRandom );

	if ( me->GetTeamNumber() == TF_TEAM_BOSS )
		me->m_lifeTimeDuration.Start( flRandom + tf_eyeball_boss_lifetime.GetFloat() );
	else
		me->m_lifeTimeDuration.Start( flRandom + tf_eyeball_boss_lifetime_spell.GetFloat() );

	me->m_iOldHealth = me->GetHealth();

	me->m_teleportTimer.Start( RandomFloat( 10.0f, 15.0f ) );

	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossIdle::OnResume( CEyeBallBoss *me, Action<CEyeBallBoss> *interruptingAction )
{
	me->m_idleTimer.Start( RandomFloat( 3.0f, 5.0f ) );
	return Action<CEyeBallBoss>::Continue();
}

ActionResult<CEyeBallBoss> CEyeBallBossIdle::Update( CEyeBallBoss *me, float dt )
{
	if ( me->GetTeamNumber() == TF_TEAM_BOSS )
	{
		const float flTimeLeft = me->m_lifeTimeDuration.GetRemainingTime();
		if ( flTimeLeft >= 10.0f || me->m_flTimeLeftAlive <= 10.0f )
		{
			if ( flTimeLeft >= 30.0f || me->m_flTimeLeftAlive <= 30.0f )
			{
				if ( flTimeLeft >= 60.0f || me->m_flTimeLeftAlive <= 60.0f )
				{
					me->m_flTimeLeftAlive = flTimeLeft;
				}
				else
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
					if ( event )
					{
						event->SetInt( "time_remaining", 60 );
						event->SetInt( "level", me->GetLevel() );

						gameeventmanager->FireEvent( event );
					}

					me->m_flTimeLeftAlive = flTimeLeft;
				}
			}
			else
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
				if ( event )
				{
					event->SetInt( "time_remaining", 30 );
					event->SetInt( "level", me->GetLevel() );

					gameeventmanager->FireEvent( event );
				}

				me->m_flTimeLeftAlive = flTimeLeft;
			}
		}
		else
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escape_imminent" );
			if ( event )
			{
				event->SetInt( "time_remaining", 10 );
				event->SetInt( "level", me->GetLevel() );

				gameeventmanager->FireEvent( event );
			}

			me->m_flTimeLeftAlive = flTimeLeft;
		}
	}
	else if ( me->m_lifeTimeDuration.IsElapsed() )
		return Action<CEyeBallBoss>::ChangeTo( new CEyeBallBossEscape, "Escaping..." );

	if ( !me->m_teleportTimer.IsElapsed() || me->GetTeamNumber() != TF_TEAM_BOSS )
	{
		CEyeBallBossLocomotion *pLoco = (CEyeBallBossLocomotion *)me->GetLocomotionInterface();
		pLoco->SetDesiredAltitude( tf_eyeball_boss_hover_height.GetFloat() );
	}
	else
	{
		CEyeBallBossLocomotion *pLoco = (CEyeBallBossLocomotion *)me->GetLocomotionInterface();
		pLoco->SetDesiredAltitude( 0 );

		Vector vecPos = me->WorldSpaceCenter();
		float flHeight = 0;

		if ( TheNavMesh->GetSimpleGroundHeight( vecPos, &flHeight ) && TheNavMesh->GetNearestNavArea( vecPos, true, 450.0f ) )
		{
			if ( vecPos.z - flHeight < 300.0f )
			{
				if ( !me->m_lifeTimeDuration.IsElapsed() )
				{
					me->m_teleportTimer.Start( RandomFloat( 10.0f, 15.0f ) );
					return Action<CEyeBallBoss>::SuspendFor( new CEyeBallBossTeleport, "Moving..." );
				}
				else
				{
					return Action<CEyeBallBoss>::ChangeTo( new CEyeBallBossEscape, "Escaping..." );
				}
			}
		}
	}

	CBaseCombatCharacter *pVictim = me->FindNearestVisibleVictim();
	if ( !pVictim )
	{
		if ( me->m_hTarget )
		{
			me->GetBodyInterface()->AimHeadTowards( me->m_hTarget );
			m_lookAroundTimer.Start( RandomFloat( 0.5f, 2.0f ) );
			me->m_hTarget = nullptr;
		}
		else if ( m_lookAroundTimer.IsElapsed() )
		{
			m_lookAroundTimer.Start( RandomFloat( 2.0f, 4.0f ) );

			float flRandom = RandomFloat( -M_PI, M_PI );
			Vector vecOrigin = me->GetAbsOrigin();
			me->GetBodyInterface()->AimHeadTowards( vecOrigin + Vector( flRandom * 100.0f, flRandom * 100.0f, 0 ) );

			if ( me->m_bTaunt )
			{
				me->m_bTaunt = false;

				int iSequence = me->LookupSequence( "laugh" );
				const char *pszLaugh = "Halloween.EyeballBossLaugh";

				return Action<CEyeBallBoss>::SuspendFor( new CEyeBallBossEmote( iSequence, pszLaugh ), "Taunt our victim" );
			}
		}

		int iSequence = 0;
		if ( me->GetTeamNumber() != TF_TEAM_BOSS )
		{
			iSequence = me->LookupSequence( "lookaround3" );
		}
		else
		{
			if ( me->GetHealth() < ( me->GetMaxHealth() / 3 ) || ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
			{
				iSequence = me->LookupSequence( "lookaround3" );
			}
			else
			{
				if ( me->GetHealth() >= 2 * ( me->GetMaxHealth() / 3 ) || ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
				{
					iSequence = me->LookupSequence( "lookaround1" );
				}
				else
				{
					iSequence = me->LookupSequence( "lookaround2" );
				}
			}
		}

		if ( iSequence && ( iSequence != me->GetSequence() || me->IsSequenceFinished() ) )
		{
			me->SetSequence( iSequence );
			me->SetPlaybackRate( 1.0f );
			me->SetCycle( 0.0f );
			me->ResetSequenceInfo();
		}

		if ( me->m_idleNoiseTimer.IsElapsed() )
		{
			if ( me->GetHealth() < ( me->GetMaxHealth() / 3 ) || ( me->m_attitudeTimer.HasStarted() && !me->m_attitudeTimer.IsElapsed() ) )
			{
				me->EmitSound( "Halloween.EyeballBossRage" );
				me->m_idleNoiseTimer.Start( RandomFloat( 1.0f, 2.0f ) );
			}
			else
			{
				me->EmitSound( "Halloween.EyeballBossIdle" );
				me->m_idleNoiseTimer.Start( RandomFloat( 3.0f, 5.0f ) );
			}
		}

		return Action<CEyeBallBoss>::Continue();
	}

	me->m_hTarget = pVictim;

	return Action<CEyeBallBoss>::SuspendFor( new CEyeBallBossNotice, "Target found" );
}


EventDesiredResult<CEyeBallBoss> CEyeBallBossIdle::OnInjured( CEyeBallBoss *me, const CTakeDamageInfo& info )
{
	me->m_hTarget = (CBaseCombatCharacter *)info.GetAttacker();
	return Action<CEyeBallBoss>::TryContinue();
}

EventDesiredResult<CEyeBallBoss> CEyeBallBossIdle::OnOtherKilled( CEyeBallBoss *me, CBaseCombatCharacter *pOther, const CTakeDamageInfo& info )
{
	me->m_bTaunt = true;
	return Action<CEyeBallBoss>::TryContinue();
}
