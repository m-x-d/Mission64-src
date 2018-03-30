/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2000-2002 Mr. Hyde and Mad Dog

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
==============================================================================

flyer

==============================================================================
*/

#include "g_local.h"
#include "m_flyer.h"

qboolean visible (edict_t *self, edict_t *other);

static int	nextmove;			// Used for start/stop frames

static int	sound_sight;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_slash;
static int	sound_sproing;
static int	sound_die;
static int	sound_suicide_init; //mxd
static int	sound_suicide_beep; //mxd


void flyer_check_melee(edict_t *self);
void flyer_loop_melee (edict_t *self);
void flyer_melee (edict_t *self);
void flyer_setstart (edict_t *self);
void flyer_stand (edict_t *self);
void flyer_nextmove (edict_t *self);

//mxd. ROGUE - kamikaze stuff
void flyer_kamikaze(edict_t *self);
void flyer_kamikaze_check(edict_t *self);
void flyer_kamikaze_effect(edict_t *self);
void flyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void flyer_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void flyer_idle (edict_t *self)
{
	if(!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void flyer_pop_blades (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}


mframe_t flyer_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t	flyer_move_stand = {FRAME_stand01, FRAME_stand45, flyer_frames_stand, NULL};


mframe_t flyer_frames_walk [] =
{
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL
};
mmove_t	flyer_move_walk = {FRAME_stand01, FRAME_stand45, flyer_frames_walk, NULL};

int flyer_frames_run_distances_per_skill[] = { 10, 15, 20, 25 }; //mxd. Run distances per skill

mframe_t flyer_frames_run [] =
{
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL,
	ai_run, 10, NULL
};
mmove_t	flyer_move_run = {FRAME_stand01, FRAME_stand45, flyer_frames_run, NULL};

//mxd
mframe_t flyer_frames_kamizake[] =
{
	ai_charge, 40,	flyer_kamikaze_check,
	ai_charge, 40,	flyer_kamikaze_check,
	ai_charge, 40,	flyer_kamikaze_check,
	ai_charge, 40,	flyer_kamikaze_check,
	ai_charge, 40,	flyer_kamikaze_check
};
mmove_t flyer_move_kamikaze = { FRAME_rollr02, FRAME_rollr06, flyer_frames_kamizake, flyer_kamikaze };

//mxd
mframe_t flyer_frames_kamikaze_start[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, flyer_kamikaze_effect,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t flyer_move_kamikaze_start = { FRAME_defens01, FRAME_defens06, flyer_frames_kamikaze_start, flyer_kamikaze };

void flyer_run (edict_t *self)
{
	if(self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE) //mxd
		self->monsterinfo.currentmove = &flyer_move_kamikaze;
	else if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &flyer_move_stand;
	else
		self->monsterinfo.currentmove = &flyer_move_run;
}

void flyer_walk (edict_t *self)
{
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE) //mxd
		flyer_run(self);
	else
		self->monsterinfo.currentmove = &flyer_move_walk;
}

void flyer_stand (edict_t *self)
{
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE) //mxd
		flyer_run(self);
	else
		self->monsterinfo.currentmove = &flyer_move_stand;
}

//mxd. ROGUE - kamikaze stuff
void flyer_kamikaze_explode(edict_t *self)
{
	vec3_t dir;

	if (self->enemy)
	{
		VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
		//T_Damage(self->enemy, self, self, dir, self->s.origin, vec3_origin, 50, 50, DAMAGE_RADIUS, MOD_EXPLOSIVE);
		T_RadiusDamage(self, self, 45 + 5*skill->value, NULL, 128 + 16*skill->value, MOD_EXPLOSIVE, -2.0 / (4.0 + skill->value)); //mxd. We can explode when stuck, so no direct damage to the target
	}

	flyer_die(self, NULL, NULL, 0, dir);
}

//mxd
void flyer_kamikaze_effect(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_suicide_init, 1, ATTN_NORM, 0);

	vec3_t dir = { crandom(), crandom(), crandom() };
	M_SpawnEffect(self, TE_BLASTER, vec3_origin, dir);
}

void flyer_kamikaze(edict_t *self)
{
	self->monsterinfo.currentmove = &flyer_move_kamikaze;
}

