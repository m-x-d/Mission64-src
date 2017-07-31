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

INFANTRY

==============================================================================
*/

#include "g_local.h"
#include "m_infantry.h"

void InfantryMachineGun (edict_t *self);


static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;

static int	sound_gunshot;
static int	sound_weapon_cock;
static int	sound_punch_swing;
static int	sound_punch_hit;
static int	sound_sight;
static int	sound_search;
static int	sound_idle;


mframe_t infantry_frames_stand [] =
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
	ai_stand, 0, NULL
};
mmove_t infantry_move_stand = {FRAME_stand50, FRAME_stand71, infantry_frames_stand, NULL};

void infantry_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_stand;
}


mframe_t infantry_frames_fidget [] =
{
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 6,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -2, NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL,
	ai_stand, -3, NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL
};
mmove_t infantry_move_fidget = {FRAME_stand01, FRAME_stand49, infantry_frames_fidget, infantry_stand};

void infantry_fidget (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_fidget;
	if(!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//mxd. 54 frames.
vec3_t infantry_bleed_positions[][2] =
{
	// x, y, z, nx, ny, nz
	{ -4.722, 0.083, 23.434, -0.208, 0.368, 0.906 }, // Frame 125
	{ -4.913, -1.692, 23.322, -0.253, 0.334, 0.908 }, // Frame 126
	{ -5.188, -3.52, 23.049, -0.14, 0.086, 0.986 }, // Frame 127
	{ -6.274, 0.656, 22.046, -0.218, 0.387, 0.896 }, // Frame 128
	{ -5.094, 4.887, 20.514, 0.145, 0.285, 0.947 }, // Frame 129
	{ -5.205, 4.745, 20.984, 0.14, 0.274, 0.951 }, // Frame 130
	{ -5.87, 4.172, 21.038, -0.299, 0.278, 0.913 }, // Frame 131
	{ -6.267, 3.602, 21.166, -0.183, 0.533, 0.826 }, // Frame 132
	{ -5.983, 2.93, 21.291, -0.024, 0.513, 0.858 }, // Frame 133
	{ -3.787, 3.072, 22.175, 0.195, 0.194, 0.961 }, // Frame 134
	{ 0.203, 2.497, 20.875, 0.614, 0.327, 0.718 }, // Frame 135
	{ 3.265, 1.72, 17.998, 0.899, 0.368, 0.24 }, // Frame 136
	{ 7.761, 1.572, 13.992, 0.894, 0.437, 0.1 }, // Frame 137
	{ 8.185, 0.733, 14.951, 0.69, 0.55, 0.47 }, // Frame 138
	{ 8.625, 1.1, 16.022, 0.52, 0.518, 0.679 }, // Frame 139
	{ 1.449, 6.383, 9.446, -0.156, 0.09, 0.984 }, // Frame 140
	{ 13.601, 1.04, -1.854, 0.85, 0.145, 0.506 }, // Frame 141
	{ 15.255, 5.221, -19.717, 0.974, 0.006, 0.226 }, // Frame 142
	{ 17.518, 5.759, -20.409, 0.979, 0.189, -0.079 }, // Frame 143
	{ 20.485, 6.008, -20.05, 0.953, 0.283, -0.108 }, // Frame 144
	{ -9.446, 0.029, 21.451, 0, 1, 0 }, // Frame 145
	{ -5.265, 1.806, 19.379, -0.057, 0.359, 0.932 }, // Frame 146
	{ -0.467, 3.214, 16.207, 0.717, 0.342, 0.608 }, // Frame 147
	{ 3.926, 3.268, 11.674, 0.909, 0.389, 0.146 }, // Frame 148
	{ 5.667, 1.71, 9.608, 0.909, 0.414, 0.041 }, // Frame 149
	{ 7.333, -0.761, 7.69, 0.918, 0.393, -0.052 }, // Frame 150
	{ 7.628, -3.145, 8.68, 0.893, 0.45, 0.017 }, // Frame 151
	{ 2.914, -6.246, 12.103, 0.779, 0.627, 0 }, // Frame 152
	{ -3.354, -8.956, 15.062, 0, 1, 0 }, // Frame 153
	{ -4.606, -8.184, 15.43, 0.816, 0.408, 0.408 }, // Frame 154
	{ -4.15, -7.312, 15.623, 0, 0, 0 }, // Frame 155
	{ -3.478, -6.4, 15.839, 0, 1, 0 }, // Frame 156
	{ -2.367, -5.596, 16.07, 0, 0, 0 }, // Frame 157
	{ -3.162, -4.647, 16.36, 0, 1, 0 }, // Frame 158
	{ -3.082, -3.797, 16.352, 0, 1, 0 }, // Frame 159
	{ -1.484, -2.545, 16.325, 0, 0, 0 }, // Frame 160
	{ -1.702, -1.324, 16.188, 1, 0, 0 }, // Frame 161
	{ -1.972, -2.4, 15.842, 0.74, 0.244, 0.627 }, // Frame 162
	{ -3.623, -3.048, 15.4, 0, 0, 1 }, // Frame 163
	{ -6.679, -2.962, 15.39, 0, 0, 0 }, // Frame 164
	{ -12.206, -2.921, 11.42, -1, 0, 0 }, // Frame 165
	{ -17.999, -3.464, -0.368, 0, 0, 0 }, // Frame 166
	{ -15.849, -3.857, -12.581, 0, 0, 0 }, // Frame 167
	{ -19.962, -3.142, -12.381, 0, 0, 0 }, // Frame 168
	{ -20.71, -2.108, -12.249, 0, 0, 0 }, // Frame 169
	{ 0.612, 0.702, 25.392, 0.363, 0.352, 0.863 }, // Frame 170
	{ -8.682, 1.569, 19.231, -0.253, 0.308, 0.917 }, // Frame 171
	{ -8.089, 2.514, -7.319, -0.766, 0.275, 0.581 }, // Frame 172
	{ -8.429, 2.702, -15.403, -0.95, 0.291, -0.11 }, // Frame 173
	{ -3.807, 2.269, -23.078, -0.621, 0.3, -0.724 }, // Frame 174
	{ -3.807, 2.269, -23.078, -0.621, 0.3, -0.724 }, // Frame 175
	{ 21.655, 3.993, -13.767, 0.843, 0.239, 0.482 }, // Frame 176
	{ 22.671, 3.691, -19.448, 0.952, 0.216, 0.216 }, // Frame 177
	{ 22.783, 3.691, -19.453, 0.951, 0.218, 0.219 }, // Frame 178
};


//mxd. Spawn some blood using precalculated positions...
void infantry_bleed(edict_t *self)
{
	if (random() > 0.7f) return;

	int i;
	i = self->s.frame - FRAME_death101;

	M_SpawnEffect(self, (random() > 0.8f ? TE_MOREBLOOD : TE_BLOOD), infantry_bleed_positions[i][0], infantry_bleed_positions[i][1]);
}

mframe_t infantry_frames_walk [] =
{
	ai_walk, 5,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  actor_footstep_medium, //mxd. Footsteps
	ai_walk, 6,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  actor_footstep_medium //mxd. Footsteps
};
mmove_t infantry_move_walk = {FRAME_walk03, FRAME_walk14, infantry_frames_walk, NULL};

void infantry_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_walk;
}

mframe_t infantry_frames_run [] =
{
	ai_run, 10, NULL,
	ai_run, 20, NULL,
	ai_run, 5,  actor_footstep_medium_loud, //mxd. Footsteps
	ai_run, 7,  NULL,
	ai_run, 30, NULL,
	ai_run, 35, NULL,
	ai_run, 2,  actor_footstep_medium_loud, //mxd. Footsteps
	ai_run, 6,  NULL
};
mmove_t infantry_move_run = {FRAME_run01, FRAME_run08, infantry_frames_run, NULL};

void infantry_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &infantry_move_stand;
	else
		self->monsterinfo.currentmove = &infantry_move_run;
}


