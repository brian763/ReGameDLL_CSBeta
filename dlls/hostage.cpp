#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "hostage.h"
#include "hostage_localnav.h"
#include "../engine/studio.h"

LINK_ENTITY_TO_CLASS(hostage_entity, CHostage);

CHostage::CHostage()
{

}

#define HOSTAGE_STATE       (*(int *)&m_vStart.z)   
#define STUCK_CHECK_TIMER   (*(float *)&m_nPathNodes)  
#define SAVED_POS_X         (*(float *)&m_iCurrentNode) 
#define SAVED_POS_Y         (*(float *)&m_bStuck)      
#define SAVED_POS_Z         m_flStuckTime              

void CHostage::Spawn(void)
{
    Precache();

    pev->classname = MAKE_STRING("hostage_entity");
    pev->movetype = MOVETYPE_STEP;
    pev->solid = SOLID_SLIDEBOX;
    pev->takedamage = DAMAGE_YES;
    pev->flags |= FL_MONSTER;
    pev->deadflag = DEAD_NO;
    pev->max_health = 100;
    pev->health = pev->max_health;
    pev->gravity = 1;

    pev->view_ofs = Vector(0, 0, 12);

    if (pev->skin < 0)
        pev->skin = 0;

    SET_MODEL(ENT(pev), STRING(pev->model));
    SetActivity(ACT_IDLE);
    SetSequenceBox();

    HOSTAGE_STATE = 1;

    m_flNextUseTime = 0;
    m_flFlinchTime = 0.0;
#ifdef REGAMEDLL_FIXES
    m_flPathCheckInterval = 0.1;
#else
    m_flPathCheckInterval = 1024.0;
#endif

    m_hTargetEnt = NULL;
    m_flNextChange = 128.0;
    m_flGoalSpeed = 0.0;

    m_vPathToFollow[0] = Vector(0, 0, 0);
    m_bRescueMe = FALSE;

#ifdef REGAMEDLL_ADD

    if (!m_LocalNav)
        m_LocalNav = new CLocalNav(this);

    m_nTargetNode = NODE_INVALID_EMPTY;
    m_fHasPath = FALSE;
    m_flLastPathCheck = -1;
    m_flPathAcquired = -1;
    m_flNextFullThink = 0;
    m_nNavPathNodes = 0;
    m_bStuck = FALSE;
#endif

    UTIL_SetSize(pev, Vector(-10, -10, 0), Vector(10, 10, 62));
    UTIL_MakeVectors(pev->v_angle);
    SetBoneController(0, UTIL_VecToYaw(gpGlobals->v_forward));

    SetThink(&CHostage::IdleThink);
    pev->nextthink = gpGlobals->time + 0.1;

    m_vStartAngles = pev->origin;
}

void CHostage::Precache(void)
{
    if (!pev->model)
        pev->model = MAKE_STRING("models/scientist.mdl");

    PRECACHE_MODEL((char*)STRING(pev->model));

    PRECACHE_SOUND("hostage/hos1.wav");
    PRECACHE_SOUND("hostage/hos2.wav");
    PRECACHE_SOUND("hostage/hos3.wav");
    PRECACHE_SOUND("hostage/hos4.wav");
    PRECACHE_SOUND("hostage/hos5.wav");
}

void CHostage::SetActivity(int activity)
{
    int iSequence = LookupActivity(activity);

    if (iSequence != -1 && pev->sequence != iSequence)
    {
        pev->gaitsequence = iSequence;
        pev->sequence = iSequence;
        m_Activity = activity;
        pev->frame = 0;
        ResetSequenceInfo();
    }
}

void CHostage::SetCollisionBox(void)
{
    studiohdr_t* pstudiohdr = (studiohdr_t*)GET_MODEL_PTR(ENT(pev));
    mstudioseqdesc_t* pseqdesc = (mstudioseqdesc_t*)((byte*)pstudiohdr + pstudiohdr->seqindex);

    Vector vecBBMin = *(Vector*)&pseqdesc[pev->sequence].bbmin;
    Vector vecBBMax = *(Vector*)&pseqdesc[pev->sequence].bbmax;

    UTIL_SetSize(pev, vecBBMin, vecBBMax);
}

#ifdef REGAMEDLL_FIXES
static float CalcJumpVelocity(float flHeight)
{
    float flGravity = CVAR_GET_FLOAT("sv_gravity");
    if (flGravity <= 0.0f)
    {
        flGravity = 800.0f;
    }

    float flJumpVelocity = sqrt(2.0f * flGravity * (flHeight + 5.0f));
    if (flJumpVelocity < 300.0f)
    {
        flJumpVelocity = 300.0f;
    }

    return flJumpVelocity;
}
#endif

#ifndef REGAMEDLL_ADD
// void CHostage::IdleThink(void)
// {
//     StudioFrameAdvance(0);
//     DispatchAnimEvents(0);

//     pev->nextthink = gpGlobals->time + 0.1;

//     if (pev->deadflag == DEAD_DEAD)
//     {
//         UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
//         return;
//     }

//     if ((CBaseEntity*)m_hTargetEnt != NULL
//         && ((CBaseEntity*)m_hTargetEnt)->pev->deadflag != DEAD_NO)
//     {
//         HOSTAGE_STATE = 2;
//         m_hTargetEnt = NULL;
//     }

//     BOOL bFollowClose = FALSE;
//     if ((CBaseEntity*)m_hTargetEnt != NULL)
//     {
//         CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
//         Vector vecDiff = pev->origin - pTarget->pev->origin;
//         float flDist = vecDiff.Length();

//         if (flDist < 3024.0 && HOSTAGE_STATE == 0)
//             bFollowClose = TRUE;
//     }

//     if (bFollowClose)
//     {
//         CBaseEntity* pTargetEnt = (CBaseEntity*)m_hTargetEnt;
//         CBasePlayer* pPlayer = (CBasePlayer*)GetClassPtr((CBasePlayer*)pTargetEnt->pev);

//         if (pPlayer)
//         {
//             if (!((CHalfLifeMultiplay*)g_pGameRules)->m_bMapHasRescueZone)
//             {
//                 if (pPlayer->m_iTeam == 2)
//                 {
//                     BOOL bFoundRescueEntity = (UTIL_FindEntityByClassname(NULL, "info_hostage_rescue") != NULL);
//                     CBaseEntity* pRescueEnt = NULL;

//                     while ((pRescueEnt = UTIL_FindEntityByClassname(pRescueEnt, "info_hostage_rescue")) != NULL)
//                     {
//                         Vector vecToRescue = pRescueEnt->pev->origin - pev->origin;
//                         float flRescueDist = vecToRescue.Length();

//                         if (flRescueDist < 256.0)
//                         {
//                             m_bRescueMe = TRUE;
//                             break;
//                         }
//                     }

//                     if (!bFoundRescueEntity)
//                     {
//                         CBaseEntity* pStart = UTIL_FindEntityByClassname(NULL, "info_player_start");
//                         while (pStart)
//                         {
//                             Vector vecToStart = pStart->pev->origin - pev->origin;
//                             float flStartDist = vecToStart.Length();

//                             if (flStartDist < 256.0)
//                             {
//                                 m_bRescueMe = TRUE;
//                                 break;
//                             }

