//=============================================================================//
//
// Purpose: Base class for predatory NPCs like the Bullsquid.
//
// Heavily based on the bullsquid but adapted so that another monster can extend
// it in Entropy : Zero 2.
//
// 1upD
//
//=============================================================================//

#include "cbase.h"
#include "AI_Hint.h"
#include "AI_Senses.h"
#include "npc_basepredator.h"
#include "ai_localnavigator.h"
#include "fire.h"
#ifdef EZ2
#include "npc_wilson.h"
#include "hl2_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#include <Sprite.h>

ConVar ai_debug_predator("ai_debug_predator", "0");
ConVar sv_predator_heal_render_effects("sv_predator_heal_render_effects", "1", FCVAR_NONE, "Should predators like bullsquids turn green after healing?");

LINK_ENTITY_TO_CLASS(npc_basepredator, CNPC_BasePredator);

string_t CNPC_BasePredator::gm_iszGooPuddle;

//---------------------------------------------------------
// Activities
//---------------------------------------------------------
int ACT_EAT;
int ACT_EXCITED;
int ACT_DETECT_SCENT;
int ACT_INSPECT_FLOOR;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BasePredator)
DEFINE_FIELD(m_fCanThreatDisplay, FIELD_BOOLEAN),
DEFINE_FIELD(m_flLastHurtTime, FIELD_TIME),
DEFINE_FIELD(m_flNextSpitTime, FIELD_TIME),
DEFINE_FIELD(m_tBossState, FIELD_INTEGER),

DEFINE_FIELD(m_flHungryTime, FIELD_TIME),
DEFINE_KEYFIELD(m_bSpawningEnabled, FIELD_BOOLEAN, "SpawningEnabled"),
DEFINE_KEYFIELD(m_iTimesFed, FIELD_INTEGER, "TimesFed"),
DEFINE_FIELD(m_flNextSpawnTime, FIELD_TIME),
DEFINE_FIELD(m_hMate, FIELD_EHANDLE),
DEFINE_KEYFIELD(m_bIsBaby, FIELD_BOOLEAN, "IsBaby"),
DEFINE_KEYFIELD(m_iszMate, FIELD_STRING, "Mate"),

DEFINE_KEYFIELD(m_bIsBoss, FIELD_BOOLEAN, "IsBoss"),
DEFINE_KEYFIELD(m_tWanderState, FIELD_INTEGER, "WanderStrategy"),
DEFINE_KEYFIELD(m_bDormant, FIELD_BOOLEAN, "Dormant"),

DEFINE_FIELD(m_nextSoundTime, FIELD_TIME),

DEFINE_OUTPUT(m_OnBossHealthReset, "OnBossHealthReset"),
DEFINE_OUTPUT(m_OnFed, "OnFed"),
DEFINE_OUTPUT(m_OnSpawnNPC, "OnSpawnNPC"),

DEFINE_INPUTFUNC(FIELD_VOID, "EnterNormalMode", InputEnterNormalMode),
DEFINE_INPUTFUNC(FIELD_VOID, "EnterBerserkMode", InputEnterBerserkMode),
DEFINE_INPUTFUNC(FIELD_VOID, "EnterRetreatMode", InputEnterRetreatMode),

DEFINE_INPUTFUNC(FIELD_VOID, "SetWanderNever", InputSetWanderNever),
DEFINE_INPUTFUNC(FIELD_VOID, "SetWanderAlways", InputSetWanderAlways),
DEFINE_INPUTFUNC(FIELD_VOID, "SetWanderIdle", InputSetWanderIdle),
DEFINE_INPUTFUNC(FIELD_VOID, "SetWanderAlert", InputSetWanderAlert),

DEFINE_INPUTFUNC(FIELD_VOID, "Spawn", InputSpawnNPC),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableSpawning", InputEnableSpawning),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableSpawning", InputDisableSpawning),

END_DATADESC()