mframe_t infantry_frames_pain1 [] =
{
	ai_move, -3, NULL,
	ai_move, -2, NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL,
	ai_move, -1, NULL,
	ai_move, 1,  NULL,
	ai_move, -1, NULL,
	ai_move, 1,  NULL,
	ai_move, 6,  NULL,
	ai_move, 2,  NULL
};
mmove_t infantry_move_pain1 = {FRAME_pain101, FRAME_pain110, infantry_frames_pain1, infantry_run};

mframe_t infantry_frames_pain2 [] =
{
	ai_move, -3, NULL,
	ai_move, -3, NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 2,  NULL,
	ai_move, 5,  NULL,
	ai_move, 2,  NULL
};
mmove_t infantry_move_pain2 = {FRAME_pain201, FRAME_pain210, infantry_frames_pain2, infantry_run};

void infantry_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	n = rand() % 2;
	if (n == 0)
	{
		self->monsterinfo.currentmove = &infantry_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &infantry_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}


vec3_t	aimangles[] =
{
	0.0, 5.0, 0.0,
	10.0, 15.0, 0.0,
	20.0, 25.0, 0.0,
	25.0, 35.0, 0.0,
	30.0, 40.0, 0.0,
	30.0, 45.0, 0.0,
	25.0, 50.0, 0.0,
	20.0, 40.0, 0.0,
	15.0, 35.0, 0.0,
	40.0, 35.0, 0.0,
	70.0, 35.0, 0.0,
	90.0, 35.0, 0.0
};