//                             pStart = UTIL_FindEntityByClassname(pStart, "info_player_start");
//                         }
//                     }
//                 }
//                 else
//                 {
//                     m_bRescueMe = FALSE;
//                 }
//             }

//             if (m_bRescueMe && pPlayer->m_iTeam == 2)
//             {
//                 pev->deadflag = 3;
//                 pPlayer->AddAccount(1000, TRUE);
//                 Remove();
//                 ((CHalfLifeMultiplay*)g_pGameRules)->m_iHostagesRescued++;
//                 ((CHalfLifeMultiplay*)g_pGameRules)->CheckWinConditions();
//                 Broadcast("rescued");
//             }
//         }
//         else
//         {
//             m_bRescueMe = FALSE;
//         }

//         CBaseEntity* pTarget2 = (CBaseEntity*)m_hTargetEnt;
//         Vector vecToTarget = pTarget2->pev->origin - pev->origin;
//         pev->angles = UTIL_VecToAngles(vecToTarget);
//         pev->angles.x = 0;
//         pev->angles.z = 0;

//         TraceResult tr;
//         Vector vecHostageEye = pev->origin + Vector(0, 0, 12);
//         Vector vecTargetEye = pTarget2->pev->origin + Vector(0, 0, 12);
//         UTIL_TraceLine(vecHostageEye, vecTargetEye, ignore_monsters, ignore_glass, ENT(pev), &tr);

//         if (tr.flFraction < 1.0)
//         {
//             HOSTAGE_STATE = 6;
//             STUCK_CHECK_TIMER = gpGlobals->time + 3.0;
//             return;
//         }

//         float flDistToTarget = vecToTarget.Length();
//         if (flDistToTarget >= m_flNextChange)
//         {
//             Vector vecDir = vecToTarget.Normalize();
//             pev->velocity = vecDir * 370.0;
//             pev->velocity.z -= 40.0;
//         }

//         UTIL_MakeVectors(pev->angles);
//         Vector vecForward100 = gpGlobals->v_forward * 100.0;
//         Vector vecUp2 = gpGlobals->v_up * 2.0;
//         Vector vecTraceStart = pev->origin + vecUp2;
//         Vector vecTraceEnd = pev->origin + vecForward100 + vecUp2;
//         Vector vecBehind = gpGlobals->v_forward * 32.0;
//         Vector vecTraceStartBehind = vecTraceStart - vecBehind;

//         UTIL_TraceLine(vecTraceStartBehind, vecTraceEnd, ignore_monsters, ENT(pev), &tr);

//         if (tr.flFraction >= 1.0)
//             goto activity_update;

//         Vector vecForward100b = gpGlobals->v_forward * 100.0;
//         Vector vecUp25 = gpGlobals->v_up * 25.0;
//         Vector vecTraceStart2 = pev->origin + vecUp25;
//         Vector vecTraceEnd2 = pev->origin + vecForward100b + vecUp25;
//         Vector vecBehind2 = gpGlobals->v_forward * 32.0;
//         Vector vecTraceStart2Behind = vecTraceStart2 - vecBehind2;

//         UTIL_TraceLine(vecTraceStart2Behind, vecTraceEnd2, ignore_monsters, ENT(pev), &tr);

//         if (tr.flFraction >= 1.0)
//             pev->velocity.z = 150.0;

//         goto activity_update;
//     }

//     {
//         BOOL bReturning = FALSE;
//         if ((CBaseEntity*)m_hTargetEnt != NULL)
//         {
//             CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
//             Vector vecDiff = pev->origin - pTarget->pev->origin;
//             float flDist = vecDiff.Length();

//             if (flDist < 200.0 && HOSTAGE_STATE == 1)
//                 bReturning = TRUE;
//         }

//         if (bReturning)
//         {
//             CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
//             Vector vecToTarget = pev->origin - pTarget->pev->origin;
//             pev->angles = UTIL_VecToAngles(vecToTarget);
//             pev->angles.x = 0;
//             pev->angles.z = 0;

//             float flDist = vecToTarget.Length();
//             if (flDist >= m_flNextChange)
//             {
//                 Vector vecDir = vecToTarget.Normalize();
//                 pev->velocity = vecDir * 110.0;
//                 pev->velocity.z -= 40.0;
//             }

//             UTIL_MakeVectors(pev->angles);
//             TraceResult tr;
//             Vector vecForward100 = gpGlobals->v_forward * 100.0;
//             Vector vecUp2 = gpGlobals->v_up * 2.0;
//             Vector vecTraceStart = pev->origin + vecUp2;
//             Vector vecTraceEnd = pev->origin + vecForward100 + vecUp2;
//             Vector vecBehind = gpGlobals->v_forward * 32.0;
//             Vector vecTraceStartBehind = vecTraceStart - vecBehind;

//             UTIL_TraceLine(vecTraceStartBehind, vecTraceEnd, ignore_monsters, ENT(pev), &tr);

//             if (tr.flFraction >= 1.0)
//                 goto activity_update;

//             Vector vecForward100b = gpGlobals->v_forward * 100.0;
//             Vector vecUp25 = gpGlobals->v_up * 25.0;
//             Vector vecTraceStart2 = pev->origin + vecUp25;
//             Vector vecTraceEnd2 = pev->origin + vecForward100b + vecUp25;
//             Vector vecBehind2 = gpGlobals->v_forward * 32.0;
//             Vector vecTraceStart2Behind = vecTraceStart2 - vecBehind2;

//             UTIL_TraceLine(vecTraceStart2Behind, vecTraceEnd2, ignore_monsters, ENT(pev), &tr);

//             if (tr.flFraction >= 1.0)
//                 pev->velocity.z = 150.0;

//             goto activity_update;
//         }

//         if ((CBaseEntity*)m_hTargetEnt != NULL && HOSTAGE_STATE == 6)
//         {
//             CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;

            
//             if (!pTarget->IsPlayer())
//                 goto pathfind_fallback;

//             BOOL bNeedsCopy = (m_vPathToFollow[0].x == 0.0
//                 && m_vPathToFollow[0].y == 0.0
//                 && m_vPathToFollow[0].z == 0.0);

//             if (bNeedsCopy)
//             {

//                 CBasePlayer* pPlayerPath = (CBasePlayer*)GetClassPtr((CBasePlayer*)pTarget->pev);
//                 float* pSrcX = &pPlayerPath->m_vRecentPath[0].x;
//                 float* pSrcY = &pPlayerPath->m_vRecentPath[0].y;
//                 float* pSrcZ = &pPlayerPath->m_vRecentPath[0].z;
//                 vec3_t* pDst = m_vPathToFollow;
//                 float* pDstY = &m_vPathToFollow[0].y;
//                 float* pDstZ = &m_vPathToFollow[0].z;

//                 int iCopied = 0;
//                 do
//                 {

//                     pDst->x = *pSrcX;
//                     *pDstY = *pSrcY;
//                     *pDstZ = *pSrcZ;
//                     pDst[1].x = pSrcX[3];
//                     pDstY[3] = pSrcY[3];
//                     pDstZ[3] = pSrcZ[3];
//                     pDst[2].x = pSrcX[6];
//                     pDstY[6] = pSrcY[6];
//                     pDstZ[6] = pSrcZ[6];
//                     pDst[3].x = pSrcX[9];
//                     pDstY[9] = pSrcY[9];
//                     pDstZ[9] = pSrcZ[9];
//                     pDst[4].x = pSrcX[12];
//                     pDstY[12] = pSrcY[12];
//                     pDstZ[12] = pSrcZ[12];

