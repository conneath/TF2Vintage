#include "cbase.h"
#include "tf_gamerules.h"
#include "team_control_point.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot.h"
#include "map_entities/tf_hint.h"
#include "tf_bot_sniper_lurk.h"
#include "behavior/tf_bot_melee_attack.h"


ConVar tf_bot_debug_sniper( "tf_bot_debug_sniper", "0", FCVAR_CHEAT );
ConVar tf_bot_sniper_patience_duration( "tf_bot_sniper_patience_duration", "10", FCVAR_CHEAT, "How long a Sniper bot will wait without seeing an enemy before picking a new spot" );
ConVar tf_bot_sniper_target_linger_duration( "tf_bot_sniper_target_linger_duration", "2", FCVAR_CHEAT, "How long a Sniper bot will keep toward at a target it just lost sight of" );
ConVar tf_bot_sniper_allow_opportunistic( "tf_bot_sniper_allow_opportunistic", "1", FCVAR_NONE, "If set, Snipers will stop on their way to their preferred lurking spot to snipe at opportunistic targets" );
ConVar tf_mvm_bot_sniper_target_by_dps( "tf_mvm_bot_sniper_target_by_dps", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "If set, Snipers in MvM mode target the victim that has the highest DPS" );
extern ConVar tf_bot_sniper_melee_range;


CTFBotSniperLurk::CTFBotSniperLurk()
{
}

CTFBotSniperLurk::~CTFBotSniperLurk()
{
}


const char *CTFBotSniperLurk::GetName() const
{
	return "SniperLurk";
}


ActionResult<CTFBot> CTFBotSniperLurk::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );

	m_vecHome = me->GetAbsOrigin();
	m_bHasHome = false;
	m_bNearHome = false;
	m_nAttempts = 0;
	m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

	CBaseEntity *pEntity = nullptr;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_tfbot_hint" ) ) != nullptr )
	{
		CTFBotHint *pHint = static_cast<CTFBotHint *>( pEntity );

		if ( pHint->m_hint == CTFBotHint::SNIPER_SPOT )
		{
			m_Hints.AddToTail( pHint );

			if ( me->IsSelf( pHint->GetOwnerEntity() ) )
				pHint->SetOwnerEntity( nullptr );
		}
	}

	m_hHint = nullptr;

	if ( TFGameRules()->IsMannVsMachineMode() && me->GetTeamNumber() == TF_TEAM_MVM_BOTS )
		me->SetMission( CTFBot::MissionType::SNIPER, false );

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::Update( CTFBot *me, float dt )
{
	me->AccumulateSniperSpots();

	if ( !m_bHasHome )
		FindNewHome( me );

	bool bWantsToZoom = false;

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && threat->GetEntity()->IsAlive() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		if ( threat->IsVisibleInFOVNow() )
		{
			m_nAttempts = 0;

			if ( threat->GetLastKnownPosition().DistToSqr( me->GetAbsOrigin() ) < Square( tf_bot_sniper_melee_range.GetFloat() ) )
				return Action<CTFBot>::SuspendFor( new CTFBotMeleeAttack( 1.25f * tf_bot_sniper_melee_range.GetFloat() ), "Melee attacking nearby threat" );
		}

		if ( threat->GetTimeSinceLastSeen() < tf_bot_sniper_target_linger_duration.GetFloat() && me->IsLineOfFireClear( threat->GetEntity() ) )
		{
			if ( m_bOpportunistic )
			{
				CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
				if ( pPrimary != nullptr )
				{
					me->Weapon_Switch( pPrimary );
				}

				m_patienceDuration.Reset();

				bWantsToZoom = true;

				if ( !m_bHasHome )
				{
					m_vecHome = me->GetAbsOrigin();

					m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );
				}
			}
			else
			{
				CBaseCombatWeapon *pSecondary = me->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY );
				if ( pSecondary != nullptr )
				{
					me->Weapon_Switch( pSecondary );
				}
			}
		}
	}

	float flDistToHome = ( me->GetAbsOrigin().AsVector2D() - m_vecHome.AsVector2D() ).LengthSqr();
	m_bNearHome = ( flDistToHome < Square( 25.0f ) );

	if ( m_bNearHome )
	{
		bWantsToZoom = true;
		m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

		if ( m_patienceDuration.IsElapsed() )
		{
			++m_nAttempts;

			if ( FindNewHome( me ) )
			{
				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE );

				m_patienceDuration.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );
			}
			else
			{
				m_patienceDuration.Start( 1.0f );
			}
		}
	}
	else
	{
		m_patienceDuration.Reset();
	}

	if ( !bWantsToZoom )
	{
		if ( m_recomputePathTimer.IsElapsed() )
		{
			m_recomputePathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_PathFollower.Compute( me, m_vecHome, cost );
		}

		m_PathFollower.Update( me );

		if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			me->PressAltFireButton();
		}
	}
	else
	{
		CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
		if ( pPrimary != nullptr )
		{
			me->Weapon_Switch( pPrimary );

			CTFWeaponBase *pWeapon = static_cast<CTFWeaponBase *>( pPrimary );
			if ( !me->m_Shared.InCond( TF_COND_ZOOMED ) && !pWeapon->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
			{
				me->PressAltFireButton();
			}
		}
	}

	return Action<CTFBot>::Continue();
}