void InfantryMachineGun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right;
	vec3_t	vec;
	int		flash_number;

	if (self->s.frame == FRAME_attak111)
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_1;
		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

		if (self->enemy && self->enemy->inuse)
		{
			VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;

			// Lazarus fog reduction of accuracy
			/*if(self->monsterinfo.visibility < FOG_CANSEEGOOD)
			{
				target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
				target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
				target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			}*/
			AdjustAccuracy(self, target); //mxd

			VectorSubtract (target, start, forward);
			VectorNormalize (forward);
		}
		else
		{
			AngleVectors (self->s.angles, forward, right, NULL);
		}
	}
	else
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

		VectorSubtract (self->s.angles, aimangles[flash_number-MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors (vec, forward, NULL, NULL);

		//mxd. Also BLEED!
		infantry_bleed(self);
	}

	monster_fire_bullet (self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

void infantry_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_BODY, sound_sight, 1, ATTN_NORM, 0);
}

void infantry_dead (edict_t *self)
{
	// Lazarus: Stupid... if flies aren't set by M_FlyCheck, monster_think
	//          will cause us to come back here over and over and over
	//          until flies ARE set or monster is gibbed.
	//          This line fixes that:
	self->nextthink = 0;

	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if(world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think=FadeDieSink;
		self->nextthink=level.time+corpse_fadetime->value;
	}
}

//mxd. Skip longer death animations when touched by player
void infantry_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if ( !(other->client || (other->flags & FL_ROBOT) || (other->svflags & SVF_MONSTER)) || other->health <= 0 ) return;

	// Advance animation past firing frames
	int nextframe;
	if (self->s.frame >= FRAME_death101 && self->s.frame < FRAME_death120 - 5) // stand a bit then fall animation
		nextframe = FRAME_death120 - 5;
	else if (self->s.frame >= FRAME_death201 && self->s.frame < FRAME_death225 - 6) // "last stand firing" animation
		nextframe = FRAME_death225 - 4;
	else
		nextframe = 0;

	if (nextframe) 
	{
		// Skip animation
		self->monsterinfo.nextframe = nextframe;
		
		// Play kick sound
		gi.sound(self, CHAN_BODY, gi.soundindex("weapons/kick.wav"), 1, ATTN_NORM, 0);
		if (other->client) PlayerNoise(other, other->s.origin, PNOISE_SELF);

		// Push player back a bit
		if (other->client)
		{
			vec3_t dir;

			VectorSubtract(other->s.origin, self->s.origin, dir);
			VectorNormalize(dir);
			VectorMA(other->velocity, 340, dir, other->velocity);

			// Also push the view up a bit
			other->client->kick_angles[0] -= 7 + random() * 5;
		}
	}
}

mframe_t infantry_frames_death1 [] =
{
	ai_move, -4, infantry_bleed, //mxd. NULL -> infantry_bleed
	ai_move, 0,  infantry_bleed,
	ai_move, 0,  infantry_bleed,
	ai_move, -1, infantry_bleed,
	ai_move, -4, infantry_bleed,
	ai_move, 0,  infantry_bleed,
	ai_move, 0,  infantry_bleed,
	ai_move, 0,  infantry_bleed,
	ai_move, -1, infantry_bleed,
	ai_move, 3,  infantry_bleed,
	ai_move, 1,  infantry_bleed,
	ai_move, 1,  infantry_bleed,
	ai_move, -2, infantry_bleed,
	ai_move, 2,  infantry_bleed,
	ai_move, 2,  infantry_bleed,
	ai_move, 9,  infantry_bleed,
	ai_move, 9,  infantry_bleed,
	ai_move, 5,  infantry_bleed,
	ai_move, -3, infantry_bleed,
	ai_move, -3, infantry_bleed
};
mmove_t infantry_move_death1 = {FRAME_death101, FRAME_death120, infantry_frames_death1, infantry_dead};

