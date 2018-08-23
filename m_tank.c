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

TANK

==============================================================================
*/

#include "g_local.h"
#include "m_tank.h"


void tank_refire_rocket(edict_t *self);
void tank_doattack_rocket(edict_t *self);
void tank_reattack_blaster(edict_t *self);

static int	sound_thud;
static int	sound_pain;
static int	sound_idle;
static int	sound_die;
static int	sound_step;
static int	sound_sight;
static int	sound_windup;
static int	sound_strike;

//
// misc
//

//mxd. 32 frames.
vec3_t tank_bleed_positions[][2] =
{
	// x, y, z, nx, ny, nz
	{{-13.737, 7.487, 48.963}, {-0.449, 0.639, 0.624}}, // Frame 222
	{{-15.639, 15.196, 45.571}, {-0.584, 0.386, 0.714}}, // Frame 223
	{{-13.459, 12.627, 45.747}, {-0.518, 0.533, 0.668}}, // Frame 224
	{{-10.057, 2.937, 47.546}, {-0.164, 0.872, 0.461}}, // Frame 225
	{{2.708, -5.499, 44.988}, {0.428, 0.903, 0.038}}, // Frame 226
	{{13.522, -1.689, 35.976}, {0.747, 0.519, -0.417}}, // Frame 227
	{{17.731, 7.344, 26.636}, {0.71, 0.146, -0.689}}, // Frame 228
	{{17.232, 9.804, 22.892}, {0.622, 0.033, -0.782}}, // Frame 229
	{{17.313, 6.365, 22.869}, {0.635, 0.033, -0.772}}, // Frame 230
	{{18.881, 1.307, 23.725}, {0.645, 0.117, -0.755}}, // Frame 231
	{{19.305, -4.529, 25.098}, {0.66, 0.222, -0.718}}, // Frame 232
	{{18.449, -9.627, 27.366}, {0.687, 0.313, -0.655}}, // Frame 233
	{{17.306, -12.755, 29.438}, {0.712, 0.383, -0.589}}, // Frame 234
	{{18.267, -12.446, 32.967}, {0.781, 0.393, -0.486}}, // Frame 235
	{{17.266, -10.264, 36.858}, {0.871, 0.386, -0.305}}, // Frame 236
	{{15.903, -6.849, 40.285}, {0.939, 0.327, -0.103}}, // Frame 237
	{{17.235, -3.527, 41.589}, {0.958, 0.284, 0.047}}, // Frame 238
	{{16.594, -1.348, 41.833}, {0.956, 0.268, 0.119}}, // Frame 239
	{{16.768, -2.128, 40.479}, {0.944, 0.311, 0.113}}, // Frame 240
	{{15.904, -4.529, 38.288}, {0.914, 0.398, 0.082}}, // Frame 241
	{{14.544, -7.942, 36.242}, {0.846, 0.533, 0.013}}, // Frame 242
	{{11.965, -10.223, 34.781}, {0.76, 0.65, -0.018}}, // Frame 243
	{{8.244, -8.428, 36.305}, {0.607, 0.792, -0.062}}, // Frame 244
	{{5.821, -6.051, 39.136}, {0.494, 0.868, -0.056}}, // Frame 245
	{{1.121, -7.033, 41.131}, {0.491, 0.867, 0.085}}, // Frame 246
	{{-5.768, -8.668, 39.862}, {0.43, 0.857, 0.284}}, // Frame 247
	{{-12.451, -10.375, 33.847}, {0.247, 0.863, 0.44}}, // Frame 248
	{{-20.484, -10.657, 0.734}, {0.102, 0.923, 0.37}}, // Frame 249
	{{-19.72, -10.455, 0.579}, {0.099, 0.921, 0.376}}, // Frame 250
	{{-19.438, -10.401, 0.579}, {0.105, 0.923, 0.371}}, // Frame 251
	{{-19.662, -10.46, 0.579}, {0.107, 0.924, 0.368}}, // Frame 252
	{{-19.702, -10.369, -2.158}, {0.097, 0.922, 0.376}}, // Frame 253
};