void CTFBotSniperLurk::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	if ( m_hHint )
	{
		m_hHint->SetOwnerEntity( nullptr );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}
}

ActionResult<CTFBot> CTFBotSniperLurk::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	if ( m_hHint )
	{
		m_hHint->SetOwnerEntity( nullptr );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	m_recomputePathTimer.Invalidate();

	m_hHint = nullptr;
	FindNewHome( me );

	return Action<CTFBot>::Continue();
}


QueryResultType CTFBotSniperLurk::ShouldRetreat( const INextBot *me ) const
{
	if ( TFGameRules()->IsMannVsMachineMode() && me->GetEntity()->GetTeamNumber() == TF_TEAM_MVM_BOTS )
		return ANSWER_NO;

	return ANSWER_UNDEFINED;
}

QueryResultType CTFBotSniperLurk::ShouldAttack( const INextBot *me, const CKnownEntity *threat ) const
{
	CTFBot *pMe = ToTFBot( me->GetEntity() );
	CTFNavArea *pNavArea = pMe->GetLastKnownArea();
	if ( TFGameRules()->IsMannVsMachineMode() && pNavArea && pNavArea->HasTFAttributes( TF_NAV_BLUE_SPAWN_ROOM ) )
		return ANSWER_NO;

	return ANSWER_YES;
}

const CKnownEntity *CTFBotSniperLurk::SelectMoreDangerousThreat( const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2 ) const
{
	if ( TFGameRules()->IsMannVsMachineMode() && tf_mvm_bot_sniper_target_by_dps.GetBool() )
	{
		CTFBot *pMe = ToTFBot( nextbot->GetEntity() );

		if ( !threat1->IsVisibleRecently() )
		{
			if ( threat2->IsVisibleRecently() )
				return threat2;
		}
		else if ( !threat2->IsVisibleRecently() )
		{
			return threat1;
		}

		CTFPlayer *pTFThreat1 = ToTFPlayer( threat1->GetEntity() );
		CTFPlayer *pTFThreat2 = ToTFPlayer( threat2->GetEntity() );
		if ( !pTFThreat1 || !pTFThreat2 )
			return nullptr;

		float flRangeSq1 = pMe->GetRangeSquaredTo( pTFThreat1 );
		float flRangeSq2 = pMe->GetRangeSquaredTo( pTFThreat2 );

		if ( pMe->HasWeaponRestriction( CTFBot::WeaponRestrictionType::MELEEONLY ) )
		{
			if ( flRangeSq1 > flRangeSq2 )
				return threat2;
			else
				return threat1;
		}

		if ( flRangeSq1 < Sqr( 500.0f ) )
		{
			if ( flRangeSq2 > Sqr( 500.0f ) )
				return threat1;
		}
		else if ( flRangeSq2 < Sqr( 500.0f ) )
		{
			return threat2;
		}

		if ( pTFThreat1->GetDamagePerSecond() > ( pTFThreat2->GetDamagePerSecond() + 50 ) )
			return threat1;
		else if ( pTFThreat2->GetDamagePerSecond() > ( pTFThreat1->GetDamagePerSecond() + 50 ) )
			return threat2;
		else if ( flRangeSq1 < flRangeSq2 )
			return threat1;
		else
			return threat2;
	}

	return nullptr;
}