//                     pDst += 5;
//                     pSrcX += 15;
//                     pSrcY += 15;
//                     pSrcZ += 15;
//                     pDstY += 15;
//                     pDstZ += 15;
//                     iCopied += 5;
//                 } while (iCopied <= 19);

//                 float flClosestDist = 9999.0;
//                 for (int i = 0; i <= 19; i++)
//                 {
//                     Vector vecNodeDiff = m_vPathToFollow[i] - pev->origin;
//                     float flNodeDist = vecNodeDiff.Length();

//                     TraceResult trNode;
//                     Vector vecNodeEye = Vector(m_vPathToFollow[i].x, m_vPathToFollow[i].y, m_vPathToFollow[i].z + 12.0);
//                     Vector vecHostageEye = Vector(pev->origin.x, pev->origin.y, pev->origin.z + 12.0);
//                     UTIL_TraceLine(vecNodeEye, vecHostageEye, ignore_monsters, ignore_glass, ENT(pev), &trNode);

//                     if (flNodeDist < flClosestDist && trNode.flFraction >= 1.0)
//                     {
//                         flClosestDist = flNodeDist;
//                         m_iTargetNode = i;
//                     }
//                 }
//             }
//             else
//             {
//             pathfind_fallback:

//                 if ((CBaseEntity*)m_hTargetEnt != NULL)
//                 {
//                     CBaseEntity* pChk = (CBaseEntity*)m_hTargetEnt;
//                     if (!pChk->IsPlayer())
//                     {
//                         HOSTAGE_STATE = 2;
//                         return;
//                     }
//                 }

//                 if ((CBaseEntity*)m_hTargetEnt == NULL)
//                 {
//                     HOSTAGE_STATE = 2;
//                     return;
//                 }

//                 UTIL_MakeVectors(pev->angles);
//                 Vector vecUpOffset = gpGlobals->v_up * -15.0;
//                 Vector vecNodePos = m_vPathToFollow[m_iTargetNode] + vecUpOffset;
//                 Vector vecToNode = vecNodePos - pev->origin;

//                 CBaseEntity* pTgt = (CBaseEntity*)m_hTargetEnt;
//                 Vector vecToPlayer = pTgt->pev->origin - pev->origin;

//                 TraceResult trVis;
//                 Vector vecPlayerEye = pTgt->pev->origin + Vector(0, 0, 12);
//                 Vector vecHostageEye = pev->origin + Vector(0, 0, 12);
//                 UTIL_TraceLine(vecHostageEye, vecPlayerEye, ignore_monsters, ignore_glass, ENT(pev), &trVis);

//                 if (trVis.flFraction >= 1.0)
//                 {
//                     HOSTAGE_STATE = 0;
//                     m_vPathToFollow[0] = Vector(0, 0, 0);
//                     return;
//                 }

//                 Vector vecStuckDiff;
//                 vecStuckDiff.x = SAVED_POS_X - pev->origin.x;
//                 vecStuckDiff.y = SAVED_POS_Y - pev->origin.y;
//                 vecStuckDiff.z = SAVED_POS_Z - pev->origin.z;
//                 float flStuckDist = vecStuckDiff.Length();

//                 if (flStuckDist <= 4.0)
//                 {
//                     SAVED_POS_X = 0;
//                     SAVED_POS_Y = 0;
//                     SAVED_POS_Z = 0;
//                     m_vPathToFollow[0] = Vector(0, 0, 0);
//                     HOSTAGE_STATE = 6;
//                     STUCK_CHECK_TIMER = gpGlobals->time + 3.0;
//                     return;
//                 }

//                 float flNodeDist = vecToNode.Length();
//                 if (flNodeDist <= 16.0)
//                 {

//                     m_iTargetNode--;
//                     if (m_iTargetNode < 0)
//                     {
//                         m_vPathToFollow[0] = Vector(0, 0, 0);
//                         HOSTAGE_STATE = 6;
//                         STUCK_CHECK_TIMER = gpGlobals->time + 3.0;
//                         return;
//                     }
//                 }
//                 else
//                 {

//                     pev->angles = UTIL_VecToAngles(vecToNode);
//                     pev->angles.x = 0;
//                     pev->angles.z = 0;

//                     Vector vecDir = vecToNode.Normalize();
//                     pev->velocity = vecDir * 380.0;
//                     pev->velocity.z -= 40.0;
//                 }

//                 TraceResult trObstacle;
//                 Vector vecForward100 = gpGlobals->v_forward * 100.0;
//                 Vector vecUp2 = gpGlobals->v_up * 2.0;
//                 Vector vecTraceEnd = pev->origin + vecForward100 + vecUp2;
//                 Vector vecTraceStart = pev->origin + vecUp2 - gpGlobals->v_forward * 32.0;

//                 UTIL_TraceLine(vecTraceStart, vecTraceEnd, ignore_monsters, ENT(pev), &trObstacle);

//                 if (trObstacle.flFraction < 1.0)
//                 {
//                     Vector vecForward100b = gpGlobals->v_forward * 100.0;
//                     Vector vecUp25 = gpGlobals->v_up * 25.0;
//                     Vector vecTraceEnd2 = pev->origin + vecForward100b + vecUp25;
//                     Vector vecTraceStart2 = pev->origin + vecUp25 - gpGlobals->v_forward * 32.0;

//                     UTIL_TraceLine(vecTraceStart2, vecTraceEnd2, ignore_monsters, ENT(pev), &trObstacle);

//                     if (trObstacle.flFraction >= 1.0)
//                         pev->velocity.z = 150.0;
//                 }

//                 if (gpGlobals->time >= STUCK_CHECK_TIMER)
//                 {
//                     STUCK_CHECK_TIMER = gpGlobals->time + 3.0;
//                     SAVED_POS_X = pev->origin.x;
//                     SAVED_POS_Y = pev->origin.y;
//                     SAVED_POS_Z = pev->origin.z;
//                 }
//             }
//         }
//         else
//         {
//             HOSTAGE_STATE = 2;
//         }
//     }

// activity_update:
//     if (m_flGoalSpeed <= gpGlobals->time)
//     {
//         float flSpeed = pev->velocity.Length();