//mxd. Spawn some blood using precalculated positions...
void tank_bleed(edict_t *self)
{
	if (random() < 0.7f)
	{
		const int i = self->s.frame - FRAME_death101;
		M_SpawnEffect(self, (random() > 0.7f ? TE_MOREBLOOD : TE_BLOOD), tank_bleed_positions[i][0], tank_bleed_positions[i][1]);
	}
}

void tank_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}


void tank_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

vec3_t tank_impact_positions[] =
{
	{-23.9, -11.75, -15},
	{-11.6, -10.66, -15},
	{-22.19, 27.43, -15},
	{-35.87, 1.38, -15},
	{-48.17, 19.33, -15},
	{-2.71, 34.87, -15},
	{2.07, 18.02, -15},
	{0.02, -10, -15},
};

//mxd
void tank_impact(edict_t *self)
{
	vec3_t normal;
	VectorSet(normal, 0, 0, 1);

	for (int i = 0; i < 8; i++)
		M_SpawnEffect(self, TE_CHAINFIST_SMOKE, tank_impact_positions[i], normal);
	
	// Also BLEED!
	tank_bleed(self);
}

void tank_thud(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);

	//mxd. Also BLEED!
	tank_bleed(self);
}

void tank_windup(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

void tank_idle(edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


//
// stand
//

mframe_t tank_frames_stand[]=
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}
};
mmove_t	tank_move_stand = {FRAME_stand01, FRAME_stand30, tank_frames_stand, NULL};
	
void tank_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &tank_move_stand;
}


//
// walk
//

void tank_walk(edict_t *self);

mframe_t tank_frames_start_walk[] =
{
	{ai_walk,  0, NULL},
	{ai_walk,  6, NULL},
	{ai_walk,  6, NULL},
	{ai_walk, 11, tank_footstep}
};
mmove_t	tank_move_start_walk = {FRAME_walk01, FRAME_walk04, tank_frames_start_walk, tank_walk};

mframe_t tank_frames_walk[] =
{
	{ai_walk, 4,	NULL},
	{ai_walk, 5,	NULL},
	{ai_walk, 3,	NULL},
	{ai_walk, 2,	NULL},
	{ai_walk, 5,	NULL},
	{ai_walk, 5,	NULL},
	{ai_walk, 4,	NULL},
	{ai_walk, 4,	tank_footstep},
	{ai_walk, 3,	NULL},
	{ai_walk, 5,	NULL},
	{ai_walk, 4,	NULL},
	{ai_walk, 5,	NULL},
	{ai_walk, 7,	NULL},
	{ai_walk, 7,	NULL},
	{ai_walk, 6,	NULL},
	{ai_walk, 6,	tank_footstep}
};
mmove_t	tank_move_walk = {FRAME_walk05, FRAME_walk20, tank_frames_walk, NULL};

mframe_t tank_frames_stop_walk[] =
{
	{ai_walk,  3, NULL},
	{ai_walk,  3, NULL},
	{ai_walk,  2, NULL},
	{ai_walk,  2, NULL},
	{ai_walk,  4, tank_footstep}
};
mmove_t	tank_move_stop_walk = {FRAME_walk21, FRAME_walk25, tank_frames_stop_walk, tank_stand};

void tank_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &tank_move_walk;
}


//
// run
//

void tank_run(edict_t *self);

mframe_t tank_frames_start_run[] =
{
	{ai_run,  0, NULL},
	{ai_run,  6, NULL},
	{ai_run,  6, NULL},
	{ai_run, 11, tank_footstep}
};
mmove_t	tank_move_start_run = {FRAME_walk01, FRAME_walk04, tank_frames_start_run, tank_run};

mframe_t tank_frames_run[] =
{
	{ai_run, 4,	NULL},
	{ai_run, 5,	NULL},
	{ai_run, 3,	NULL},
	{ai_run, 2,	NULL},
	{ai_run, 5,	NULL},
	{ai_run, 5,	NULL},
	{ai_run, 4,	NULL},
	{ai_run, 4,	tank_footstep},
	{ai_run, 3,	NULL},
	{ai_run, 5,	NULL},
	{ai_run, 4,	NULL},
	{ai_run, 5,	NULL},
	{ai_run, 7,	NULL},
	{ai_run, 7,	NULL},
	{ai_run, 6,	NULL},
	{ai_run, 6,	tank_footstep}
};
mmove_t	tank_move_run = {FRAME_walk05, FRAME_walk20, tank_frames_run, NULL};