CNPC_BasePredator::CNPC_BasePredator()
{
	// By default, is not a boss
	m_bIsBoss = false;
	// Initially can threat display
	m_fCanThreatDisplay = TRUE;

	m_iszMate = NULL_STRING;

#ifdef EZ2
	m_flStartedFearingEnemy = -1;
#endif
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_BasePredator::Precache()
{
	// Use this gib particle system to any predator healing
	// TODO - do we need different particles for different species?
	// TODO - should ALL NPCs use this particle?
	//PrecacheParticleSystem("bullsquid_heal");
	//PrecacheScriptSound("npc_basepredator.heal");
	PrecacheModel("sprites/vortring1.vmt");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BasePredator::Spawn()
{
	// Almost all of the Spawn() code is handled by child classes
	SetupGlobalModelData();
	BaseClass::Spawn();
}
void CNPC_BasePredator::Activate(void)
{
	BaseClass::Activate();

	SetupGlobalModelData();

	m_hMate = gEntList.FindEntityByName(NULL, m_iszMate);
	m_iszMate = NULL_STRING;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BasePredator::OnRestore()
{
	BaseClass::OnRestore();
	SetupGlobalModelData();
}

void CNPC_BasePredator::SetupGlobalModelData()
{
	CollisionProp()->SetSurroundingBoundsType(USE_HITBOXES);
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this monster's place in the relationship table.
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_BasePredator::Classify(void)
{
	// Don't notice any other NPCs or be noticed while dormant
	if (m_bDormant)
		return CLASS_NONE;

	return CLASS_ZOMBIE;
}

//=========================================================
// IdleSound 
//=========================================================
void CNPC_BasePredator::IdleSound(void)
{
	EmitSound(UTIL_VarArgs("%s.Idle", GetSoundscriptClassname()));
}

//=========================================================
// PainSound 
//=========================================================
void CNPC_BasePredator::PainSound(const CTakeDamageInfo& info)
{
	EmitSound(UTIL_VarArgs("%s.Pain", GetSoundscriptClassname()));
}

//=========================================================
// AlertSound
//=========================================================
void CNPC_BasePredator::AlertSound(void)
{
	EmitSound(UTIL_VarArgs("%s.Alert", GetSoundscriptClassname()));
}

//=========================================================
// DeathSound
//=========================================================
void CNPC_BasePredator::DeathSound(const CTakeDamageInfo& info)
{
	EmitSound(UTIL_VarArgs("%s.Death", GetSoundscriptClassname()));
}

//=========================================================
// AttackSound
//=========================================================
void CNPC_BasePredator::AttackSound(void)
{
	EmitSound(UTIL_VarArgs("%s.Attack1", GetSoundscriptClassname()));
}

//=========================================================
// FoundEnemySound
//=========================================================
void CNPC_BasePredator::FoundEnemySound(void)
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound(UTIL_VarArgs("%s.FoundEnemy", GetSoundscriptClassname()));
		m_nextSoundTime = gpGlobals->curtime + random->RandomInt(1.5, 3.0);
	}
}

//=========================================================
// GrowlSound
//=========================================================
void CNPC_BasePredator::GrowlSound(void)
{
	if (gpGlobals->curtime >= m_nextSoundTime)
	{
		EmitSound(UTIL_VarArgs("%s.Growl", GetSoundscriptClassname()));
		m_nextSoundTime = gpGlobals->curtime + random->RandomInt(1.5, 3.0);
	}
}

//=========================================================
// BiteSound
//=========================================================
void CNPC_BasePredator::BiteSound(void)
{
	EmitSound(UTIL_VarArgs("%s.Bite", GetSoundscriptClassname()));
}

//=========================================================
// EatSound
//=========================================================
void CNPC_BasePredator::EatSound(void)
{
	EmitSound(UTIL_VarArgs("%s.Eat", GetSoundscriptClassname()));
}

//=========================================================
// BeginSpawnSound
//=========================================================
void CNPC_BasePredator::BeginSpawnSound(void)
{
	EmitSound(UTIL_VarArgs("%s.BeginSpawn", GetSoundscriptClassname()));
}

//=========================================================
// EndSpawnSound
//=========================================================
void CNPC_BasePredator::EndSpawnSound(void)
{
	EmitSound(UTIL_VarArgs("%s.EndSpawn", GetSoundscriptClassname()));
}

int CNPC_BasePredator::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
		// Replace idle stand with idle wander if applicable
	case SCHED_IDLE_STAND:
		if (m_tWanderState == WANDER_STATE_ALWAYS || m_tWanderState == WANDER_STATE_IDLE_ONLY) {
			return SCHED_PREDATOR_WANDER;
		}
		break;

		// Replace alert stand with idle wander if applicable
	case SCHED_ALERT_STAND:
		if (m_tWanderState > WANDER_STATE_NEVER && m_tWanderState < WANDER_STATE_IDLE_ONLY) {

			return SCHED_PREDATOR_WANDER;
		}
		break;
	}

	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPC_BasePredator::GatherConditions(void)
{
	// Baby creature growth conditions
	// COND_PREDATOR_CAN_GROW and COND_BABYSQUID_GROWTH_INVALID are separate conditions so that
	// COND_PREDATOR_GROWTH_INVALID can interrupt the schedule for growth if the baby creature is not
	// in a physical space that would support it.
	if (m_bIsBaby && m_iTimesFed >= 2)
	{
		SetCondition(COND_PREDATOR_CAN_GROW);

		Vector	vUpBit = GetAbsOrigin();
		vUpBit.z += 1;

		trace_t tr;
		AI_TraceHull(GetAbsOrigin(), vUpBit, Vector(-16, -16, 0), Vector(16, 16, 32), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr);
		if (tr.startsolid || (tr.fraction < 1.0))
		{
			SetCondition(COND_PREDATOR_GROWTH_INVALID);
		}
		else
		{
			ClearCondition(COND_PREDATOR_GROWTH_INVALID);
		}
	}
	else
	{
		ClearCondition(COND_PREDATOR_CAN_GROW);
	}

	BaseClass::GatherConditions();
}

//=========================================================
// Show effects when this predator regains health
// TODO Should this apply to ALL NPCs?
//=========================================================
void CNPC_BasePredator::HealEffects(void)
{
	DispatchParticleEffect("bullsquid_heal", WorldSpaceCenter(), GetAbsAngles());
	EmitSound("npc_basepredator.heal");
	RemoveAllDecals();

	CSprite* pBlastSprite = CSprite::SpriteCreate("sprites/vortring1.vmt", WorldSpaceCenter(), true);
	if (pBlastSprite != NULL)
	{
		pBlastSprite->SetTransparency(kRenderWorldGlow, 255, 255, 255, 50, kRenderFxNone);
		pBlastSprite->SetBrightness(100);
		pBlastSprite->SetScale(2.5);
		pBlastSprite->SetGlowProxySize(90.0f);
		pBlastSprite->AnimateAndDie(45);
		pBlastSprite->SetParent(this);
	}

	if (sv_predator_heal_render_effects.GetBool())
	{
		color32 color = GetRenderColor();
		SetRenderColor(color.r / 4, color.g, color.b / 4);
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_BasePredator::MaxYawSpeed(void)
{
	float flYS = 0;

	switch (GetActivity())
	{
	case	ACT_WALK:			flYS = 90;	break;
	case	ACT_RUN:			flYS = 90;	break;
	case	ACT_IDLE:			flYS = 90;	break;
	case	ACT_RANGE_ATTACK1:	flYS = 90;	break;
	default:
		flYS = 90;
		break;
	}

	return flYS;
}

//=========================================================
// Used by classes that extend BasePredator
//=========================================================
bool CNPC_BasePredator::IsJumpLegal(const Vector& startPos, const Vector& apex, const Vector& endPos, float maxUp, float maxDown, float maxDist) const
{
	return BaseClass::IsJumpLegal(startPos, apex, endPos, maxUp, maxDown, maxDist);
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_BasePredator::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	// Allow these schedules to stop for melee attacks
	// just like player companions
	if (IsCurSchedule(SCHED_RANGE_ATTACK1) ||
		IsCurSchedule(SCHED_BACK_AWAY_FROM_ENEMY) ||
		IsCurSchedule(SCHED_RUN_FROM_ENEMY)
		)
	{
		SetCustomInterruptCondition(COND_CAN_MELEE_ATTACK1);
	}

	// Like zombies, ignore damage if we're attacking.
	switch (GetActivity())
	{
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
	case ACT_RANGE_ATTACK2:
		ClearCustomInterruptCondition(COND_LIGHT_DAMAGE);
		ClearCustomInterruptCondition(COND_HEAVY_DAMAGE);
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
int CNPC_BasePredator::RangeAttack1Conditions(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// monster will fall too far behind if he stops running to spit at this distance from the enemy.
		return (COND_NONE);
	}

	if (flDist > 85 && flDist <= m_flDistTooFar && flDot >= 0.5 && gpGlobals->curtime >= m_flNextSpitTime)
	{
		if (GetEnemy() != NULL)
		{
			if (fabs(GetAbsOrigin().z - GetEnemy()->GetAbsOrigin().z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return(COND_NONE);
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->curtime + GetMaxSpitWaitTime();
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->curtime + GetMinSpitWaitTime();
		}

		return(COND_CAN_RANGE_ATTACK1);
	}

	return(COND_NONE);
}

//=========================================================
// MeleeAttack2Conditions - predators tend to be big guys, 
// so they have longer melee ranges than most monsters. 
// For bullsquids, this is the tailwhip attack.
//=========================================================
int CNPC_BasePredator::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (GetEnemy()->m_iHealth <= GetWhipDamage() && flDist <= 85 && flDot >= 0.7)
	{
		return (COND_CAN_MELEE_ATTACK1);
	}

	return(COND_NONE);
}

//=========================================================
// MeleeAttack2Conditions - predators tend to be big guys, 
// so they have longer melee ranges than most monsters. 
// For bullsquids, this is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
int CNPC_BasePredator::MeleeAttack2Conditions(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7 && !HasCondition(COND_CAN_MELEE_ATTACK1))		// The player & predator (bullsquid) can be as much as their bboxes 
		return (COND_CAN_MELEE_ATTACK2);

	return(COND_NONE);
}

bool CNPC_BasePredator::FValidateHintType(CAI_Hint* pHint)
{
	// TODO Does this hint do anything for bullsquids and other predators?
	if (pHint->HintType() == HINT_HL1_WORLD_HUMAN_BLOOD)
		return true;

	/*if (pHint->HintType() == HINT_BEAST_HOME || pHint->HintType() == HINT_BEAST_FRUSTRATION)
		return true;*/

	DevMsg("Couldn't validate hint type");

	return false;
}

//=========================================================
// Event_Killed - override death to handle boss regen
//		1upD
//=========================================================
void CNPC_BasePredator::Event_Killed(const CTakeDamageInfo& info)
{
	// If we're dead, return to normal render color
	if (sv_predator_heal_render_effects.GetBool())
	{
		SetRenderColor(255, 255, 255);
	}
	BaseClass::Event_Killed( info );
	/*if (IsBoss()) {
		// Reset to max health
		this->m_iHealth = this->m_iMaxHealth;
		DevMsg("Boss health reset! \n");
		m_OnBossHealthReset.FireOutput(info.GetAttacker(), this);
	}
	else {
	}*/
}

void CNPC_BasePredator::RemoveIgnoredConditions(void)
{
	if (m_flHungryTime > gpGlobals->curtime)
		ClearCondition(COND_PREDATOR_SMELL_FOOD);

	if (gpGlobals->curtime - m_flLastHurtTime <= 20)
	{
		// haven't been hurt in 20 seconds, so let the monster care about stink. 
		ClearCondition(COND_SMELL);
	}

	if (GetEnemy() != NULL)
	{
		// ( Unless after tasty prey, yumm ^_^ )
		if (IsPrey(GetEnemy()))
			ClearCondition(COND_SMELL);
	}
}

Disposition_t CNPC_BasePredator::IRelationType(CBaseEntity* pTarget)
{
	Disposition_t disposition = BaseClass::IRelationType(pTarget);;

	if (ShouldInfight(pTarget))
	{
		disposition = D_HT;
	}

	// if predator fears or hates this enemy and has been hurt in the last five seconds...
	if (disposition <= D_FR && gpGlobals->curtime - m_flLastHurtTime < 5)
	{
		if (IsPrey(pTarget))
		{
			// if predator has been hurt in the last 5 seconds, and is getting relationship for a prey creature, 
			// tell pred to disregard prey. 
			PredMsg("Predator %s disregarding prey because of injury.\n");
			return D_NU;
		}
		else if (!IsBoss() && m_iHealth < m_iMaxHealth / 3.0f && HasFearResponse())
		{
#ifdef EZ2
			if (m_flStartedFearingEnemy == -1)
				m_flStartedFearingEnemy = gpGlobals->curtime;
#endif

			// if hurt in the last five seconds and very low on health, retreat from the current enemy
			PredMsg("Predator %s retreating because of injury.\n");
			return D_FR;
		}
	}
	return disposition;
}

#ifdef EZ2
// 
// Blixibon - Needed so the player's speech AI doesn't pick this up as D_FR before it's apparent (e.g. fast, rapid kills)
// 
bool CNPC_BasePredator::JustStartedFearing(CBaseEntity* pTarget)
{
	Assert(IRelationType(pTarget) == D_FR);

	// If we've only been afraid for 1.25 seconds, the player probably doesn't realize
	if (gpGlobals->curtime - m_flStartedFearingEnemy < 1.25f)
		return true;

	return false;
}
#endif

//=========================================================
// TakeDamage - overridden for predators so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CNPC_BasePredator::OnTakeDamage_Alive(const CTakeDamageInfo& inputInfo)
{
	// No longer dormant
	m_bDormant = false;

	if (m_tBossState == BOSS_STATE_BERSERK) {
		// Berserk predators take no damage!
		return 0;
	}

	if (!IsPrey(inputInfo.GetAttacker()))
	{
		// don't forget about prey if it was prey that hurt the predator.
		m_flLastHurtTime = gpGlobals->curtime;
	}

	// Don't take damage from zombie goo
	else if (inputInfo.GetInflictor() && inputInfo.GetInflictor()->m_iClassname == gm_iszGooPuddle)
	{
		return 0;
	}

	// I'm hungry again!
	m_flHungryTime = gpGlobals->curtime;

	// if attacked by another predator of the same species, immediately aggro them	
	if (ShouldInfight(inputInfo.GetAttacker()))
	{
		SetEnemy(inputInfo.GetAttacker()->MyNPCPointer());
	}

	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

//=========================================================
// OnFed - Handles eating food and regenerating health
//	Fires the output m_OnFed
//=========================================================
void CNPC_BasePredator::OnFed()
{
	// 1upD - Eating restores predators to full health
	int maxHealth = GetMaxHealth();
	if (m_iHealth < maxHealth)
	{
		HealEffects();
		m_iHealth = maxHealth;
	}

	// Increment feeding counter
	m_iTimesFed++;

	// Fire output
	m_OnFed.FireOutput(this, this);

	// Damage props underneath this predator
	EatAttack();
}

//-----------------------------------------------------------------------------
// Deal damage to props / ragdolls / gibs while eating
//-----------------------------------------------------------------------------
void CNPC_BasePredator::EatAttack()
{
	// TODO This functionality needs to be rewritten for a few reasons.
	// It should be called from an animation event rather than upon restoring health
	// so that certain predators like the zassassin deal damage at the right time.
	// It should damage props but not other NPCs.
	//
	// For now, this will do.
	CTakeDamageInfo info = CTakeDamageInfo(this, this, 100.0f, DMG_SLASH | DMG_ALWAYSGIB);
	Vector vBitePos;

	GetAttachment("Mouth", vBitePos);
	RadiusDamage(info, vBitePos, 64.0f, Classify(), this);
}

//-----------------------------------------------------------------------------
// Should this NPC always think?
//-----------------------------------------------------------------------------
bool CNPC_BasePredator::ShouldAlwaysThink()
{
	return IsBoss() || BaseClass::ShouldAlwaysThink();
}

//=========================================================
// Should this predator seek out a mate?
//=========================================================
bool CNPC_BasePredator::ShouldFindMate()
{
	return !m_bIsBaby && m_iTimesFed > 0 && m_iHealth == m_iMaxHealth;
}

//=========================================================
// Can this predator mate with the target NPC?
//=========================================================
bool CNPC_BasePredator::CanMateWithTarget(CNPC_BasePredator* pTarget, bool receiving)
{
	return pTarget && (receiving || pTarget->m_bSpawningEnabled) && !pTarget->m_bIsBaby && pTarget->m_iTimesFed > 0;
}

//=========================================================
// Should this predator eat while enemies are present?
//=========================================================
bool CNPC_BasePredator::ShouldEatInCombat()
{
	// Do we need health?
	if (m_iHealth >= (m_iMaxHealth * GetEatInCombatPercentHealth()))
		return false;

	// Do we smell food?
	if (!HasCondition(COND_PREDATOR_SMELL_FOOD))
		return false;

	// Are we fighting with a rival over food?
	if (IsSameSpecies(GetEnemy()))
		return false;

	// Can we acquire a squad slot for this?
	if (IsInSquad() && !OccupyStrategySlot(SQUAD_SLOT_FEED))
		return false;

	return true;
}

//=========================================================
// Thinking, including core thinking, movement, animation
// Overridden to handle healing effects
//=========================================================
void CNPC_BasePredator::NPCThink(void)
{
	// Return to 255, 255, 255 after healing
	if (sv_predator_heal_render_effects.GetBool())
	{
		color32 color = GetRenderColor();
		SetRenderColor(MIN(color.r + 8, 255), color.g, MIN(color.b + 8, 255));
	}

	BaseClass::NPCThink();
}

//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards.
// Predatory monsters are interested in sounds and scents.
//=========================================================
int CNPC_BasePredator::GetSoundInterests(void)
{
	BaseClass::GetSoundInterests();
	return	SOUND_WORLD |
		SOUND_COMBAT |
		SOUND_BULLET_IMPACT | // Should investigate bullet impact sounds
		SOUND_CARCASS |
		SOUND_MEAT |
		SOUND_GARBAGE |
		SOUND_PLAYER_VEHICLE | // Should investigate vehicle sounds
		SOUND_PLAYER;
}

//=========================================================
// OnListened - monsters dig through the active sound list for
// any sounds that may interest them. (smells, too!)
//=========================================================
void CNPC_BasePredator::OnListened(void)
{
	AISoundIter_t iter;

	CSound* pCurrentSound;

	static int conditionsToClear[] =
	{
		COND_PREDATOR_SMELL_FOOD,
	};

	ClearConditions(conditionsToClear, ARRAYSIZE(conditionsToClear));

	pCurrentSound = GetSenses()->GetFirstHeardSound(&iter);

	while (pCurrentSound)
	{
		// the npc cares about this sound, and it's close enough to hear.
		int condition = COND_NONE;

		if (!pCurrentSound->FIsSound())
		{
			// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
			if (pCurrentSound->IsSoundType(SOUND_MEAT | SOUND_CARCASS))
			{
				// the detected scent is a food item
				condition = COND_PREDATOR_SMELL_FOOD;
			}
		}

		if (condition != COND_NONE)
			SetCondition(condition);

		pCurrentSound = GetSenses()->GetNextHeardSound(&iter);
	}

	BaseClass::OnListened();
}

bool CNPC_BasePredator::QueryHearSound(CSound* pSound)
{
	// Don't smell while dormant
	if (pSound->FIsScent() && m_bDormant)
		return false;

	// While dormant, only hear sounds within a laterial distance of 384 units
	if (m_bDormant && GetAbsOrigin().AsVector2D().DistTo(pSound->GetSoundOrigin().AsVector2D()) > 384)
		return false;

	return BaseClass::QueryHearSound(pSound);
}

//========================================================
// RunAI - overridden because there are things
// that need to be checked every think.
//========================================================
void CNPC_BasePredator::RunAI(void)
{
	// first, do base class stuff
	BaseClass::RunAI();

	if (GetEnemy() != NULL && GetActivity() == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((GetAbsOrigin() - GetEnemy()->GetAbsOrigin()).Length2D() < GetSprintDistance())
		{
			m_flPlaybackRate = 1.25;
		}
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool CNPC_BasePredator::FInViewCone(Vector pOrigin)
{
	Vector los = (pOrigin - GetAbsOrigin());

	// do this in 2D
	los.z = 0;
	VectorNormalize(los);

	Vector facingDir = EyeDirection2D();

	float flDot = DotProduct(los, facingDir);

	if (flDot > m_flFieldOfView)
		return true;

	return false;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  
// OVERRIDDEN for custom predator tasks
//=========================================================
void CNPC_BasePredator::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_PLAY_EXCITE_ACT:
	{
		SetIdealActivity((Activity)ACT_EXCITED);
		break;
	}
	case TASK_PREDATOR_PLAY_EAT_ACT:
		EatSound();
#ifdef EZ2
		if (CNPC_Wilson::GetWilson())
		{
			float flDist = 4096.0f;
			CNPC_Wilson* pWilson = CNPC_Wilson::GetBestWilson(flDist, &GetAbsOrigin());
			CSound* pScent = GetBestScent();
			if (pScent && pWilson && pWilson->FInViewCone(this) && pWilson->FVisible(this))
			{
				pWilson->SetSpeechTarget(pScent->m_hOwner);
				pWilson->SpeakIfAllowed(TLK_WITNESS_EAT);
			}
		}
#endif
		SetIdealActivity((Activity)ACT_EAT);
		break;
	case TASK_PREDATOR_PLAY_SNIFF_ACT:
		SetIdealActivity((Activity)ACT_DETECT_SCENT);
		break;
	case TASK_PREDATOR_PLAY_INSPECT_ACT:
		SetIdealActivity((Activity)ACT_INSPECT_FLOOR);
		break;
	case TASK_MELEE_ATTACK2:
	{
		if (GetEnemy())
		{
			GrowlSound();

			m_flLastAttackTime = gpGlobals->curtime;

			BaseClass::StartTask(pTask);
		}
		break;
	}
	case TASK_PREDATOR_HOPTURN:
	{
		SetActivity(ACT_HOP);

		if (GetEnemy())
		{
			Vector	vecFacing = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
			VectorNormalize(vecFacing);

			GetMotor()->SetIdealYaw(vecFacing);
		}

		break;
	}
	case TASK_PREDATOR_EAT:
	{
		m_flHungryTime = gpGlobals->curtime + pTask->flTaskData;
		TaskComplete();
		break;
	}

	case TASK_PREDATOR_REGEN:
	{
		OnFed();
		TaskComplete();
		break;
	}
	case TASK_PREDATOR_SPAWN_SOUND:
		BeginSpawnSound();
		TaskComplete();
		break;
		// If the child class has no defined spawn task, just complete the task
		// here in the base class.
	case TASK_PREDATOR_SPAWN:
		PredMsg("Warning: %s does not have spawn behavior.\n");
		m_bReadyToSpawn = false;
		TaskComplete();
		break;
	default:
	{
		BaseClass::StartTask(pTask);
		break;
	}
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_BasePredator::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PREDATOR_HOPTURN:
	{
		if (GetEnemy())
		{
			Vector	vecFacing = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
			VectorNormalize(vecFacing);
			GetMotor()->SetIdealYaw(vecFacing);
		}

		if (IsSequenceFinished())
		{
			TaskComplete();
		}
		break;
	}
	case TASK_PREDATOR_PLAY_EXCITE_ACT:
	case TASK_PREDATOR_PLAY_EAT_ACT:
	case TASK_PREDATOR_PLAY_SNIFF_ACT:
	case TASK_PREDATOR_PLAY_INSPECT_ACT:
	{
		AutoMovement();
		if (IsActivityFinished())
		{
			TaskComplete();
		}
		break;
	}
	case TASK_PREDATOR_SPAWN:
	case TASK_PREDATOR_GROW:
	{
		// If we fall in this case, end the task when the activity ends
		if (IsActivityFinished())
		{
			TaskComplete();
		}
		break;
	}
	default:
	{
		BaseClass::RunTask(pTask);
		break;
	}
	}
}

int CNPC_BasePredator::SelectBossSchedule(void)
{
	// Handle new Boss state before doing anything else
	if (HasCondition(COND_NEW_BOSS_STATE)) {
		ClearCondition(COND_NEW_BOSS_STATE);
		// TODO add a scream sound here
		return SCHED_PREDATOR_HURTHOP;
	}

	switch (m_tBossState) {
	case BOSS_STATE_NORMAL:
		return SCHED_NONE;
		break;
	case BOSS_STATE_RETREAT:
		return SCHED_RUN_FROM_ENEMY; // Don't worry about minimum distance if the enemy can't see me
		break;
	case BOSS_STATE_BERSERK:
	default:
		return SCHED_NONE;
	}
	return SCHED_NONE;
}

//=========================================================
// GetSchedule 
//=========================================================
int CNPC_BasePredator::SelectSchedule(void)
{
	int bossSchedule = SelectBossSchedule();
	if (bossSchedule != SCHED_NONE)
		return bossSchedule;

	if (m_bReadyToSpawn && gpGlobals->curtime > m_flNextSpawnTime)
	{
		return SCHED_PREDATOR_SPAWN;
	}

	switch (m_NPCState)
	{
	case NPC_STATE_ALERT:
	{
		// No longer dormant
		m_bDormant = false;

		if (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			return SCHED_PREDATOR_HURTHOP;
		}
	}
	// 1upD - Fall through to idle. Idle predators should be allowed to eat.
	case NPC_STATE_IDLE:
	{
		if (HasCondition(COND_PROVOKED))
			break;

		if (HasCondition(COND_PREDATOR_SMELL_FOOD))
		{
			CSound* pSound;

			pSound = GetBestScent();

			if (pSound && (!FInViewCone(pSound->GetSoundOrigin()) || !FVisible(pSound->GetSoundOrigin())))
			{
				// scent is behind or occluded
				return SCHED_PRED_SNIFF_AND_EAT;
			}

			// food is right out in the open. Just go get it.
			return SCHED_PREDATOR_EAT;
		}

		// If the predator smells something and doesn't hear any danger
		if (HasCondition(COND_SMELL) && !HasCondition(COND_HEAR_COMBAT) && !HasCondition(COND_HEAR_DANGER) && !HasCondition(COND_HEAR_BULLET_IMPACT))
		{
			// there's something stinky. 
			CSound* pSound;

			pSound = GetBestScent();
			if (pSound)
				return SCHED_PREDATOR_WALLOW;
		}

		break;
	}
	case NPC_STATE_COMBAT:
	{
		// dead enemy
		if (HasCondition(COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return BaseClass::SelectSchedule();
		}

		if (HasCondition(COND_NEW_ENEMY))
		{
			FoundEnemySound(); // 1upD - Scream at the enemy!
			if (m_fCanThreatDisplay && IRelationType(GetEnemy()) == D_HT && IsPrey(GetEnemy()) && (!IsInSquad() || OccupyStrategySlot(SQUAD_SLOT_THREAT_DISPLAY)))
			{
				// this means predator sees prey!
				m_fCanThreatDisplay = FALSE;// only do the dance once per lifetime.
				return SCHED_PREDATOR_SEE_PREY;
			}
			else
			{
				return SCHED_WAKE_ANGRY;
			}
		}

		if (HasCondition(COND_CAN_RANGE_ATTACK1) && (!IsInSquad() || OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2)))
		{
			return SCHED_RANGE_ATTACK1;
		}

		if (HasCondition(COND_CAN_MELEE_ATTACK1))
		{
			return SCHED_MELEE_ATTACK1;
		}

		if (HasCondition(COND_CAN_MELEE_ATTACK2))
		{
			return SCHED_MELEE_ATTACK2;
		}

		// If a predator is below maximum health, smells food, is not targetting a fellow predator, and has the squad slot (if applicable)
		// it may eat in combat
		if (ShouldEatInCombat())
		{
			// Don't bother sniffing food while in combat
			return SCHED_PREDATOR_RUN_EAT;
		}

		// Scared predators should run away instead of chasing
		if ((IsInSquad() && !OccupyStrategySlotRange(SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2)) || IRelationType(GetEnemy()) == D_FR)
		{
			return SCHED_CHASE_ENEMY_FAILED;
		}

		return SCHED_CHASE_ENEMY;

		break;
	}
	}

	// We're returning to the base class here anyway, so don't bother with an if statement
	// unless this is moved farther up so it could override schedules
	BehaviorSelectSchedule();

	return BaseClass::SelectSchedule();
}

int CNPC_BasePredator::SelectFailSchedule(int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode)
{
	// Need special case handling for when predators smell food they can't navigate too
	switch (failedSchedule)
	{
	case SCHED_PREDATOR_EAT:
	case SCHED_PREDATOR_RUN_EAT:
	case SCHED_PRED_SNIFF_AND_EAT:
	case SCHED_PREDATOR_SPAWN:
	case SCHED_PREDATOR_WALLOW:
		ClearCondition(COND_SMELL);
		ClearCondition(COND_PREDATOR_SMELL_FOOD);
		// Retry schedule selection with conditions removed
		return SelectSchedule();
		// fall through
	default:
		return BaseClass::SelectFailSchedule(failedSchedule, failedTask, taskFailCode);
	}
}

//=========================================================
// GetIdealState - Overridden for predators to deal with
// the feature that makes them lose interest in prey for 
// a while if something injures it. 
//=========================================================
NPC_STATE CNPC_BasePredator::SelectIdealState(void)
{
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch (m_NPCState)
	{
	case NPC_STATE_COMBAT:
	{
		// COMBAT goes to ALERT upon death of enemy
		if (GetEnemy() != NULL && (HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE)) && IsPrey(GetEnemy()))
		{
			// if the predator has a prey enemy and something hurts it, it's going to forget about the prey for a while.
			SetEnemy(NULL);
			return NPC_STATE_ALERT;
		}
		break;
	}
	}

	return BaseClass::SelectIdealState();
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
bool CNPC_BasePredator::FVisible(Vector vecOrigin)
{
	trace_t tr;
	Vector		vecLookerOrigin;

	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, MASK_BLOCKLOS, this/*pentIgnore*/, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction != 1.0)
		return false; // Line of sight is not established
	else
		return true;// line of sight is valid.
}

// Declared in npc_playercompanion.cpp
extern CBaseEntity* OverrideMoveCache_FindTargetsInRadius(CBaseEntity* pFirstEntity, const Vector& vecOrigin, float flRadius);
const float AVOID_TEST_DIST = 18.0f * 12.0f;

#define PREDATOR_AVOID_ENTITY_FLAME_RADIUS	18.0f

//-----------------------------------------------------------------------------
// Purpose -	Allows predators to avoid zombie goo and fires.
//				Blixibon
//-----------------------------------------------------------------------------
bool CNPC_BasePredator::OverrideMove(float flInterval)
{
	bool overrode = BaseClass::OverrideMove(flInterval);

	if (!overrode && GetNavigator()->GetGoalType() != GOALTYPE_NONE)
	{
		string_t iszEnvFire = AllocPooledString("env_fire");
		string_t iszEntityFlame = AllocPooledString("entityflame");
		string_t iszZombieGoo = gm_iszGooPuddle;

		CBaseEntity* pEntity = NULL;
		trace_t tr;

		// For each possible entity, compare our known interesting classnames to its classname, via ID
		while ((pEntity = OverrideMoveCache_FindTargetsInRadius(pEntity, GetAbsOrigin(), AVOID_TEST_DIST)) != NULL)
		{
			// Handle each type
			if (pEntity->m_iClassname == iszEnvFire)
			{
				Vector vMins, vMaxs;
				if (FireSystem_GetFireDamageDimensions(pEntity, &vMins, &vMaxs))
				{
					UTIL_TraceLine(WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_FIRE_SOLID, pEntity, COLLISION_GROUP_NONE, &tr);
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle(pEntity->GetAbsOrigin(), ((vMaxs.x - vMins.x) * 1.414 * 0.5) + 6.0, AIMST_AVOID_DANGER);
					}
				}
			}
			else if (pEntity->m_iClassname == iszEntityFlame && pEntity->GetParent() && !pEntity->GetParent()->IsNPC())
			{
				float flDist = pEntity->WorldSpaceCenter().DistTo(WorldSpaceCenter());

				if (flDist > PREDATOR_AVOID_ENTITY_FLAME_RADIUS)
				{
					// If I'm not in the flame, prevent me from getting close to it.
					// If I AM in the flame, avoid placing an obstacle until the flame frightens me away from itself.
					UTIL_TraceLine(WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_BLOCKLOS, pEntity, COLLISION_GROUP_NONE, &tr);
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle(pEntity->WorldSpaceCenter(), PREDATOR_AVOID_ENTITY_FLAME_RADIUS, AIMST_AVOID_OBJECT);
					}
				}
			}
			else if (pEntity->m_iClassname == iszZombieGoo)
			{
				float flDist = pEntity->WorldSpaceCenter().DistTo(WorldSpaceCenter());
				if (flDist > 50.0f)
				{
					// If I'm not in the goo, prevent me from getting close to it.
					// If I AM in the goo, avoid placing an obstacle until the goo frightens me away from itself.
					UTIL_TraceLine(WorldSpaceCenter(), pEntity->WorldSpaceCenter(), MASK_BLOCKLOS, pEntity, COLLISION_GROUP_NONE, &tr);
					if (tr.fraction == 1.0 && !tr.startsolid)
					{
						GetLocalNavigator()->AddObstacle(pEntity->WorldSpaceCenter(), 50.0f, AIMST_AVOID_OBJECT);
					}
				}
			}
		}
	}

	return overrode;
}

#ifdef EZ2
extern int g_interactionXenGrenadeCreate;
extern int g_interactionBadCopKick;
//-----------------------------------------------------------------------------
// Purpose:  
// Input  :  The type of interaction, extra info pointer, and who started it
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_BasePredator::HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionXenGrenadeCreate)
	{
		InputSetWanderAlways(inputdata_t());
		InputEnableSpawning(inputdata_t());
		if (!m_bIsBaby)
		{
			// Xen predators come into the world ready to spawn.
			// Ready for the infestation?
			SetReadyToSpawn(true);
			SetNextSpawnTime(gpGlobals->curtime + 15.0f); // 15 seconds to first spawn
			SetHungryTime(gpGlobals->curtime); // Be ready to eat as soon as we come through
			SetTimesFed(2); // Twins!
		}
	}

	// YEET
	if (interactionType == g_interactionBadCopKick && m_bIsBaby)
	{
		KickInfo_t* pInfo = static_cast<KickInfo_t*>(data);
		CTakeDamageInfo* pDamageInfo = pInfo->dmgInfo;
		pDamageInfo->SetDamage(GetMaxHealth()); // Instant-kill
		pDamageInfo->ScaleDamageForce(100);
		ApplyAbsVelocityImpulse((pInfo->tr->endpos - pInfo->tr->startpos) * 100);
		DispatchTraceAttack(*pDamageInfo, pDamageInfo->GetDamageForce(), pInfo->tr);

		// Already handled the kick
		return true;
	}

	return BaseClass::HandleInteraction(interactionType, data, sourceEnt);
}
#endif


//------------------------------------------------------------------------------
//
// Inputs
//
//------------------------------------------------------------------------------
void CNPC_BasePredator::InputEnterNormalMode(inputdata_t& inputdata)
{
	if (m_tBossState != BOSS_STATE_NORMAL)
	{
		// Set the state to normal
		m_tBossState = BOSS_STATE_NORMAL;

		// Add removed capabilities
		CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK1);
		// Set a condition to play an animation
		SetCondition(COND_NEW_BOSS_STATE);
	}
}