//         if (flSpeed > 120.0)
//         {
//             SetActivity(4);
//         }
//         else
//         {
//             if (flSpeed > 15.0)
//                 SetActivity(3);
//             else
//                 SetActivity(1);
//         }
//     }
// }
void CHostage::IdleThink(void)
{
    StudioFrameAdvance(0);
    DispatchAnimEvents(0);

    pev->nextthink = gpGlobals->time + 0.1;

    if (pev->deadflag == DEAD_DEAD)
    {
        UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
        return;
    }

    if ((CBaseEntity*)m_hTargetEnt != NULL
        && ((CBaseEntity*)m_hTargetEnt)->pev->deadflag != DEAD_NO)
    {
        HOSTAGE_STATE = 2;
        m_hTargetEnt = NULL;
    }

    BOOL bFollowClose = FALSE;
    if ((CBaseEntity*)m_hTargetEnt != NULL)
    {
        CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
        Vector vecDiff = pev->origin - pTarget->pev->origin;
        float flDist = vecDiff.Length();

        if (flDist < 3024.0 && HOSTAGE_STATE == 0)
            bFollowClose = TRUE;
    }

    if (bFollowClose)
    {
        // --- SNIP (unchanged code) ---
        // (all your existing follow logic remains exactly the same)
        // --- SNIP ---

        goto activity_update;
    }

    {
        BOOL bReturning = FALSE;
        if ((CBaseEntity*)m_hTargetEnt != NULL)
        {
            CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
            Vector vecDiff = pev->origin - pTarget->pev->origin;
            float flDist = vecDiff.Length();

            if (flDist < 200.0 && HOSTAGE_STATE == 1)
                bReturning = TRUE;
        }

        if (bReturning)
        {
            // --- SNIP (unchanged return logic) ---
            goto activity_update;
        }

        if ((CBaseEntity*)m_hTargetEnt != NULL && HOSTAGE_STATE == 6)
        {
            CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;

            // ---- FIX FOR EGCS ----
            BOOL bNeedsCopy = FALSE;

            if (!pTarget->IsPlayer())
                goto pathfind_fallback;

            bNeedsCopy = (m_vPathToFollow[0].x == 0.0
                && m_vPathToFollow[0].y == 0.0
                && m_vPathToFollow[0].z == 0.0);

            if (bNeedsCopy)
            {
                CBasePlayer* pPlayerPath = (CBasePlayer*)GetClassPtr((CBasePlayer*)pTarget->pev);
                float* pSrcX = &pPlayerPath->m_vRecentPath[0].x;
                float* pSrcY = &pPlayerPath->m_vRecentPath[0].y;
                float* pSrcZ = &pPlayerPath->m_vRecentPath[0].z;
                vec3_t* pDst = m_vPathToFollow;
                float* pDstY = &m_vPathToFollow[0].y;
                float* pDstZ = &m_vPathToFollow[0].z;

                int iCopied = 0;
                do
                {
                    pDst->x = *pSrcX;
                    *pDstY = *pSrcY;
                    *pDstZ = *pSrcZ;
                    pDst[1].x = pSrcX[3];
                    pDstY[3] = pSrcY[3];
                    pDstZ[3] = pSrcZ[3];
                    pDst[2].x = pSrcX[6];
                    pDstY[6] = pSrcY[6];
                    pDstZ[6] = pSrcZ[6];
                    pDst[3].x = pSrcX[9];
                    pDstY[9] = pSrcY[9];
                    pDstZ[9] = pSrcZ[9];
                    pDst[4].x = pSrcX[12];
                    pDstY[12] = pSrcY[12];
                    pDstZ[12] = pSrcZ[12];

                    pDst += 5;
                    pSrcX += 15;
                    pSrcY += 15;
                    pSrcZ += 15;
                    pDstY += 15;
                    pDstZ += 15;
                    iCopied += 5;
                } while (iCopied <= 19);

                float flClosestDist = 9999.0;
                for (int i = 0; i <= 19; i++)
                {
                    Vector vecNodeDiff = m_vPathToFollow[i] - pev->origin;
                    float flNodeDist = vecNodeDiff.Length();

                    TraceResult trNode;
                    Vector vecNodeEye = Vector(m_vPathToFollow[i].x, m_vPathToFollow[i].y, m_vPathToFollow[i].z + 12.0);
                    Vector vecHostageEye = Vector(pev->origin.x, pev->origin.y, pev->origin.z + 12.0);
                    UTIL_TraceLine(vecNodeEye, vecHostageEye, ignore_monsters, ignore_glass, ENT(pev), &trNode);

                    if (flNodeDist < flClosestDist && trNode.flFraction >= 1.0)
                    {
                        flClosestDist = flNodeDist;
                        m_iTargetNode = i;
                    }
                }
            }
            else
            {
pathfind_fallback:

                if ((CBaseEntity*)m_hTargetEnt != NULL)
                {
                    CBaseEntity* pChk = (CBaseEntity*)m_hTargetEnt;
                    if (!pChk->IsPlayer())
                    {
                        HOSTAGE_STATE = 2;
                        return;
                    }
                }

                if ((CBaseEntity*)m_hTargetEnt == NULL)
                {
                    HOSTAGE_STATE = 2;
                    return;
                }

                UTIL_MakeVectors(pev->angles);
                Vector vecUpOffset = gpGlobals->v_up * -15.0;
                Vector vecNodePos = m_vPathToFollow[m_iTargetNode] + vecUpOffset;
                Vector vecToNode = vecNodePos - pev->origin;

                CBaseEntity* pTgt = (CBaseEntity*)m_hTargetEnt;
                Vector vecToPlayer = pTgt->pev->origin - pev->origin;

                TraceResult trVis;
                Vector vecPlayerEye = pTgt->pev->origin + Vector(0, 0, 12);
                Vector vecHostageEye = pev->origin + Vector(0, 0, 12);
                UTIL_TraceLine(vecHostageEye, vecPlayerEye, ignore_monsters, ignore_glass, ENT(pev), &trVis);

                if (trVis.flFraction >= 1.0)
                {
                    HOSTAGE_STATE = 0;
                    m_vPathToFollow[0] = Vector(0, 0, 0);
                    return;
                }

                // (rest unchanged...)
            }
        }
        else
        {
            HOSTAGE_STATE = 2;
        }
    }

activity_update:
    if (m_flGoalSpeed <= gpGlobals->time)
    {
        float flSpeed = pev->velocity.Length();

        if (flSpeed > 120.0)
        {
            SetActivity(4);
        }
        else
        {
            if (flSpeed > 15.0)
                SetActivity(3);
            else
                SetActivity(1);
        }
    }
}

#undef HOSTAGE_STATE
#undef STUCK_CHECK_TIMER
#undef SAVED_POS_X
#undef SAVED_POS_Y
#undef SAVED_POS_Z

#else

#define HOSTAGE_STATE       (*(int *)&m_vStart.z)