mframe_t tank_frames_stop_run[] =
{
	{ai_run,  3, NULL},
	{ai_run,  3, NULL},
	{ai_run,  2, NULL},
	{ai_run,  2, NULL},
	{ai_run,  4, tank_footstep}
};
mmove_t	tank_move_stop_run = {FRAME_walk21, FRAME_walk25, tank_frames_stop_run, tank_walk};

void tank_run(edict_t *self)
{
	if (self->enemy && self->enemy->client)
		self->monsterinfo.aiflags |= AI_BRUTAL;
	else
		self->monsterinfo.aiflags &= ~AI_BRUTAL;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &tank_move_stand;
		return;
	}

	if (self->monsterinfo.currentmove == &tank_move_walk || self->monsterinfo.currentmove == &tank_move_start_run)
		self->monsterinfo.currentmove = &tank_move_run;
	else
		self->monsterinfo.currentmove = &tank_move_start_run;
}

//
// pain
//

mframe_t tank_frames_pain1[] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t tank_move_pain1 = {FRAME_pain101, FRAME_pain104, tank_frames_pain1, tank_run};

mframe_t tank_frames_pain2[] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t tank_move_pain2 = {FRAME_pain201, FRAME_pain205, tank_frames_pain2, tank_run};

mframe_t tank_frames_pain3[] =
{
	{ai_move, -7, NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 2,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 3,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 2,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  NULL},
	{ai_move, 0,  tank_footstep}
};
mmove_t	tank_move_pain3 = {FRAME_pain301, FRAME_pain316, tank_frames_pain3, tank_run};


void tank_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < self->max_health / 2)
	{
		self->s.skinnum |= 1;
		if (!(self->fogclip & 2)) //custom bloodtype flag check
			self->blood_type = 3; //sparks and blood
	}

	if (damage <= 10)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	if (damage <= 30 && random() > 0.2)
		return;
	
	// If hard or nightmare, don't go into pain while attacking
	if (skill->value >= 2)
	{
		if (self->s.frame >= FRAME_attak301 && self->s.frame <= FRAME_attak330)
			return;
		if (self->s.frame >= FRAME_attak101 && self->s.frame <= FRAME_attak116)
			return;
	}

	self->pain_debounce_time = level.time + 3;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (damage <= 30)
		self->monsterinfo.currentmove = &tank_move_pain1;
	else if (damage <= 60)
		self->monsterinfo.currentmove = &tank_move_pain2;
	else
		self->monsterinfo.currentmove = &tank_move_pain3;
}


//
// attacks
//

void TankBlaster(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	int		flash_number;

	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->s.frame == FRAME_attak110)
		flash_number = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flash_number = MZ2_TANK_BLASTER_2;
	else // (self->s.frame == FRAME_attak116)
		flash_number = MZ2_TANK_BLASTER_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	/*if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}*/
	AdjustAccuracy(self, end); //mxd. Fog & Invisibility mode adjustments

	VectorSubtract(end, start, dir);
	monster_fire_blaster(self, start, dir, 30, 800, flash_number, EF_BLASTER, BLASTER_ORANGE);
}	

void TankStrike(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);
}	