void flyer_kamikaze_check(edict_t *self)
{
	// PMM - this needed because we could have gone away before we get here (blocked code)
	if (!self->inuse) return;

	if(!self->enemy || !self->enemy->inuse || VectorCompare(self->s.old_origin, self->s.origin)) //mxd. Also explode when stuck...
	{
		flyer_kamikaze_explode(self);
		return;
	}

	self->goalentity = self->enemy;
	self->s.effects |= EF_ROCKET;
	VectorCopy(self->s.origin, self->s.old_origin); //mxd. Store old position...

	//mxd. Shrink bounding box to reduce the chance of getting STUK...
	if(self->maxs[0] != 8)
	{
		VectorSet(self->mins, -8, -8, -8);
		VectorSet(self->maxs, 8, 8, 8);
		gi.linkentity(self);
	}

	float dist = realrange(self, self->enemy);
	if(dist < 90)
		flyer_kamikaze_explode(self);
	else if(dist < 128 || (level.framenum % 2 == 0)) //mxd. Play beep sound
		gi.sound(self, CHAN_VOICE, sound_suicide_beep, 1, ATTN_NORM, 0);
}
// ROGUE - kamikaze stuff end

mframe_t flyer_frames_start [] =
{
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	flyer_nextmove
};
mmove_t flyer_move_start = {FRAME_start01, FRAME_start06, flyer_frames_start, NULL};

mframe_t flyer_frames_stop [] =
{
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	NULL,
		ai_move, 0,	flyer_nextmove
};
mmove_t flyer_move_stop = {FRAME_stop01, FRAME_stop07, flyer_frames_stop, NULL};

void flyer_stop (edict_t *self)
{
		self->monsterinfo.currentmove = &flyer_move_stop;
}

void flyer_start (edict_t *self)
{
		self->monsterinfo.currentmove = &flyer_move_start;
}