void CNPC_BasePredator::InputEnterRetreatMode(inputdata_t& inputdata)
{
	if (m_tBossState != BOSS_STATE_RETREAT)
	{
		// Play appropriate sound script
		RetreatModeSound();
		// Remove ranged attack
		CapabilitiesRemove(bits_CAP_INNATE_RANGE_ATTACK1);
		// Set the state to normal
		m_tBossState = BOSS_STATE_RETREAT;
		// Set a condition to play an animation
		SetCondition(COND_NEW_BOSS_STATE);
	}
}

void CNPC_BasePredator::InputEnterBerserkMode(inputdata_t& inputdata)
{
	if (m_tBossState != BOSS_STATE_BERSERK)
	{
		// Play appropriate sound script
		BerserkModeSound();
		// Remove ranged attack
		CapabilitiesRemove(bits_CAP_INNATE_RANGE_ATTACK1);
		// Set the state to normal
		m_tBossState = BOSS_STATE_BERSERK;
		// Set a condition to play an animation
		SetCondition(COND_NEW_BOSS_STATE);
	}
}

void CNPC_BasePredator::InputSetWanderNever(inputdata_t& inputdata)
{
	m_tWanderState = WANDER_STATE_NEVER;
}

void CNPC_BasePredator::InputSetWanderAlways(inputdata_t& inputdata)
{
	m_tWanderState = WANDER_STATE_ALWAYS;
}