void TankRocket(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		flash_number;
	// Lazarus: Added skill level-dependent rocket speed similar to Rogue pack
	int		rocketSpeed;

	// check if enemy went away
	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->s.frame == FRAME_attak324)
		flash_number = MZ2_TANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak327)
		flash_number = MZ2_TANK_ROCKET_2;
	else // (self->s.frame == FRAME_attak330)
		flash_number = MZ2_TANK_ROCKET_3;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	if (self->spawnflags & SF_MONSTER_SPECIAL)
		rocketSpeed = 400; // Lazarus: Homing rockets are tougher if slow
	else
		rocketSpeed = 500 + (100 * skill->value);

	// Lazarus: Added homers
	VectorCopy(self->enemy->s.origin, vec);
	if (random() < 0.66f || start[2] < self->enemy->absmin[2])
		vec[2] += self->enemy->viewheight;
	else
		vec[2] = self->enemy->absmin[2];

	// Lazarus fog reduction of accuracy
	/*if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		vec[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		vec[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		vec[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}*/
	AdjustAccuracy(self, vec); //mxd. Fog & Invisibility mode adjustments

	VectorSubtract(vec, start, dir);
	// lead target, but not if using homers
	// 20, 35, 50, 65 chance of leading
	// Lazarus: Switched this around from Rogue code... it led target more often for Easy, which seemed backwards
	if ( (random() < (0.2 + skill->value * 0.15) ) && !(self->spawnflags & SF_MONSTER_SPECIAL))
	{
		const float dist = VectorLength(dir);
		const float time = dist / rocketSpeed;
		VectorMA(vec, time, self->enemy->velocity, vec);
		VectorSubtract(vec, start, dir);
	}

	VectorNormalize(dir);
	// paranoia, make sure we're not shooting a target right next to us
	const trace_t trace = gi.trace(start, vec3_origin, vec3_origin, vec, self, MASK_SHOT);
	if (trace.ent == self->enemy || trace.ent == world)
	{
		if (trace.fraction > 0.5 || (trace.ent && trace.ent->client))
			monster_fire_rocket(self, start, dir, 50, rocketSpeed, flash_number, (self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL));
	}
}	

void TankMachineGun(edict_t *self)
{
	vec3_t	dir;
	vec3_t	vec;
	vec3_t	start;
	vec3_t	forward, right;

	// check if enemy went away
	if (!self->enemy || !self->enemy->inuse)
		return;

	const int flash_number = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	if (self->enemy)
	{
		VectorCopy(self->enemy->s.origin, vec);
		vec[2] += self->enemy->viewheight;

		// Lazarus fog reduction of accuracy
		/*if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
		{
			vec[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			vec[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			vec[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		}*/
		AdjustAccuracy(self, vec); //mxd. Fog & Invisibility mode adjustments

		VectorSubtract(vec, start, vec);
		vectoangles(vec, vec);
		dir[0] = vec[0];
	}
	else
	{
		dir[0] = 0;
	}

	if (self->s.frame <= FRAME_attak415)
		dir[1] = self->s.angles[1] - 8 * (self->s.frame - FRAME_attak411);
	else
		dir[1] = self->s.angles[1] + 8 * (self->s.frame - FRAME_attak419);

	dir[2] = 0;

	AngleVectors(dir, forward, NULL, NULL);

	monster_fire_bullet(self, start, forward, 20, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}	

mframe_t tank_frames_attack_blast[] =
{
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, -1,	NULL},
	{ai_charge, -2,	NULL},
	{ai_charge, -1,	NULL},
	{ai_charge, -1,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	TankBlaster},	// 10
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	TankBlaster},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	TankBlaster} // 16
};
mmove_t tank_move_attack_blast = {FRAME_attak101, FRAME_attak116, tank_frames_attack_blast, tank_reattack_blaster};

mframe_t tank_frames_reattack_blast[] =
{
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	TankBlaster},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	NULL},
	{ai_charge, 0,	TankBlaster} // 16
};
mmove_t tank_move_reattack_blast = {FRAME_attak111, FRAME_attak116, tank_frames_reattack_blast, tank_reattack_blaster};

mframe_t tank_frames_attack_post_blast[] =
{
	{ai_move, 0,	NULL},			// 17
	{ai_move, 0,	NULL},
	{ai_move, 2,	NULL},
	{ai_move, 3,	NULL},
	{ai_move, 2,	NULL},
	{ai_move, -2,	tank_footstep} // 22
};
mmove_t tank_move_attack_post_blast = {FRAME_attak117, FRAME_attak122, tank_frames_attack_post_blast, tank_run};

void tank_reattack_blaster(edict_t *self)
{
	if (skill->value >= 2 && random() <= 0.6 && self->enemy->health > 0 && visible(self, self->enemy))
		self->monsterinfo.currentmove = &tank_move_reattack_blast;
	else
		self->monsterinfo.currentmove = &tank_move_attack_post_blast;
}


void tank_poststrike(edict_t *self)
{
	self->enemy = NULL;
	tank_run(self);
}