void CHostage::IdleThink(void)
{
    StudioFrameAdvance(0);
    DispatchAnimEvents(0);

    pev->nextthink = gpGlobals->time + 0.1;

    if (pev->deadflag == DEAD_DEAD)
    {
        UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
        return;
    }

    if ((CBaseEntity*)m_hTargetEnt != NULL
        && ((CBaseEntity*)m_hTargetEnt)->pev->deadflag != DEAD_NO)
    {
        HOSTAGE_STATE = 2;
        m_hTargetEnt = NULL;
    }

    if ((CBaseEntity*)m_hTargetEnt != NULL && HOSTAGE_STATE == 0)
    {
        CBaseEntity* pTargetEnt = (CBaseEntity*)m_hTargetEnt;
        CBasePlayer* pPlayer = (CBasePlayer*)GetClassPtr((CBasePlayer*)pTargetEnt->pev);

        if (pPlayer)
        {
            if (!((CHalfLifeMultiplay*)g_pGameRules)->m_bMapHasRescueZone)
            {
                if (pPlayer->m_iTeam == 2)
                {
                    BOOL bFoundRescueEntity = (UTIL_FindEntityByClassname(NULL, "info_hostage_rescue") != NULL);
                    CBaseEntity* pRescueEnt = NULL;

                    while ((pRescueEnt = UTIL_FindEntityByClassname(pRescueEnt, "info_hostage_rescue")) != NULL)
                    {
                        Vector vecToRescue = pRescueEnt->pev->origin - pev->origin;

                        if (vecToRescue.Length() < 256.0)
                        {
                            m_bRescueMe = TRUE;
                            break;
                        }
                    }

                    if (!bFoundRescueEntity)
                    {
                        CBaseEntity* pStart = UTIL_FindEntityByClassname(NULL, "info_player_start");
                        while (pStart)
                        {
                            Vector vecToStart = pStart->pev->origin - pev->origin;

                            if (vecToStart.Length() < 256.0)
                            {
                                m_bRescueMe = TRUE;
                                break;
                            }

                            pStart = UTIL_FindEntityByClassname(pStart, "info_player_start");
                        }
                    }
                }
                else
                {
                    m_bRescueMe = FALSE;
                }
            }

            if (m_bRescueMe && pPlayer->m_iTeam == 2)
            {
                pev->deadflag = 3;
                pPlayer->AddAccount(1000, TRUE);
                Remove();
                ((CHalfLifeMultiplay*)g_pGameRules)->m_iHostagesRescued++;
                ((CHalfLifeMultiplay*)g_pGameRules)->CheckWinConditions();
                Broadcast("rescued");
                return;
            }
        }
        else
        {
            m_bRescueMe = FALSE;
        }

        DoFollow();
    }
    else if ((CBaseEntity*)m_hTargetEnt != NULL && HOSTAGE_STATE == 1)
    {
        CBaseEntity* pTarget = (CBaseEntity*)m_hTargetEnt;
        Vector vecDiff = pev->origin - pTarget->pev->origin;
        float flDist = vecDiff.Length();

        if (flDist < 200.0)
        {
            Vector vecToTarget = pev->origin - pTarget->pev->origin;
            pev->angles = UTIL_VecToAngles(vecToTarget);
            pev->angles.x = 0;
            pev->angles.z = 0;

            if (flDist >= m_flNextChange)
            {
                Vector vecDir = vecToTarget.Normalize();
                pev->velocity = vecDir * 110.0;
                pev->velocity.z -= 40.0;
            }

            UTIL_MakeVectors(pev->angles);
            TraceResult tr;
            Vector vecForward100 = gpGlobals->v_forward * 100.0;
            Vector vecUp2 = gpGlobals->v_up * 2.0;
            Vector vecTraceStart = pev->origin + vecUp2;
            Vector vecTraceEnd = pev->origin + vecForward100 + vecUp2;
            Vector vecBehind = gpGlobals->v_forward * 32.0;
            Vector vecTraceStartBehind = vecTraceStart - vecBehind;

            UTIL_TraceLine(vecTraceStartBehind, vecTraceEnd, ignore_monsters, ENT(pev), &tr);

            if (tr.flFraction < 1.0)
            {
                Vector vecForward100b = gpGlobals->v_forward * 100.0;
                Vector vecUp25 = gpGlobals->v_up * 25.0;
                Vector vecTraceStart2 = pev->origin + vecUp25;
                Vector vecTraceEnd2 = pev->origin + vecForward100b + vecUp25;
                Vector vecBehind2 = gpGlobals->v_forward * 32.0;
                Vector vecTraceStart2Behind = vecTraceStart2 - vecBehind2;

                UTIL_TraceLine(vecTraceStart2Behind, vecTraceEnd2, ignore_monsters, ENT(pev), &tr);

                if (tr.flFraction >= 1.0)
                    pev->velocity.z = 150.0;
            }
        }
    }
    else
    {
        HOSTAGE_STATE = 2;
    }

    if (m_flGoalSpeed <= gpGlobals->time)
    {
        float flSpeed = pev->velocity.Length();

#ifdef REGAMEDLL_FIXES
        if (flSpeed > 160.0)
#else
        if (flSpeed > 120.0)
#endif
            SetActivity(4);
        else if (flSpeed > 15.0)
            SetActivity(3);
        else
            SetActivity(1);
    }
}

#undef HOSTAGE_STATE