void CNPC_BasePredator::InputSetWanderIdle(inputdata_t& inputdata)
{
	m_tWanderState = WANDER_STATE_IDLE_ONLY;
}

void CNPC_BasePredator::InputSetWanderAlert(inputdata_t& inputdata)
{
	m_tWanderState = WANDER_STATE_ALERT_ONLY;
}

void CNPC_BasePredator::InputSpawnNPC(inputdata_t& inputdata)
{
	m_bReadyToSpawn = true;
}

void CNPC_BasePredator::InputEnableSpawning(inputdata_t& inputdata)
{
	m_bSpawningEnabled = true;
}

void CNPC_BasePredator::InputDisableSpawning(inputdata_t& inputdata)
{
	m_bSpawningEnabled = false;
}

//------------------------------------------------------------------------------
//
// Debug
//
//------------------------------------------------------------------------------
void CNPC_BasePredator::PredMsg(const tchar* pMsg)
{
	if (ai_debug_predator.GetBool() && m_flNextPredMsgTime <= gpGlobals->curtime) {
		DevMsg(pMsg, GetDebugName());
		m_flNextPredMsgTime = gpGlobals->curtime + 0.25f;
	}
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_predator, CNPC_BasePredator)

DECLARE_TASK(TASK_PREDATOR_HOPTURN)
DECLARE_TASK(TASK_PREDATOR_EAT)
DECLARE_TASK(TASK_PREDATOR_REGEN)
DECLARE_TASK(TASK_PREDATOR_PLAY_EXCITE_ACT)
DECLARE_TASK(TASK_PREDATOR_PLAY_EAT_ACT)
DECLARE_TASK(TASK_PREDATOR_PLAY_SNIFF_ACT)
DECLARE_TASK(TASK_PREDATOR_PLAY_INSPECT_ACT)
DECLARE_TASK(TASK_PREDATOR_SPAWN)
DECLARE_TASK(TASK_PREDATOR_SPAWN_SOUND)
DECLARE_TASK(TASK_PREDATOR_GROW)