// Off with his head
mframe_t infantry_frames_death2 [] =
{
	ai_move, 0,   infantry_bleed, //mxd. NULL -> infantry_bleed
	ai_move, 1,   infantry_bleed,
	ai_move, 5,   infantry_bleed,
	ai_move, -1,  infantry_bleed,
	ai_move, 0,   infantry_bleed,
	ai_move, 1,   infantry_bleed,
	ai_move, 1,   infantry_bleed,
	ai_move, 4,   infantry_bleed,
	ai_move, 3,   infantry_bleed,
	ai_move, 0,   infantry_bleed,
	ai_move, -2,  InfantryMachineGun,
	ai_move, -2,  InfantryMachineGun,
	ai_move, -3,  InfantryMachineGun,
	ai_move, -1,  InfantryMachineGun,
	ai_move, -2,  InfantryMachineGun,
	ai_move, 0,   InfantryMachineGun,
	ai_move, 2,   InfantryMachineGun,
	ai_move, 2,   InfantryMachineGun,
	ai_move, 3,   InfantryMachineGun,
	ai_move, -10, InfantryMachineGun,
	ai_move, -7,  InfantryMachineGun,
	ai_move, -8,  InfantryMachineGun,
	ai_move, -6,  infantry_bleed,
	ai_move, 4,   infantry_bleed,
	ai_move, 0,   infantry_bleed
};
mmove_t infantry_move_death2 = {FRAME_death201, FRAME_death225, infantry_frames_death2, infantry_dead};

mframe_t infantry_frames_death3 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -6,  NULL,
	ai_move, -11, NULL,
	ai_move, -3,  NULL,
	ai_move, -11, NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death3 = {FRAME_death301, FRAME_death309, infantry_frames_death3, infantry_dead};

//mxd
qboolean is_headless(edict_t *self)
{
	if (self->s.frame >= FRAME_death101 && self->s.frame <= FRAME_death120) return true;
	if (self->s.frame >= FRAME_death201 && self->s.frame <= FRAME_death225) return true;
	return false;
}

void infantry_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.skinnum |= 1;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	if (self->health <= self->gib_health && !(self->spawnflags & SF_MONSTER_NOGIB))
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 4; n++) //mxd. 2 -> 4
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 6; n++) //mxd. 4 -> 6
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		if(!is_headless(self)) ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC); //mxd. Don't throw 2 heads...
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	//mxd. Calculate head position
	vec3_t head_pos;
	VectorSet(head_pos, 0, 0, self->maxs[2] - 8);
	VectorAdd(head_pos, self->s.origin, head_pos);

	//mxd. Skip "last stand" attack (infantry_move_death2) on Easy
	n = (skill->integer < 1 ? ((rand() % 2) ? 2 : 0) : rand() % 3);
	//n = rand() % 3;
	if (n == 0)
	{
		self->touch = infantry_touch; //mxd
		self->monsterinfo.currentmove = &infantry_move_death1;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
		
		//mxd. Throw head now
		ThrowGibEx(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC, head_pos, NULL, NULL, NULL);
	}
	else if (n == 1)
	{
		self->touch = infantry_touch; //mxd
		self->monsterinfo.currentmove = &infantry_move_death2;
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);

		//mxd. Throw head now
		ThrowGibEx(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC, head_pos, NULL, NULL, NULL);
	}
	else
	{
		self->monsterinfo.currentmove = &infantry_move_death3;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
}


void infantry_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 20; //mxd. Was 32
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1;
	gi.linkentity (self);
}

void infantry_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void infantry_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 20; //mxd. Was 32
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t infantry_frames_duck [] =
{
	ai_move, -2, infantry_duck_down,
	ai_move, -5, infantry_duck_hold,
	ai_move, 3,  NULL,
	ai_move, 4,  infantry_duck_up,
	ai_move, 0,  NULL
};
mmove_t infantry_move_duck = {FRAME_duck01, FRAME_duck05, infantry_frames_duck, infantry_run};