bool CTFBotSniperLurk::FindHint( CTFBot *actor )
{
	CUtlVector<CTFBotHint *> hints;
	FOR_EACH_VEC( m_Hints, i )
	{
		if ( !m_Hints[i]->IsFor( actor ) )
			continue;

		hints.AddToTail( m_Hints[i] );
	}

	if ( m_hHint )
	{
		m_hHint->SetOwnerEntity( NULL );
		if ( tf_bot_debug_sniper.GetBool() )
			DevMsg( "%3.2f: %s: Releasing hint.", gpGlobals->curtime, actor->GetPlayerName() );
	}

	if ( hints.IsEmpty() )
		return false;

	CTFBotHint *pSelected = nullptr;
	if ( !m_hHint || m_nAttempts > 1 )
	{
		CUtlVector<CTFPlayer *> enemies;
		CollectPlayers( &enemies, GetEnemyTeam( actor ), COLLECT_ONLY_LIVING_PLAYERS );

		CUtlVector<CTFBotHint *> emptyHints;
		CUtlVector<CTFBotHint *> dangerHints;
		FOR_EACH_VEC( hints, i )
		{
			if ( hints[i]->GetOwnerEntity() == nullptr )
			{
				emptyHints.AddToTail( hints[i] );

				FOR_EACH_VEC( enemies, j )
				{
					if ( !enemies[j]->IsLineOfSightClear( hints[i]->WorldSpaceCenter(), CBaseCombatCharacter::IGNORE_ACTORS ) )
						continue;

					dangerHints.AddToTail( hints[i] );
				}
			}
		}

		if ( !dangerHints.IsEmpty() )
		{
			pSelected = dangerHints.Random();
		}
		else
		{
			if ( !emptyHints.IsEmpty() )
			{
				pSelected = emptyHints.Random();
			}
			else
			{
				pSelected = hints.Random();

				if ( tf_bot_debug_sniper.GetBool() )
					DevMsg( "%3.2f: %s: No un-owned hints available! Doubling up.\n", gpGlobals->curtime, actor->GetPlayerName() );
			}
		}

		if ( pSelected )
		{
			Vector vecMins, vecMaxs;
			pSelected->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );

			Vector vecRandomBounds;
			vecRandomBounds.x = RandomFloat( vecMins.x, vecMaxs.x );
			vecRandomBounds.y = RandomFloat( vecMins.y, vecMaxs.y );
			vecRandomBounds.z = ( vecMaxs.z + vecMins.z ) / 2.0f;

			TheNavMesh->GetSimpleGroundHeight( vecRandomBounds, &vecRandomBounds.z );

			m_bHasHome = true;
			m_vecHome = vecRandomBounds;
			m_hHint = pSelected;
			pSelected->SetOwnerEntity( actor );

			return true;
		}

		return false;
	}

	CUtlVector<CTFBotHint *> backupHints;
	if ( hints.IsEmpty() )
	{
		++m_nAttempts;
		return false;
	}

	FOR_EACH_VEC( hints, i )
	{
		if ( m_hHint == hints[i] )
			continue;

		if ( m_hHint->WorldSpaceCenter().DistToSqr( hints[i]->WorldSpaceCenter() ) > Square( 500.0f ) )
			continue;

		backupHints.AddToTail( hints[i] );
	}

	if ( backupHints.IsEmpty() )
		return false;

	pSelected = backupHints.Random();

	Vector vecMins, vecMaxs;
	pSelected->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );

	Vector vecRandomBounds;
	vecRandomBounds.x = RandomFloat( vecMins.x, vecMaxs.x );
	vecRandomBounds.y = RandomFloat( vecMins.y, vecMaxs.y );
	vecRandomBounds.z = ( vecMaxs.z + vecMins.z ) / 2.0f;

	TheNavMesh->GetSimpleGroundHeight( vecRandomBounds, &vecRandomBounds.z );

	m_bHasHome = true;
	m_vecHome = vecRandomBounds;
	m_hHint = pSelected;
	pSelected->SetOwnerEntity( actor );

	return true;
}

bool CTFBotSniperLurk::FindNewHome( CTFBot *actor )
{
	if ( !m_findHomeTimer.IsElapsed() )
		return false;

	m_findHomeTimer.Start( RandomFloat( 1.0f, 2.0f ) );

	if ( FindHint( actor ) )
		return true;

	if ( actor->GetSniperSpots().IsEmpty() )
	{
		m_bHasHome = false;

		CTeamControlPoint *pPoint = actor->GetMyControlPoint();
		if ( pPoint )
		{
			CCopyableUtlVector<CTFNavArea *> areas( (const CCopyableUtlVector<CTFNavArea *> &)TFNavMesh()->GetControlPointAreas( pPoint->GetPointIndex() ) );
			if ( areas.IsEmpty() )
			{
				TFNavMesh()->CollectSpawnRoomThresholdAreas( &areas, actor->GetTeamNumber() );
				if ( areas.IsEmpty() )
				{
					m_vecHome = actor->GetAbsOrigin();
				}
				else
				{
					CTFNavArea *area = areas.Random();
					m_vecHome = area->GetRandomPoint();
				}
			}
			else
			{
				CTFNavArea *area = areas.Random();
				m_vecHome = area->GetRandomPoint();
			}
		}

		return false;
	}

	m_vecHome = actor->GetSniperSpots().Random().m_vecVantage;
	m_bHasHome = true;

	return true;
}