DECLARE_CONDITION(COND_PREDATOR_SMELL_FOOD)
DECLARE_CONDITION(COND_NEW_BOSS_STATE)
DECLARE_CONDITION(COND_PREDATOR_CAN_GROW)
DECLARE_CONDITION(COND_PREDATOR_GROWTH_INVALID)

DECLARE_SQUADSLOT(SQUAD_SLOT_FEED)
DECLARE_SQUADSLOT(SQUAD_SLOT_THREAT_DISPLAY)

DECLARE_ACTIVITY(ACT_EAT)
DECLARE_ACTIVITY(ACT_EXCITED)
DECLARE_ACTIVITY(ACT_DETECT_SCENT)
DECLARE_ACTIVITY(ACT_INSPECT_FLOOR)

//=========================================================
// > SCHED_PREDATOR_HURTHOP
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_HURTHOP,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SOUND_WAKE				0"
	"		TASK_PREDATOR_HOPTURN		0"
	"		TASK_FACE_ENEMY				0"
	"	"
	"	Interrupts"
)

//=========================================================
// > SCHED_PREDATOR_SEE_PREY
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_SEE_PREY,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_SOUND_WAKE					0"
	"		TASK_PREDATOR_PLAY_EXCITE_ACT	0"
	"		TASK_FACE_ENEMY					0"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
)