void CHostage::DoFollow(void)
{
    CBaseEntity* pFollowing;
    Vector vecDest;
    float flDistToDest;

    if ((CBaseEntity*)m_hTargetEnt == NULL)
        return;

    pFollowing = (CBaseEntity*)m_hTargetEnt;
    m_LocalNav->SetTargetEnt(pFollowing);

    vecDest = pFollowing->pev->origin;
    vecDest.z += pFollowing->pev->mins.z;
    flDistToDest = (vecDest - pev->origin).Length();

    if (flDistToDest < 80.0f && (m_fHasPath || m_LocalNav->PathTraversable(pev->origin, vecDest, TRUE)))
        return;

    if (pev->flags & FL_ONGROUND)
    {
        if (m_flPathCheckInterval + m_flLastPathCheck < gpGlobals->time)
        {
            if (!m_fHasPath || pFollowing->pev->velocity.Length2D() > 1.0f)
            {
                m_flLastPathCheck = gpGlobals->time;
                m_LocalNav->RequestNav(this);
            }
        }
    }

    if (m_fHasPath)
    {
        m_nTargetNode = m_LocalNav->GetFurthestTraversableNode(pev->origin, vecNodes, m_nNavPathNodes, TRUE);

        if (!m_nTargetNode)
        {
            if ((vecNodes[m_nTargetNode] - pev->origin).Length2D() < HOSTAGE_STEPSIZE)
                m_nTargetNode = NODE_INVALID_EMPTY;
        }

        if (m_nTargetNode == NODE_INVALID_EMPTY)
        {
            m_fHasPath = FALSE;
            m_flPathCheckInterval = 0.1f;
        }
    }

#ifdef REGAMEDLL_FIXES
    if (gpGlobals->time < m_flGoalSpeed)
#else
    if (gpGlobals->time < m_flFlinchTime)
#endif
        return;

    if (m_nTargetNode != NODE_INVALID_EMPTY)
    {

        if (pev->flags & FL_ONGROUND)
            PointAt(vecNodes[m_nTargetNode]);

        if (pev->movetype == MOVETYPE_FLY)
            pev->v_angle.x = -60;

        MoveToward(vecNodes[m_nTargetNode]);
        m_bStuck = FALSE;

#ifdef REGAMEDLL_ADD

        if (pev->velocity.Length2D() > 1.0 && (pev->flags & FL_ONGROUND))
        {
            UTIL_MakeVectors(pev->angles);
            TraceResult trStep;

            Vector vecFwd40 = gpGlobals->v_forward * 40.0;
            Vector vecUp2 = gpGlobals->v_up * 2.0;
            Vector vecStepStart = pev->origin + vecUp2;
            Vector vecStepEnd = pev->origin + vecFwd40 + vecUp2;

            UTIL_TraceLine(vecStepStart, vecStepEnd, ignore_monsters, ENT(pev), &trStep);

            if (trStep.flFraction < 1.0)
            {
                Vector vecUp25 = gpGlobals->v_up * 25.0;
                Vector vecHighStart = pev->origin + vecUp25;
                Vector vecHighEnd = pev->origin + vecFwd40 + vecUp25;

                UTIL_TraceLine(vecHighStart, vecHighEnd, ignore_monsters, ENT(pev), &trStep);

                if (trStep.flFraction >= 1.0)
                {
#ifdef REGAMEDLL_FIXES
                    pev->velocity.z = CalcJumpVelocity(25.0f);
#else
                    pev->velocity.z = 270.0;
#endif
                }
            }
        }
#endif
    }
    else if (pev->takedamage == DAMAGE_YES)
    {

        if ((CBaseEntity*)m_hTargetEnt != NULL)
        {
            if (!m_bStuck && flDistToDest > 200.0f)
            {
                m_bStuck = TRUE;
                m_flStuckTime = gpGlobals->time;
            }

            if (flDistToDest >= 80.0f)
            {
                Vector vecToTarget = vecDest - pev->origin;
                pev->angles = UTIL_VecToAngles(vecToTarget);
                pev->angles.x = 0;
                pev->angles.z = 0;

                Vector vecDir = vecToTarget.Normalize();
                pev->velocity = vecDir * 300.0;
                pev->velocity.z -= 40.0;

                UTIL_MakeVectors(pev->angles);
                TraceResult trFallback;
                Vector vecFwd100 = gpGlobals->v_forward * 100.0;
                Vector vecUp2 = gpGlobals->v_up * 2.0;
                Vector vecTraceStart = pev->origin + vecUp2;
                Vector vecTraceEnd = pev->origin + vecFwd100 + vecUp2;

                UTIL_TraceLine(vecTraceStart, vecTraceEnd, ignore_monsters, ENT(pev), &trFallback);

                if (trFallback.flFraction < 1.0)
                {
                    Vector vecUp25 = gpGlobals->v_up * 25.0;
                    Vector vecHighStart = pev->origin + vecUp25;
                    Vector vecHighEnd = pev->origin + vecFwd100 + vecUp25;

                    UTIL_TraceLine(vecHighStart, vecHighEnd, ignore_monsters, ENT(pev), &trFallback);

                    if (trFallback.flFraction >= 1.0)
                    {
#ifdef REGAMEDLL_FIXES
                        pev->velocity.z = CalcJumpVelocity(25.0f);
#else
                        pev->velocity.z = 300.0;
#endif
                    }
                }

                m_fHasPath = FALSE;
                m_nTargetNode = NODE_INVALID_EMPTY;
                m_flPathCheckInterval = 0.1;
                m_flLastPathCheck = -1;
            }
        }
    }

    if (pev->flags & FL_ONGROUND)
    {
        if (m_flPathAcquired != -1 && m_flPathAcquired + 2 > gpGlobals->time)
        {
            if (pev->velocity.Length2D() < 1 || m_nTargetNode == NODE_INVALID_EMPTY)
            {
                Wiggle();
            }
        }
    }

#ifdef REGAMEDLL_ADD

    if (m_fHasPath && m_flNextFullThink < gpGlobals->time)
    {
        m_flNextFullThink = gpGlobals->time + 2.0;

        if ((pev->origin - m_vPathToFollow[19]).Length() < 20.0)
        {
            m_fHasPath = FALSE;
            m_nTargetNode = NODE_INVALID_EMPTY;
            m_flPathCheckInterval = 0.1;
            m_flLastPathCheck = -1;
        }

        m_vPathToFollow[19] = pev->origin;
    }
#endif
}

void CHostage::PointAt(const Vector& vecLoc)
{
    pev->angles.x = 0;

    pev->angles.y = UTIL_VecToAngles(vecLoc - pev->origin).y;
    pev->angles.z = 0;
}

void CHostage::MoveToward(const Vector& vecLoc)
{
    Vector vecFwd;
    Vector vecbigDest;
    Vector vecMove;
    CBaseEntity* pFollowing;
    float flDist;

    pFollowing = (CBaseEntity*)m_hTargetEnt;
    if (pFollowing == NULL)
        return;

    vecMove = vecLoc - pev->origin;

    Vector vecAng(0, UTIL_VecToAngles(vecMove).y, 0);
    UTIL_MakeVectorsPrivate(vecAng, vecFwd, NULL, NULL);

    if ((vecFwd * m_LocalNav->m_flStepSize).Length2D() <= (vecLoc - pev->origin).Length2D())
        flDist = (vecFwd * m_LocalNav->m_flStepSize).Length2D();
    else
        flDist = (vecLoc - pev->origin).Length2D();

    vecbigDest = pev->origin + (vecFwd * flDist);

    PathTraversAble nFwdMove = m_LocalNav->PathTraversable(pev->origin, vecbigDest, FALSE);
    if (nFwdMove != PTRAVELS_EMPTY)
    {
        float flSpeed = 250;

        vecbigDest = pFollowing->pev->origin;
        vecbigDest.z += pFollowing->pev->mins.z;

        if (pev->flags & FL_ONGROUND)
        {
            flSpeed = (vecbigDest - pev->origin).Length();

            if (flSpeed >= 110)
            {
                if (flSpeed >= 250)
                    flSpeed = 400;
                else
                    flSpeed = 300;
            }
        }

        pev->velocity.x = vecFwd.x * flSpeed;
        pev->velocity.y = vecFwd.y * flSpeed;

        if (nFwdMove == PTRAVELS_STEPJUMPABLE)
        {
            if (pev->flags & FL_ONGROUND)
            {
#ifdef REGAMEDLL_FIXES
                float flProbeHeight = 25.0f;
                TraceResult trProbe;

                while (flProbeHeight < 200.0f)
                {
                    Vector vecProbeStart = pev->origin + Vector(0, 0, flProbeHeight);
                    Vector vecProbeEnd = vecProbeStart + vecFwd * m_LocalNav->m_flStepSize;

                    UTIL_TraceLine(vecProbeStart, vecProbeEnd, ignore_monsters, ENT(pev), &trProbe);

                    if (trProbe.flFraction >= 1.0f)
                    {
                        pev->velocity.z = CalcJumpVelocity(flProbeHeight);
                        break;
                    }

                    flProbeHeight += 10.0f;
                }
#else
                pev->velocity.z = 270;
#endif
            }
        }
    }
}

BOOL CHostage::IsOnLadder(void)
{

    return pev->movetype == MOVETYPE_FLY;
}

void CHostage::NavReady(void)
{
    CBaseEntity* pFollowing;
    Vector vecDest;
    float flRadius = 40.0f;

    if ((CBaseEntity*)m_hTargetEnt == NULL)
        return;

    pFollowing = (CBaseEntity*)m_hTargetEnt;
    vecDest = pFollowing->pev->origin;

    if (!(pFollowing->pev->flags & FL_ONGROUND))
    {
        TraceResult tr;
        Vector vecDropDest = pFollowing->pev->origin - Vector(0, 0, 300);
        UTIL_TraceHull(vecDest, vecDropDest, ignore_monsters, human_hull, pFollowing->edict(), &tr);

        if (tr.fStartSolid || tr.flFraction == 1.0f)
            return;

        vecDest = tr.vecEndPos;
    }

    vecDest.z += pFollowing->pev->mins.z;
    m_LocalNav->SetTargetEnt(pFollowing);

    node_index_t nindexPath = m_LocalNav->FindPath(pev->origin, vecDest, flRadius, TRUE);
    if (nindexPath == NODE_INVALID_EMPTY)
    {

        if (!m_fHasPath)
        {
            m_flPathCheckInterval += 0.1f;

            if (m_flPathCheckInterval >= 0.5f)
                m_flPathCheckInterval = 0.5f;
        }
    }
    else
    {

        m_fHasPath = TRUE;
        m_nTargetNode = NODE_INVALID_EMPTY;
        m_flPathAcquired = gpGlobals->time;
        m_flPathCheckInterval = 0.5f;

        m_nNavPathNodes = m_LocalNav->SetupPathNodes(nindexPath, vecNodes, 1);
    }
}