void infantry_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &infantry_move_duck;
}


void infantry_cock_gun (edict_t *self)
{
	int		n;

	gi.sound (self, CHAN_WEAPON, sound_weapon_cock, 1, ATTN_NORM, 0);
	n = (rand() & 15) + 3 + 7;
	self->monsterinfo.pausetime = level.time + n * FRAMETIME;
}

void infantry_fire (edict_t *self)
{
	InfantryMachineGun (self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t infantry_frames_attack1 [] =
{
	ai_charge, 4,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -1, NULL,
	ai_charge, 0,  infantry_cock_gun,
	ai_charge, -1, NULL,
	ai_charge, 1,  NULL,
	ai_charge, 1,  NULL,
	ai_charge, 2,  NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL,
	ai_charge, 1,  infantry_fire,
	ai_charge, 5,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL
};

mmove_t infantry_move_attack1 = {FRAME_attak101, FRAME_attak115, infantry_frames_attack1, infantry_run};


void infantry_swing (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_punch_swing, 1, ATTN_NORM, 0);
}

void infantry_smack (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 0);
	if (fire_hit (self, aim, (5 + (rand() % 5)), 50))
		gi.sound (self, CHAN_WEAPON, sound_punch_hit, 1, ATTN_NORM, 0);
}

mframe_t infantry_frames_attack2 [] =
{
	ai_charge, 3, NULL,
	ai_charge, 6, NULL,
	ai_charge, 0, infantry_swing,
	ai_charge, 8, NULL,
	ai_charge, 5, NULL,
	ai_charge, 8, infantry_smack,
	ai_charge, 6, NULL,
	ai_charge, 3, NULL,
};
mmove_t infantry_move_attack2 = {FRAME_attak201, FRAME_attak208, infantry_frames_attack2, infantry_run};

void infantry_attack(edict_t *self)
{
	if (range (self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = &infantry_move_attack2;
	else
		self->monsterinfo.currentmove = &infantry_move_attack1;
}

//Jump
mframe_t infantry_frames_jump [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t infantry_move_jump = { FRAME_run01, FRAME_run08, infantry_frames_jump, infantry_run };

void infantry_jump (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_jump;
}

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_infantry (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("infantry/infpain1.wav");
	sound_pain2 = gi.soundindex ("infantry/infpain2.wav");
	sound_die1 = gi.soundindex ("infantry/infdeth1.wav");
	sound_die2 = gi.soundindex ("infantry/infdeth2.wav");

	sound_gunshot = gi.soundindex ("infantry/infatck1.wav");
	sound_weapon_cock = gi.soundindex ("infantry/infatck3.wav");
	sound_punch_swing = gi.soundindex ("infantry/infatck2.wav");
	sound_punch_hit = gi.soundindex ("infantry/melee2.wav");
	
	sound_sight = gi.soundindex ("infantry/infsght1.wav");
	sound_search = gi.soundindex ("infantry/infsrch1.wav");
	sound_idle = gi.soundindex ("infantry/infidle1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/infantry/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	// Lazarus: mapper-configurable health
	if(!self->health)
		self->health = 100;
	if(!self->gib_health)
		self->gib_health = -40;
	if(!self->mass)
		self->mass = 200;

	self->pain = infantry_pain;
	self->die = infantry_die;

	self->monsterinfo.stand = infantry_stand;
	self->monsterinfo.walk = infantry_walk;
	self->monsterinfo.run = infantry_run;
	self->monsterinfo.dodge = infantry_dodge;
	self->monsterinfo.attack = infantry_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = infantry_sight;
	self->monsterinfo.idle = infantry_fidget;
	if(monsterjump->value)
	{
		self->monsterinfo.jump = infantry_jump;
		self->monsterinfo.jumpup = 48;
		self->monsterinfo.jumpdn = 160;
	}

	// Lazarus
	if(self->powerarmor) {
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}
	if(!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.40;

	gi.linkentity (self);
	self->monsterinfo.currentmove = &infantry_move_stand;
	if(self->health < 0)
	{
		mmove_t	*deathmoves[] = {&infantry_move_death1,
			                     &infantry_move_death2,
								 &infantry_move_death3,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->common_name = "Enforcer";

	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}