mframe_t flyer_frames_rollright [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_rollright = {FRAME_rollr01, FRAME_rollr09, flyer_frames_rollright, NULL};

mframe_t flyer_frames_rollleft [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_rollleft = {FRAME_rollf01, FRAME_rollf09, flyer_frames_rollleft, NULL};

mframe_t flyer_frames_pain3 [] =
{	
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_pain3 = {FRAME_pain301, FRAME_pain304, flyer_frames_pain3, flyer_run};

mframe_t flyer_frames_pain2 [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_pain2 = {FRAME_pain201, FRAME_pain204, flyer_frames_pain2, flyer_run};

mframe_t flyer_frames_pain1 [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_pain1 = {FRAME_pain101, FRAME_pain109, flyer_frames_pain1, flyer_run};

mframe_t flyer_frames_defense [] = 
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,		// Hold this frame
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_defense = {FRAME_defens01, FRAME_defens06, flyer_frames_defense, NULL};

mframe_t flyer_frames_bankright [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_bankright = {FRAME_bankr01, FRAME_bankr07, flyer_frames_bankright, NULL};

mframe_t flyer_frames_bankleft [] =
{
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL,
		ai_move, 0, NULL
};
mmove_t flyer_move_bankleft = {FRAME_bankl01, FRAME_bankl07, flyer_frames_bankleft, NULL};		


void flyer_fire (edict_t *self, int flash_number)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	end;
	vec3_t	dir;
	int		effect;

	if ((self->s.frame == FRAME_attak204) || (self->s.frame == FRAME_attak207) || (self->s.frame == FRAME_attak210))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	/*if(self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}*/
	AdjustAccuracy(self, end); //mxd. Fog & Invisibility mode adjustments

	VectorSubtract (end, start, dir);
	monster_fire_blaster (self, start, dir, 1, 1000, flash_number, effect, BLASTER_ORANGE);
}

void flyer_fireleft (edict_t *self)
{
	flyer_fire (self, MZ2_FLYER_BLASTER_1);
}

void flyer_fireright (edict_t *self)
{
	flyer_fire (self, MZ2_FLYER_BLASTER_2);
}


mframe_t flyer_frames_attack2 [] =
{
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, -10, flyer_fireleft,			// left gun
		ai_charge, -10, flyer_fireright,		// right gun
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_attack2 = {FRAME_attak201, FRAME_attak217, flyer_frames_attack2, flyer_run};

//mxd. Circle-strafe frames
mframe_t flyer_frames_attack3[] =
{
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, flyer_fireleft,		// left gun
	ai_charge, 10, flyer_fireright,		// right gun
	ai_charge, 10, flyer_fireleft,		// left gun
	ai_charge, 10, flyer_fireright,		// right gun
	ai_charge, 10, flyer_fireleft,		// left gun
	ai_charge, 10, flyer_fireright,		// right gun
	ai_charge, 10, flyer_fireleft,		// left gun
	ai_charge, 10, flyer_fireright,		// right gun
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, NULL,
	ai_charge, 10, NULL
};
mmove_t flyer_move_attack3 = { FRAME_attak201, FRAME_attak217, flyer_frames_attack3, flyer_run };

void flyer_slash_left (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->mins[0], 0);
	fire_hit (self, aim, 5, 0);
	gi.sound (self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

void flyer_slash_right (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->maxs[0], 0);
	fire_hit (self, aim, 5, 0);
	gi.sound (self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

mframe_t flyer_frames_start_melee [] =
{
		ai_charge, 0, flyer_pop_blades,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_start_melee = {FRAME_attak101, FRAME_attak106, flyer_frames_start_melee, flyer_loop_melee};

mframe_t flyer_frames_end_melee [] =
{
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t flyer_move_end_melee = {FRAME_attak119, FRAME_attak121, flyer_frames_end_melee, flyer_run};


mframe_t flyer_frames_loop_melee [] =
{
		ai_charge, 0, NULL,		// Loop Start
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_slash_left,		// Left Wing Strike
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, flyer_slash_right,	// Right Wing Strike
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL		// Loop Ends
		
};
mmove_t flyer_move_loop_melee = {FRAME_attak107, FRAME_attak118, flyer_frames_loop_melee, flyer_check_melee};

void flyer_loop_melee (edict_t *self)
{
/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	self->monsterinfo.currentmove = &flyer_move_loop_melee;
}



void flyer_attack (edict_t *self)
{
	//mxd
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE)
	{
		flyer_run(self);
		return;
	}

	//mxd. Added circle-strafe attack
	float chance = (!skill->value ? 0.0f : 1.0f - (0.5f / skill->value));
	if (random() > chance)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
		self->monsterinfo.currentmove = &flyer_move_attack2;
	}
	else // Circle-strafe
	{
		if (random() <= 0.5) // Switch directions
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
		self->monsterinfo.attack_state = AS_SLIDING;
		self->monsterinfo.currentmove = &flyer_move_attack3;
	}
}

void flyer_setstart (edict_t *self)
{
	nextmove = ACTION_run;
	self->monsterinfo.currentmove = &flyer_move_start;
}

void flyer_nextmove (edict_t *self)
{
	if (nextmove == ACTION_attack1)
		self->monsterinfo.currentmove = &flyer_move_start_melee;
	else if (nextmove == ACTION_attack2)
		self->monsterinfo.currentmove = &flyer_move_attack2;
	else if (nextmove == ACTION_run)
		self->monsterinfo.currentmove = &flyer_move_run;
}

void flyer_melee (edict_t *self)
{
//	flyer.nextmove = ACTION_attack1;
//	self->monsterinfo.currentmove = &flyer_move_stop;

	// mxd
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE)
		flyer_run(self);
	else
		self->monsterinfo.currentmove = &flyer_move_start_melee;
}

void flyer_check_melee(edict_t *self)
{
	if (range (self, self->enemy) == RANGE_MELEE)
		if (random() <= 0.8)
			self->monsterinfo.currentmove = &flyer_move_loop_melee;
		else
			self->monsterinfo.currentmove = &flyer_move_end_melee;
	else
		self->monsterinfo.currentmove = &flyer_move_end_melee;
}

void flyer_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	//mxd. Kamikazes don't feel pain
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE)
		return;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	int n = rand() % 3;
	if (n == 0)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain1;
	}
	else if (n == 1)
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain2;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &flyer_move_pain3;
	}
}

//mxd
void flyer_spawn_gibs(edict_t *self, int damage)
{
	int n;
	for (n = 0; n < 6; n++)
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
	for (n = 0; n < 4; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowGib(self, "models/objects/gibs/skull/tris.md2", damage, GIB_ORGANIC); //mxd
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);
}

//mxd
void fake_flyer_touch(edict_t *self, edict_t *other, cplane_t* p, csurface_t* s)
{
	T_RadiusDamage(self, self, 45 + 5 * skill->value, NULL, 128 + 16 * skill->value, MOD_EXPLOSIVE, -2.0 / (4.0 + skill->value));
	flyer_spawn_gibs(self, self->dmg); // Removes self, must be called last
}

void flyer_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	//mxd
	if(self->class_id == ENTITY_MONSTER_FLYER && damage < 100)
	{
		int scaler = 32;

		// Single shotgun pellet does little damage
		if(inflictor->client && (inflictor->client->pers.weapon->classname == "weapon_shotgun" || inflictor->client->pers.weapon->classname == "weapon_supershotgun"))
			scaler *= 2;
		// Rockets and grenades, on the other hand, do way too much...
		else if(inflictor->classname == "rocket" || inflictor->classname == "grenade") 
			scaler *= 0.6f;

		int kick = max(300, min(damage * scaler, 600)) + rand() % 63;

		// I'm grenade!
		edict_t* g = ThrowGib(self, "models/monsters/flyer/tris.md2", damage, GIB_METALLIC);
		g->s.skinnum = self->s.skinnum;
		g->s.effects = EF_GRENADE;
		g->mass = 200;
		g->health = 100000; // Otherwise we will be destroyed by our own radius damage before spawning any gibs...
		VectorCopy(self->s.origin, g->s.origin);
		VectorCopy(self->velocity, g->velocity);
		VectorCopy(self->s.angles, g->s.angles);
		VectorScale(g->avelocity, 0.5f, g->avelocity);
		g->dmg = kick;
		g->touch = fake_flyer_touch;

		// Set clipping and size, so we can SMACK into other monsters (and player)
		g->solid = SOLID_BBOX;
		g->clipmask = MASK_SHOT;
		VectorSet(g->mins, -8, -8, -8);
		VectorSet(g->maxs, 8, 8, 8);
		gi.linkentity(g);

		// Let's launch it
		vec3_t kick_dir; 
		VectorSubtract(self->s.origin, attacker->s.origin, kick_dir);
		VectorNormalize(kick_dir);
		VectorMA(g->velocity, kick, kick_dir, g->velocity);

		G_FreeEdict(self);
	}
	else
	{
		flyer_spawn_gibs(self, damage);
	}
}

//mxd. Kamikaze code .. blow up if blocked
qboolean flyer_blocked(edict_t *self, float dist)
{
	if (self->class_id == ENTITY_MONSTER_FLYER_KAMIKAZE)
	{
		flyer_kamikaze_check(self);

		// if the above didn't blow us up (i.e. I got blocked by the player)
		if (self->inuse)
		{
			vec3_t origin;

			VectorMA(self->s.origin, -0.02, self->velocity, origin);
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_ROCKET_EXPLOSION);
			gi.WritePosition(origin);
			gi.multicast(self->s.origin, MULTICAST_PHS);

			G_FreeEdict(self);
		}

		return true;
	}

	// We're a normal flyer
	if (!self->enemy || !(self->enemy->client) || random() < 0.25 + (0.05 * skill->value)) return false; // Very stripped version of blocked_checkshot :)

	return true;
}

//mxd
void flyer_become_kamikaze(edict_t *self)
{
	self->health = min(self->health, 5 + 5*skill->value); // Allow player to one-shot the guy with any weapon, unless on Hard...
	self->common_name = "Angry Videogame Flyer"; // Le puns
	self->mass = 100; // Why is that needed?..
	self->class_id = ENTITY_MONSTER_FLYER_KAMIKAZE;
	self->monsterinfo.attack_state = AS_STRAIGHT; // No strafing here
	self->monsterinfo.currentmove = &flyer_move_kamikaze_start; // Abort currect attack
}
	

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_flyer (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// fix a map bug in jail5.bsp
	if (!Q_stricmp(level.mapname, "jail5") && (self->s.origin[2] == -104))
	{
		self->targetname = self->target;
		self->target = NULL;
	}

	sound_sight = gi.soundindex ("flyer/flysght1.wav");
	sound_idle = gi.soundindex ("flyer/flysrch1.wav");
	sound_pain1 = gi.soundindex ("flyer/flypain1.wav");
	sound_pain2 = gi.soundindex ("flyer/flypain2.wav");
	sound_slash = gi.soundindex ("flyer/flyatck2.wav");
	sound_sproing = gi.soundindex ("flyer/flyatck1.wav");
	sound_die = gi.soundindex ("flyer/flydeth1.wav");
	sound_suicide_init = gi.soundindex("flyer/suicide_init.wav"); //mxd
	sound_suicide_beep = gi.soundindex("flyer/suicide_beep.wav"); //mxd

	gi.soundindex ("flyer/flyatck3.wav");

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/flyer/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex ("models/monsters/flyer/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 16);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->classname = "monster_flyer"; //mxd
	self->class_id = ENTITY_MONSTER_FLYER; //mxd

	self->s.sound = gi.soundindex ("flyer/flyidle1.wav");

	// Lazarus: mapper-configurable health
	if(!self->health) self->health = 50;
	if(!self->mass)   self->mass = 50;

	self->pain = flyer_pain;
	self->die = flyer_die;

	self->monsterinfo.stand = flyer_stand;
	self->monsterinfo.walk = flyer_walk;
	self->monsterinfo.run = flyer_run;
	self->monsterinfo.attack = flyer_attack;
	self->monsterinfo.melee = flyer_melee;
	self->monsterinfo.sight = flyer_sight;
	self->monsterinfo.idle = flyer_idle;
	self->monsterinfo.blocked = flyer_blocked; //mxd

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 3; //sparks and blood

	// Lazarus
	if(self->powerarmor)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}
	self->common_name = "Flyer";

	//mxd. Adjust run speed based on skill
	int s = flyer_frames_run_distances_per_skill[max(0, min(3, skill->integer))];
	for(int i = 0; i < 45; i++)
	{
		flyer_frames_run[i].dist = s;
	}

	gi.linkentity (self);

	self->monsterinfo.currentmove = &flyer_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