mframe_t tank_frames_attack_strike[] =
{
	{ai_move, 3,   NULL},
	{ai_move, 2,   NULL},
	{ai_move, 2,   NULL},
	{ai_move, 1,   NULL},
	{ai_move, 6,   NULL},
	{ai_move, 7,   NULL},
	{ai_move, 9,   tank_footstep},
	{ai_move, 2,   NULL},
	{ai_move, 1,   NULL},
	{ai_move, 2,   NULL},
	{ai_move, 2,   tank_footstep},
	{ai_move, 2,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, -2,  NULL},
	{ai_move, -2,  NULL},
	{ai_move, 0,   tank_windup},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   NULL},
	{ai_move, 0,   TankStrike},
	{ai_move, 0,   NULL},
	{ai_move, -1,  NULL},
	{ai_move, -1,  NULL},
	{ai_move, -1,  NULL},
	{ai_move, -1,  NULL},
	{ai_move, -1,  NULL},
	{ai_move, -3,  NULL},
	{ai_move, -10, NULL},
	{ai_move, -10, NULL},
	{ai_move, -2,  NULL},
	{ai_move, -3,  NULL},
	{ai_move, -2,  tank_footstep}
};
mmove_t tank_move_attack_strike = {FRAME_attak201, FRAME_attak238, tank_frames_attack_strike, tank_poststrike};

mframe_t tank_frames_attack_pre_rocket[] =
{
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},			// 10

	{ai_charge, 0,  NULL},
	{ai_charge, 1,  NULL},
	{ai_charge, 2,  NULL},
	{ai_charge, 7,  NULL},
	{ai_charge, 7,  NULL},
	{ai_charge, 7,  tank_footstep},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},			// 20

	{ai_charge, -3, NULL}
};
mmove_t tank_move_attack_pre_rocket = {FRAME_attak301, FRAME_attak321, tank_frames_attack_pre_rocket, tank_doattack_rocket};

mframe_t tank_frames_attack_fire_rocket[] =
{
	{ai_charge, -3, NULL},		// Loop Start	22 
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  TankRocket},// 24
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  TankRocket},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, -1, TankRocket} // 30	Loop End
};
mmove_t tank_move_attack_fire_rocket = {FRAME_attak322, FRAME_attak330, tank_frames_attack_fire_rocket, tank_refire_rocket};

mframe_t tank_frames_attack_post_rocket[] =
{
	{ai_charge, 0,  NULL},			// 31
	{ai_charge, -1, NULL},
	{ai_charge, -1, NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 2,  NULL},
	{ai_charge, 3,  NULL},
	{ai_charge, 4,  NULL},
	{ai_charge, 2,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},			// 40

	{ai_charge, 0,  NULL},
	{ai_charge, -9, NULL},
	{ai_charge, -8, NULL},
	{ai_charge, -7, NULL},
	{ai_charge, -1, NULL},
	{ai_charge, -1, tank_footstep},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},			// 50

	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL},
	{ai_charge, 0,  NULL}
};
mmove_t tank_move_attack_post_rocket = {FRAME_attak331, FRAME_attak353, tank_frames_attack_post_rocket, tank_run};

mframe_t tank_frames_attack_chain[] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{NULL,      0, TankMachineGun},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t tank_move_attack_chain = {FRAME_attak401, FRAME_attak429, tank_frames_attack_chain, tank_run};

void tank_refire_rocket(edict_t *self)
{
	// Only on hard or nightmare
	if (skill->value >= 2 && random() <= 0.4 && self->enemy->health > 0 && visible(self, self->enemy))
		self->monsterinfo.currentmove = &tank_move_attack_fire_rocket;
	else
		self->monsterinfo.currentmove = &tank_move_attack_post_rocket;
}

void tank_doattack_rocket(edict_t *self)
{
	self->monsterinfo.currentmove = &tank_move_attack_fire_rocket;
}

