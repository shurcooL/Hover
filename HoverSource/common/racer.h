
#define NUM_VEHICLE_MODELS					1	
/*
#define SHADOW_RADIUS								(INT32)( 10 * (FLOAT)VEHICLE_MODEL_SCALE / TERR_SCALE )
#define SHADOW_RADIUS_SQRD					( SHADOW_RADIUS * SHADOW_RADIUS )
#define SHADOW_SOLID_RADIUS_SQRD		( SHADOW_RADIUS_SQRD / 2 )
#define SHADOW_WIDTH								( SHADOW_RADIUS * 2 + 1 )
#define SHADOW_AREA									SHADOW_WIDTH * SHADOW_WIDTH
*/
const D3DCOLORVALUE VEHICLE_EXHAUST_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f };
#define RACER_EXHAUST_WARMUP_RATE						2.0f
#define RACER_EXHAUST_COOLDOWN_RATE					0.5f

#define RACER_SHIELD_FADE_RATE			0.333333f // 



#define RACER_NUM_LIFT_THRUSTERS						9
#define RACER_LIFTTHRUST_MAXDIST						28.0f
#define RACER_LIFTTHRUST_MINDIST						1.5f // 1.05f
#define RACER_LIFTTHRUST_MAXACCEL						46.0f // 13.7f
#define RACER_LIFTTHRUST_MAXPITCHROLLACCEL	0.07f // 0.12f // dependent on maxaccel
#define RACER_LIFTTHRUST_SHOCKABSORB				1.0 // 0.3f // 0.7f // range from 0 (bouncy) to 1 (smooth)
#define RACER_LIFTTHRUST_CONE								3.0f // 3.0f // height of cone (base radius = 1) 
#define TERRAIN_COLLISION_ELASTICITY				1.5f // range from 1 (0%) to 2 (100%)

#define RACER_COLLISION_ELASTICITY					0.75 // range from 0 to 1

#define RACER_MAXLOOKPITCH					0.4f * g_PI
#define RACER_MAXPITCH							100000.0f //( 0.4f * g_PI )
#define RACER_MAXROLL								100000.0f //( 0.4f * g_PI )

#define RACER_MAXTURNRATE						g_PI
#define RACER_MAXPITCHRATE					( 0.75f * g_PI )
#define RACER_MAXROLLRATE						( 0.75f * g_PI )
#define RACER_MAXVEL								45.0f // 39.5f
#define RACER_REAL_MAXVEL						100.0f // 79.0f
#define RACER_DRAG									0.7f
#define RACER_PITCHADJUST						-4.0f
#define RACER_MAXACCEL							30.0f // ( RACER_MAXVEL * RACER_DRAG )

#define PLAYERPITCHASSIST_INCLINATION			0.22f * g_PI
#define PLAYERPITCHASSIST_RECHARGERATE		1.0f

#define RACER_AI_MAX_INCLINATION					0.24f * g_PI
#define RACER_AI_MAX_DOWNWARD_VELOCITY		-14.0f
#define RACER_AI_PITCHROLL_EPSILON				0.05f * g_PI
#define RACER_AI_INCLINATION_GROUNDCLEARANCE	5.0f

#define RACER_AI_NAV_GROUNDCLEARANCE			28.0f
#define RACER_AI_PATHING_VELOCITYWEIGHT		0.5f // units are seconds
//#define RACER_AI_PATHING_LAZINESS					5.0f // promotes "nearer" paths over shorter ones
#define RACER_AI_PATHING_RANDOMNESS				4.0f

#define RACER_AI_HANDICAP_POSITION					0.55f
#define RACER_AI_HANDICAP_LEAD							0.3f
#define RACER_AI_HANDICAP_LEADDIST					30.0f
#define RACER_AI_HANDICAP_SWERVE						0.35f

#define AIBEHAVIOR_RACE		0
#define AIBEHAVIOR_HUNT		1
#define AIBEHAVIOR_FLEE		2



struct RACERINPUT {
	FLOAT		turn;
	FLOAT		pitch;
	FLOAT		roll;
	FLOAT		lookPitch;
	INT			accel;
};



struct SAVEDTERRCOORD { 
	BYTE lightIntensity;
	BYTE twoTriCodes;
};




class CRacer {
public:
	DWORD						racerNum;

	RACERINPUT			input;

	D3DVECTOR				pos;
	D3DVECTOR				vel;
	FLOAT						yaw, pitch, roll;
	FLOAT						lookPitch;
	D3DMATRIX				matRot;
	FLOAT						liftThrustDistances[ RACER_NUM_LIFT_THRUSTERS ];

	INT32						currLap;
	DWORD						racePosition;
	FLOAT						handicap; // range: 1 (slow) to 0 (max speed)
	DWORD						currNavCoord;
	DWORD						previousNavCoord;
	DWORD						targetNavCoord;
	INT32						distToStartCoord;

	FLOAT						thrustIntensity;
	FLOAT						liftIntensity;

	CHAR						vehicleModelName[ 32 ];
	D3DVECTOR				vehicleModelRadius;
	FLOAT						vehicleModelMaxRadius;
	D3DCOLOR*				meshVertexEmissiveColors[ MAX_MODEL_MESHES ];
	FLOAT*					meshVertexShield[ MAX_MODEL_MESHES ];
	D3DDRAWPRIMITIVESTRIDEDDATA meshStridedData[ MAX_MODEL_MESHES ];

	HRESULT	Init( D3DVECTOR, D3DVECTOR, FLOAT, FLOAT, FLOAT, FLOAT, DWORD );

	VOID		GetAIInput();
	VOID		ProcessInput( FLOAT );
	VOID		SetRotationMatrix();
	VOID		RacerCollide( FLOAT, CRacer* );
	VOID		InflictShieldDamage( D3DVECTOR&, FLOAT );
	VOID		Move( FLOAT, TRACK* );
	VOID		UpdateShield( FLOAT );
	VOID		UpdateSounds();
	HRESULT	Render( LPDIRECT3DDEVICE7 );

	CRacer();
};