//=========================================================
// > SCHED_PREDATOR_EAT
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_EAT,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_PREDATOR_EAT					10"
	"		TASK_STORE_LASTPOSITION				0"
	"		TASK_GET_PATH_TO_BESTSCENT			0"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_EAT					30"
	"		TASK_PREDATOR_REGEN					0"
	"		TASK_GET_PATH_TO_LASTPOSITION		0"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_CLEAR_LASTPOSITION				0"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
)

//=========================================================
// > SCHED_PREDATOR_RUN_EAT
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_RUN_EAT,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_PREDATOR_EAT					10"
	"		TASK_GET_PATH_TO_BESTSCENT			0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	//			"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	//			"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_EAT					30"
	"		TASK_PREDATOR_REGEN					0"
	"	"
	"	Interrupts"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
)

//=========================================================
// > SCHED_PRED_SNIFF_AND_EAT
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PRED_SNIFF_AND_EAT,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_PREDATOR_EAT					10"
	"		TASK_PREDATOR_PLAY_SNIFF_ACT		0"
	"		TASK_STORE_LASTPOSITION				0"
	"		TASK_GET_PATH_TO_BESTSCENT			0"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_PLAY_EAT_ACT			0"
	"		TASK_PREDATOR_EAT					30"
	"		TASK_PREDATOR_REGEN					0"
	"		TASK_GET_PATH_TO_LASTPOSITION		0"
	"		TASK_WALK_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	"		TASK_CLEAR_LASTPOSITION				0"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NEW_ENEMY"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
)