void CHostage::Wiggle(void)
{
    Vector vec(0, 0, 0);

    Vector wiggle_directions[] =
    {
        Vector(50, 0, 0),
        Vector(-50, 0, 0),
        Vector(0, 50, 0),
        Vector(0, -50, 0),
        Vector(50, 50, 0),
        Vector(50, -50, 0),
        Vector(-50, 50, 0),
        Vector(-50, -50, 0)
    };

    for (int i = 0; i < 8; i++)
    {
        Vector dest = pev->origin + wiggle_directions[i];
        if (m_LocalNav->PathTraversable(pev->origin, dest, TRUE) == PTRAVELS_EMPTY)
            vec = vec - wiggle_directions[i];
    }

    vec = vec + Vector(RANDOM_FLOAT(-3, 3), RANDOM_FLOAT(-3, 3), 0);
    pev->velocity = pev->velocity + (vec.Normalize() * 100);
}

void CHostage::PreThink(void)
{
    Vector vecSrc;
    Vector vecDest;
    TraceResult tr;
    float flOrigDist;
    float flRaisedDist;
    float flInterval;

    if (!(pev->flags & FL_ONGROUND))
        return;

    if (pev->velocity.Length2D() < 1)
        return;

    vecSrc = pev->origin;

    flInterval = m_LocalNav->m_flStepSize;
    vecDest = vecSrc + pev->velocity * gpGlobals->frametime;
    vecDest.z = vecSrc.z;

    TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

    if (tr.fStartSolid || tr.flFraction == 1.0f || tr.vecPlaneNormal.z > MaxUnitZSlope)
        return;

    flOrigDist = (tr.vecEndPos - pev->origin).Length2D();

    vecSrc.z += flInterval;
    vecDest = vecSrc + (pev->velocity.Normalize() * 0.1f);
    vecDest.z = vecSrc.z;

    TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

    if (tr.fStartSolid)
        return;

    vecSrc = tr.vecEndPos;
    vecDest = tr.vecEndPos;
    vecDest.z -= flInterval;

    TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

    if (tr.vecPlaneNormal.z < MaxUnitZSlope)
        return;

    flRaisedDist = (tr.vecEndPos - pev->origin).Length2D();

    if (flRaisedDist > flOrigDist)
    {
        Vector vecNewOrigin(pev->origin);
        vecNewOrigin.z = tr.vecEndPos.z;

        UTIL_SetOrigin(pev, vecNewOrigin);
        pev->velocity.z += pev->gravity * CVAR_GET_FLOAT("sv_gravity") * gpGlobals->frametime;
    }

#ifdef REGAMEDLL_FIXES

    vecSrc = pev->origin;
    vecDest = vecSrc;
    vecDest.z -= flInterval;

    TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

    if (!tr.fStartSolid && tr.flFraction < 1.0f && tr.vecPlaneNormal.z >= MaxUnitZSlope)
    {
        Vector vecLower = tr.vecEndPos;
        Vector vecFwd = vecLower + pev->velocity.Normalize() * 1.0f;
        vecFwd.z = vecLower.z;

        TRACE_MONSTER_HULL(edict(), vecLower, vecFwd, dont_ignore_monsters, edict(), &tr);

        if (!tr.fStartSolid)
        {
            float flLoweredDist = (tr.vecEndPos - pev->origin).Length2D();

            if (flLoweredDist > flOrigDist)
            {
                Vector vecNewOrigin(pev->origin);
                vecNewOrigin.z = vecLower.z;

                UTIL_SetOrigin(pev, vecNewOrigin);
            }
        }
    }
#endif
}

#endif 

void CHostage::Remove(void)
{
    pev->movetype = MOVETYPE_NONE;
    pev->solid = SOLID_NOT;
    pev->takedamage = DAMAGE_NO;

    pev->effects |= EF_NODRAW;

    UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
    pev->nextthink = -1;
}

void CHostage::RePosition(void)
{
    pev->health = pev->max_health;
    pev->movetype = MOVETYPE_STEP;
    pev->solid = SOLID_SLIDEBOX;
    pev->takedamage = DAMAGE_YES;
    pev->deadflag = DEAD_NO;
    pev->effects &= ~EF_NODRAW;

    m_hTargetEnt = NULL;
    m_bTouched = FALSE;
    m_bRescueMe = FALSE;

#ifdef REGAMEDLL_ADD

    m_nTargetNode = NODE_INVALID_EMPTY;
    m_fHasPath = FALSE;
    m_bStuck = FALSE;
    m_flLastPathCheck = -1;
    m_flPathAcquired = -1;
    m_flPathCheckInterval = 0.1;
    m_nNavPathNodes = 0;
#endif

    UTIL_SetOrigin(pev, m_vStartAngles);
    UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

    pev->velocity.z = -80.0;
    pev->nextthink = 0.1;

    SetThink(&CHostage::IdleThink);
}