void tank_attack(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->enemy->health < 0)
	{
		self->monsterinfo.currentmove = &tank_move_attack_strike;
		self->monsterinfo.aiflags &= ~AI_BRUTAL;
		return;
	}

	vec3_t vec;
	VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
	const float range = VectorLength(vec);
	const float r = random();

	if (range <= 125)
	{
		if (r < 0.4)
			self->monsterinfo.currentmove = &tank_move_attack_chain;
		else 
			self->monsterinfo.currentmove = &tank_move_attack_blast;
	}
	else if (range <= 250)
	{
		if (r < 0.5)
			self->monsterinfo.currentmove = &tank_move_attack_chain;
		else
			self->monsterinfo.currentmove = &tank_move_attack_blast;
	}
	else
	{
		if (r < 0.33)
		{
			self->monsterinfo.currentmove = &tank_move_attack_chain;
		}
		else if (r < 0.66)
		{
			self->monsterinfo.currentmove = &tank_move_attack_pre_rocket;
			self->pain_debounce_time = level.time + 5.0;	// no pain for a while
		}
		else
		{
			self->monsterinfo.currentmove = &tank_move_attack_blast;
		}
	}
}


//
// death
//

void tank_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -16);
	VectorSet(self->maxs, 16, 16, -0);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
	M_FlyCheck(self);

	// Lazarus monster fade
	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think = FadeDieSink;
		self->nextthink = level.time + corpse_fadetime->value;
	}
}

//mxd. Skip death animation when touched by player
void tank_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!(other->client || (other->flags & FL_ROBOT) || (other->svflags & SVF_MONSTER)) || other->health <= 0)
		return;

	if (self->s.frame >= FRAME_death101 && self->s.frame < FRAME_death132 - 7)
	{
		// Skip animation
		self->monsterinfo.nextframe = FRAME_death132 - 7;

		// Play kick sound
		gi.sound(self, CHAN_BODY, gi.soundindex("weapons/kick.wav"), 1, ATTN_NORM, 0);
		if (other->client)
			PlayerNoise(other, other->s.origin, PNOISE_SELF);

		// Push player back a bit
		if (other->client)
		{
			vec3_t dir;

			VectorSubtract(other->s.origin, self->s.origin, dir);
			VectorNormalize(dir);
			VectorMA(other->velocity, 380, dir, other->velocity);

			// Also push the view up a bit
			other->client->kick_angles[0] -= 8 + random() * 6;
		}
	}
}

mframe_t tank_frames_death1[] =
{
	{ai_move, -7,  tank_bleed}, //mxd. Added tank_bleed
	{ai_move, -2,  tank_bleed},
	{ai_move, -2,  tank_bleed},
	{ai_move, 1,   tank_bleed},
	{ai_move, 3,   tank_bleed},
	{ai_move, 6,   tank_bleed},
	{ai_move, 1,   tank_bleed},
	{ai_move, 1,   tank_bleed},
	{ai_move, 2,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, -2,  tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, -3,  tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, -4,  tank_bleed},
	{ai_move, -6,  tank_bleed},
	{ai_move, -4,  tank_bleed},
	{ai_move, -5,  tank_bleed},
	{ai_move, -7,  tank_bleed},
	{ai_move, -15, tank_thud},
	{ai_move, -5,  tank_impact}, //mxd. Impact SFX
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed},
	{ai_move, 0,   tank_bleed}
};
mmove_t	tank_move_death = {FRAME_death101, FRAME_death132, tank_frames_death1, tank_dead};