//=========================================================
// > SCHED_PREDATOR_WALLOW
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_WALLOW,

	"	Tasks"
	"		TASK_STOP_MOVING				0"
	"		TASK_STORE_LASTPOSITION			0"
	"		TASK_GET_PATH_TO_BESTSCENT		0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_PREDATOR_PLAY_INSPECT_ACT	0"
	"		TASK_GET_PATH_TO_LASTPOSITION	0"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"		TASK_CLEAR_LASTPOSITION			0"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NEW_ENEMY"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_BULLET_IMPACT"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
)

//=========================================================
// > SCHED_PREDATOR_WANDER
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_WANDER,

	"	Tasks"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	200"
	"		TASK_WALK_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_CAN_RANGE_ATTACK1 "
	"		COND_CAN_RANGE_ATTACK2 "
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_HEAR_BULLET_IMPACT"
	"		COND_HEAR_WORLD"
	"		COND_SMELL"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PREDATOR_SMELL_FOOD"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_NEW_BOSS_STATE"
)

//=========================================================
// > SCHED_PREDATOR_SPAWN
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_SPAWN,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SOUND_WAKE				0"
	"		TASK_PREDATOR_SPAWN_SOUND	0"
	"		TASK_PREDATOR_SPAWN			0"
	"	"
	"	Interrupts"
)

//=========================================================
// > SCHED_PREDATOR_GROW
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_PREDATOR_GROW,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_PREDATOR_EAT					10"
	"		TASK_PREDATOR_PLAY_SNIFF_ACT		0"
	"		TASK_PREDATOR_GROW					0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
	"		TASK_WAIT							1"
	"	"
	"	Interrupts"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_CAN_MELEE_ATTACK1"
	"		COND_CAN_MELEE_ATTACK2"
	"       COND_PREDATOR_GROWTH_INVALID"
)

AI_END_CUSTOM_NPC()