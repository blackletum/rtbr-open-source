//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	GRENADEAR2_H
#define	GRENADEAR2_H

#include "basegrenade_shared.h"

#define	MAX_AR2_NO_COLLIDE_TIME 0.2

class SmokeTrail;
class CWeaponAR2;

class CGrenadeAR2 : public CBaseGrenade
{
public:
	DECLARE_CLASS( CGrenadeAR2, CBaseGrenade );

	CHandle< SmokeTrail > m_hSmokeTrail;
	float				 m_fSpawnTime;
	float				m_fDangerRadius;

	bool m_bIsFireGrenade = false;


	void		Spawn( void );
	void		Precache( void );
	void 		GrenadeAR2Touch( CBaseEntity *pOther );
	void		GrenadeAR2Think( void );
	Vector		GenerateRandomCircle(Vector originalpos);
	void		Event_Killed( const CTakeDamageInfo &info );
	void		IgniteOtherIfAllowed(CBaseEntity * pOther);

public:
	void EXPORT				Detonate(void);
	void EXPORT				DetonateFire(void);
	CGrenadeAR2(void);

	DECLARE_DATADESC();
};

#endif	//GRENADEAR2_H