void tank_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->s.skinnum |= 1;
	if (!(self->fogclip & 2)) //custom bloodtype flag check
		self->blood_type = 3; //sparks and blood

	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

	// check for gib
	if (self->health <= self->gib_health && !(self->spawnflags & SF_MONSTER_NOGIB))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		
		// Knightmare- more gibs
		for (int n = 0; n < 8; n++) //mxd. 16 -> 8
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		for (int n = 0; n < 12; n++) //mxd. 16 -> 12
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		for (int n = 0; n < 6; n++) //mxd. 8 -> 6
			ThrowGib(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);

		//mxd. Throw some MOAR custom gibs
		ThrowHead(self, "models/objects/gibs/tank/head.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/tank/arm_right.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/tank/rl_mount.md2", damage, GIB_METALLIC);

		vec3_t pos, vs, avs;
		VectorSet(vs, 0.3, 0.3, 0.3);
		VectorSet(avs, 0.0, 0.0, 0.0);

		VectorSet(pos, 0, self->mins[1] + 18, self->mins[2] + 24);
		VectorAdd(pos, self->s.origin, pos);
		edict_t * leg = ThrowGibEx(self, "models/objects/gibs/tank/leg_left.md2", damage, GIB_METALLIC, pos, NULL, vs, avs);
		if (leg) VectorCopy(self->s.angles, leg->s.angles);
		VectorSet(pos, 0, self->maxs[1] - 18, self->mins[2] + 24);
		VectorAdd(pos, self->s.origin, pos);
		leg = ThrowGibEx(self, "models/objects/gibs/tank/leg_right.md2", damage, GIB_METALLIC, pos, NULL, vs, avs);
		if (leg) VectorCopy(self->s.angles, leg->s.angles);

		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->touch = tank_touch; //mxd
	self->monsterinfo.currentmove = &tank_move_death;

	//mxd. Eject the arm
	vec3_t pos, localpos, normal, localnormal;

	VectorCopy(tank_bleed_positions[0][0], localpos);
	VectorCopy(tank_bleed_positions[0][1], localnormal);
	localnormal[0] += crandom() * 0.1f;

	// Add some velocity...
	localnormal[0] *= 3.0f;
	localnormal[1] *= 3.0f;

	// Translate spawn position to world space...
	PositionToWorld(self, localpos, pos);

	// Rotate normal to match model angles...
	NormalToWorld(self, localnormal, normal);

	ThrowGibEx(self, "models/objects/gibs/tank/arm_left.md2", damage, GIB_METALLIC, pos, normal, NULL, NULL);

	//mxd. Spawn a small explosion...
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(pos);
	gi.multicast(pos, MULTICAST_PVS);
}


//
// monster_tank
//

/*QUAKED monster_tank (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
/*QUAKED monster_tank_commander (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
void SP_monster_tank(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	// Lazarus: special purpose skins
	if (strcmp(self->classname, "monster_tank_commander") == 0)
		self->s.skinnum = 2;

	if (self->style)
	{
		PatchMonsterModel("models/monsters/tank/tris.md2");
		self->s.skinnum += self->style * 4;
	}

	self->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	VectorSet(self->mins, -32, -32, -16);
	VectorSet(self->maxs, 32, 32, 72);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_pain = gi.soundindex("tank/tnkpain2.wav");
	sound_thud = gi.soundindex("tank/tnkdeth2.wav");
	sound_idle = gi.soundindex("tank/tnkidle1.wav");
	sound_die = gi.soundindex("tank/death.wav");
	sound_step = gi.soundindex("tank/step.wav");
	sound_windup = gi.soundindex("tank/tnkatck4.wav");
	sound_strike = gi.soundindex("tank/tnkatck5.wav");
	sound_sight = gi.soundindex("tank/sight1.wav");

	gi.soundindex("tank/tnkatck1.wav");
	gi.soundindex("tank/tnkatk2a.wav");
	gi.soundindex("tank/tnkatk2b.wav");
	gi.soundindex("tank/tnkatk2c.wav");
	gi.soundindex("tank/tnkatk2d.wav");
	gi.soundindex("tank/tnkatk2e.wav");
	gi.soundindex("tank/tnkatck3.wav");

	if (strcmp(self->classname, "monster_tank_commander") == 0)
	{
		// Lazarus: mapper-configurable health
		if (!self->health)
			self->health = 1000;
		if (!self->gib_health)
			self->gib_health = -225;

		self->common_name = "Tank Commander";
	}
	else
	{
		// Lazarus: mapper-configurable health
		if (!self->health)
			self->health = 750;
		if (!self->gib_health)
			self->gib_health = -200;

		self->common_name = "Tank";
	}

	if (!self->mass)
		self->mass = 500;

	self->pain = tank_pain;
	self->die = tank_die;
	self->monsterinfo.stand = tank_stand;
	self->monsterinfo.walk = tank_walk;
	self->monsterinfo.run = tank_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = tank_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = tank_sight;
	self->monsterinfo.idle = tank_idle;

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 2; //sparks
	else
		self->fogclip |= 2; //custom bloodtype flag

	// Lazarus power armor
	if (self->powerarmor)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}

	if (!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.05;

	gi.linkentity(self);
	self->monsterinfo.currentmove = &tank_move_stand;
	
	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = { &tank_move_death, NULL };
		M_SetDeath(self, (mmove_t **)&deathmoves);
	}

	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}