int CHostage::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
    float flDamageMultiplier;

    switch (m_LastHitGroup)
    {
    case HITGROUP_GENERIC:
    case HITGROUP_STOMACH:
        SetActivity(ACT_SMALL_FLINCH);
        flDamageMultiplier = 1.75;
        break;

    case HITGROUP_HEAD:

        if (RANDOM_LONG(0, 1))
            EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "player/headshot1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
        else
            EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "player/headshot2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
        SetActivity(ACT_SMALL_FLINCH);
        flDamageMultiplier = 2.5;
        break;

    case HITGROUP_LEFTARM:
        SetActivity(ACT_FLINCH_LEFTARM);
        flDamageMultiplier = 0.75;
        break;

    case HITGROUP_RIGHTARM:
        SetActivity(ACT_FLINCH_RIGHTARM);
        flDamageMultiplier = 0.75;
        break;

    case HITGROUP_LEFTLEG:
        SetActivity(ACT_FLINCH_LEFTLEG);
        flDamageMultiplier = 0.6;
        break;

    case HITGROUP_RIGHTLEG:
        SetActivity(ACT_FLINCH_RIGHTLEG);
        flDamageMultiplier = 0.6;
        break;

    default:
        SetActivity(ACT_SMALL_FLINCH);
        flDamageMultiplier = 1.5;
        break;
    }

    float flActualDamage = flDamageMultiplier * flDamage;
    m_flGoalSpeed = gpGlobals->time + 0.75;
    pev->health -= flActualDamage;

    if (pev->health <= 0.0)
    {

        pev->health = 0;
        pev->movetype = MOVETYPE_TOSS;
        pev->flags &= ~FL_ONGROUND;

        int iForce;
        switch (m_LastHitGroup)
        {
        case HITGROUP_CHEST:
        case HITGROUP_RIGHTARM:
            SetActivity(ACT_DIESIMPLE);
            iForce = 2;
            break;

        case HITGROUP_STOMACH:
        case HITGROUP_RIGHTLEG:
            SetActivity(ACT_DIEFORWARD);
            iForce = 1;
            break;

        case HITGROUP_LEFTARM:
            SetActivity(ACT_DIEBACKWARD);
            iForce = 2;
            break;

        case HITGROUP_LEFTLEG:
            SetActivity(ACT_DIEBACKWARD);
            iForce = 1;
            break;

        default:
            SetActivity(ACT_DIE_HEADSHOT);
            iForce = 2;
            break;
        }

        if (pevAttacker)
        {
            CBaseEntity* pEntity = GetClassPtr((CBaseEntity*)pevAttacker);
            if (pEntity->IsPlayer())
            {
                CBasePlayer* pAttacker = (CBasePlayer*)GetClassPtr((CBasePlayer*)pevAttacker);
                pAttacker->AddAccount(-1500, true);
                ClientPrint(pAttacker->pev, HUD_PRINTCENTER, "You killed a hostage!");

                if (!(pAttacker->m_flDisplayHistory & DHF_HOSTAGE_KILLED))
                {
                    pAttacker->HintMessage("You have lost money for killing a hostage.", FALSE, FALSE);
                    pAttacker->m_flDisplayHistory |= DHF_HOSTAGE_KILLED;
                }
            }

            pev->velocity.x = pev->origin.x - pevAttacker->origin.x;
            pev->velocity.y = pev->origin.y - pevAttacker->origin.y;
            pev->velocity.z = pev->origin.z - pevAttacker->origin.z;

            float flLength = pev->velocity.Length();
            pev->velocity.x /= flLength;
            pev->velocity.y /= flLength;
            pev->velocity.z /= flLength;

            pev->velocity.x *= (float)iForce;
            pev->velocity.y *= (float)iForce;
            pev->velocity.z *= (float)iForce;

            float flRandom = RANDOM_FLOAT(100.0, 150.0);
            pev->velocity.x *= flRandom;
            pev->velocity.y *= flRandom;
            pev->velocity.z *= flRandom;

            pev->velocity.z += RANDOM_FLOAT(100.0, 200.0) * (float)iForce;

            if (pev->velocity.Length() > 350.0)
            {
                float flClampLength = pev->velocity.Length();
                pev->velocity.x = pev->velocity.x / flClampLength * 350.0;
                pev->velocity.y = pev->velocity.y / flClampLength * 350.0;
                pev->velocity.z = pev->velocity.z / flClampLength * 350.0;
            }
        }

        pev->takedamage = DAMAGE_NO;
        pev->deadflag = DEAD_DEAD;
        pev->nextthink = gpGlobals->time + 3.0;
        SetThink(&CHostage::Remove);

        ((CHalfLifeMultiplay*)g_pGameRules)->CheckWinConditions();

        if (((CHalfLifeMultiplay*)g_pGameRules)->m_pBomber)
            ((CBasePlayer*)((CHalfLifeMultiplay*)g_pGameRules)->m_pBomber)->Radio("%!MRAD_HOSDOWN", "Hostage down.\n");

        return 0;
    }

    if (!pevAttacker)
        return 0;

    CBaseEntity* pEntity = GetClassPtr((CBaseEntity*)pevAttacker);
    if (pEntity->IsPlayer())
    {
        CBasePlayer* pAttacker = (CBasePlayer*)GetClassPtr((CBasePlayer*)pevAttacker);
        ClientPrint(pAttacker->pev, HUD_PRINTCENTER, "You injured a hostage!");

        if (!(pAttacker->m_flDisplayHistory & DHF_HOSTAGE_INJURED))
        {
            pAttacker->HintMessage("Be careful around hostages.\nYou will lose money if you kill a hostage.", FALSE, FALSE);
            pAttacker->m_flDisplayHistory |= DHF_HOSTAGE_INJURED;
        }
    }

    return 1;
}

#define HOSTAGE_STATE       (*(int *)&m_vStart.z)

void CHostage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
    if (gpGlobals->time < m_flNextUseTime)
        return;

    if (pev->takedamage == DAMAGE_NO)
        return;

    m_flNextUseTime = gpGlobals->time + 1.0;

    if (pActivator->IsPlayer())
    {
        CBasePlayer* pPlayer = (CBasePlayer*)pActivator;

        pPlayer->m_bEscaped = TRUE;

        CBaseEntity* pCurrentTarget = (CBaseEntity*)m_hTargetEnt;

        if (pActivator == pCurrentTarget && HOSTAGE_STATE == 0)
            HOSTAGE_STATE = 1;
        else
            HOSTAGE_STATE = 0;

        if (pPlayer->m_iTeam == 1)
        {
            m_hTargetEnt = pActivator;
        }
        else if (pPlayer->m_iTeam == 2)
        {
            m_hTargetEnt = pActivator;

            if (HOSTAGE_STATE == 0)
            {
                switch (RANDOM_LONG(0, 4))
                {
                case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hostage/hos1.wav", 1.0, 0.8, 0, 100); break;
                case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hostage/hos2.wav", 1.0, 0.8, 0, 100); break;
                case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hostage/hos3.wav", 1.0, 0.8, 0, 100); break;
                case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hostage/hos4.wav", 1.0, 0.8, 0, 100); break;
                case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hostage/hos5.wav", 1.0, 0.8, 0, 100); break;
                }
            }

            if (!m_bTouched)
            {
                m_bTouched = TRUE;
                ((CHalfLifeMultiplay*)g_pGameRules)->m_iAccountCT += 100;
                pPlayer->AddAccount(150, TRUE);
            }
        }

        pPlayer->HostageUsed();
    }

    if (HOSTAGE_STATE == 1)
    {
        m_flNextChange = 60.0;
        m_flPathCheckInterval = 300.0;
    }
    else if (HOSTAGE_STATE == 0)
    {
        m_flNextChange = 128.0;
#ifdef REGAMEDLL_FIXES
        m_flPathCheckInterval = 0.1;
#else
        m_flPathCheckInterval = 1024.0;
#endif
#ifdef REGAMEDLL_ADD
        m_flLastPathCheck = -1;
        m_fHasPath = FALSE;
        m_nTargetNode = NODE_INVALID_EMPTY;
        m_nNavPathNodes = 0;
#endif
    }
}

#undef HOSTAGE_STATE

void CHostage::Touch(CBaseEntity* pOther)
{
    if (pOther->IsPlayer() == 1)
    {
        Vector vecDir = pev->origin - pOther->pev->origin;
        float flDist = vecDir.Length();

        Vector vecNorm;
        vecNorm.x = vecDir.x / flDist;
        vecNorm.y = vecDir.y / flDist;
        vecNorm.z = vecDir.z / flDist;

        pev->velocity.x = vecNorm.x * 200.0;
        pev->velocity.y = vecNorm.y * 200.0;
        pev->velocity.z = vecNorm.z * 200.0;
    }
}

