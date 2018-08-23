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

#include "g_local.h"

#define RFAST		-3
#define RMEDIUM		-2
#define RSLOW		-1
#define STOP		0
#define SLOW		1
#define MEDIUM		2
#define FAST		3

// func_tracktrain spawnflags
#define SF_TRACKTRAIN_NOPITCH		0x0001		// yaw only (maybe roll)
#define SF_TRACKTRAIN_NOCONTROL		0x0002		// no player control
#define SF_TRACKTRAIN_ONEWAY		0x0004		// cannot back up
#define SF_TRACKTRAIN_OTHERMAP		0x0008		// "parent" train resides in another map
#define SF_TRACKTRAIN_NOHUD			0x0010		// do not display speed on HUD
#define SF_TRACKTRAIN_INDICATOR		0x0020		// train has an animated speed indicator
#define SF_TRACKTRAIN_ANIMATED		0x0040		// normal animation
#define SF_TRACKTRAIN_STARTOFF		0x0080		// starts disabled
#define SF_TRACKTRAIN_DISABLED		0x1000		// internal use only
#define SF_TRACKTRAIN_ROLLSPEED		0x2000		// internal use only - roll is a function of speed
#define SF_TRACKTRAIN_SLOWSTOP		0x4000		// internal use only - train slows to stop after driver death
#define SF_TRACKTRAIN_ANIMMASK		(SF_TRACKTRAIN_INDICATOR | SF_TRACKTRAIN_ANIMATED)
// path_track spawnflags
#define SF_PATH_ALTPATH				0x0001		// branch point
#define SF_PATH_DISABLED			0x0002
#define SF_PATH_FIREONCE			0x0004		// pathtarget fired only once
#define SF_PATH_DISABLE_TRAIN		0x0008		// player loses control of train
#define SF_PATH_ABS_SPEED			0x0010		// speed modifier is absolute rather than a multiplier

void tracktrain_next(edict_t *self);
void tracktrain_reach_dest(edict_t *self);

//=============================================================================

static float track_AngleMod(float in)
{
	if (in < 0)
		return in + 360 * ((int)(in / 360) + 1);

	if (in >= 360)
		return in - 360 * (int)(in / 360);

	return in;
}

static float track_ApproachAngle(float targAngle, /*float inAngle,*/ float speed)
{
	targAngle = track_AngleMod(targAngle);
	float out = track_AngleMod(targAngle);

	float deltaAngle = targAngle - out;

	// fix speed to be non-negative
	if (speed < 0)
		speed *= -1;
	
	if (deltaAngle < -180)
		deltaAngle += 360;
	else if (deltaAngle > 180)
		deltaAngle -= 360;

	if (deltaAngle > speed)
		out += speed;
	else if (deltaAngle < -speed)
		out -= speed;
	else
		out = targAngle;

	return out;
}

static float track_AngleDistance(float nextAngle, float curAngle)
{
	float deltaAngle = nextAngle - curAngle;

	if (deltaAngle < -180)
		deltaAngle += 360;
	else if (deltaAngle > 180)
		deltaAngle -= 360;

	return deltaAngle;
}

/*=============================================================================

PATH_TRACK

target:			next path_track
target2:		alternate path
speed:			new tracktrain speed
pathtarget:		entity to trigger when touched
deathtarget:	entity to trigger at dead end

==============================================================================*/

void path_track_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & SF_PATH_ALTPATH)
	{
		char *temp = self->target;
		self->target = self->target2;
		self->target2 = temp;
	}
	else
	{
		// toggle enabled/disabled
		self->spawnflags ^= SF_PATH_DISABLED;
	}
}

void SP_path_track(edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf("%s with no targetname at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->class_id = ENTITY_PATH_TRACK;

	self->solid = SOLID_TRIGGER;
	self->use = path_track_use;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;

	if (!self->count) 
		self->count = -1;

	gi.linkentity(self);
}

/* ============================================================

FUNC_TRACKCHANGE

height			= Travel altitude (viewheight)
distance		= Rotation amount, degrees (moveinfo.distance)
target			= Name of train
pathtarget		= Top path_track
combattarget	= Bottom path_track
speed			= Move/Rotate speed
"sounds"
0,1	default
2-99 custom sound- e.g. plats/pt02_strt.wav, plats/pt02_mid.wav, plats/pt02_end.wav

=============================================================*/

#define STATE_TOP		0
#define STATE_MOVEDOWN	1
#define STATE_BOTTOM	2
#define STATE_MOVEUP	3

#define SF_TRACK_ACTIVATETRAIN	0x0001
#define SF_TRACK_STARTBOTTOM	0x0008
#define SF_TRACK_XAXIS			0x0040
#define SF_TRACK_YAXIS			0x0080


void trackchange_done(edict_t *self)
{
	edict_t	*train = self->target_ent;

	VectorClear(self->velocity);
	VectorClear(self->avelocity);

	if (self->s.sound && self->moveinfo.sound_end)
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->moveinfo.sound_end, 1, self->attenuation, 0); // was ATTN_NORM

	self->s.sound = 0;

	if (train)
	{
		VectorClear(train->velocity);
		VectorClear(train->avelocity);
		train->spawnflags &= ~SF_TRACKTRAIN_DISABLED;

		if (self->spawnflags & SF_TRACK_ACTIVATETRAIN)
		{
			train->moveinfo.state = train->moveinfo.prevstate;

			if (train->moveinfo.state && train->sounds > 0)
			{
				train->s.sound = gi.soundindex(va("%sspeed%d.wav", train->source, abs(train->moveinfo.state)));
	#ifdef LOOP_SOUND_ATTENUATION
				train->s.attenuation = train->attenuation;
	#endif
			}

			train->moveinfo.next_speed = train->moveinfo.state * train->moveinfo.speed/3;
		}
		else
		{
			// A little redundant, but clear
			train->moveinfo.state = STOP;
			train->moveinfo.next_speed = 0;
		}

		if (self->moveinfo.state == STATE_MOVEUP)
			train->target = self->pathtarget;
		else
			train->target = self->combattarget;

		train->target_ent = G_PickTarget(train->target);
		
		// If map is constructed properly, train SHOULD be at the target path_track now.
		// But physics may have caused lift to outrun train, so force it:
		VectorCopy(train->target_ent->s.origin, train->s.origin);
		train->s.origin[2] += train->viewheight;
		gi.linkentity(train);
		tracktrain_next(train);
	}

	if (self->moveinfo.state == STATE_MOVEDOWN)
		self->moveinfo.state = STATE_BOTTOM;
	else
		self->moveinfo.state = STATE_TOP;

	gi.linkentity(self);
}

void trackchange_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->moveinfo.state != STATE_TOP && self->moveinfo.state != STATE_BOTTOM)
		return; // already moving

	if (self->target)
	{
		edict_t	*track;

		self->target_ent = G_PickTarget(self->target);
		if (self->target_ent)
		{
			// Make sure this sucker is at the path_track it's supposed to be at
			if (self->moveinfo.state == STATE_BOTTOM)
				track = G_PickTarget(self->combattarget);
			else
				track = G_PickTarget(self->pathtarget);

			if (!track || (self->target_ent->target_ent != track))
			{
				self->target_ent = NULL;
			}
			else
			{
				vec3_t v;
				VectorSubtract(track->s.origin, self->target_ent->s.origin, v);
				if (VectorLength(v) > self->target_ent->moveinfo.distance)
					self->target_ent = NULL;
			}
		}
	}
	else
	{
		self->target_ent = NULL;
	}

	if (self->target_ent)
		self->target_ent->s.sound = 0;

	// Adjust speed so that "distance" rotation and "height" movement are achieved simultaneously.
	const float tt = abs(self->viewheight) / self->speed;
	const float tr = fabsf(self->moveinfo.distance) / self->speed;
	float time = max(tt, tr);
	time = 0.1f * ((int)(10.0f * time - 0.5f) + 1);

	float tspeed = -self->viewheight / time;
	float rspeed = self->moveinfo.distance / time;
	if (self->moveinfo.state == STATE_BOTTOM)
	{
		tspeed *= -1;
		rspeed *= -1;
	}

	VectorClear(self->velocity);
	VectorClear(self->avelocity);

	if (self->spawnflags & SF_TRACK_XAXIS)
		self->velocity[0] = tspeed;
	else if (self->spawnflags & SF_TRACK_YAXIS)
		self->velocity[1] = tspeed;
	else
		self->velocity[2] = tspeed;

	VectorScale(self->movedir, rspeed, self->avelocity);

	if (self->target_ent)
	{
		VectorCopy(self->velocity, self->target_ent->velocity);
		VectorCopy(self->avelocity, self->target_ent->avelocity);
		self->target_ent->spawnflags |= SF_TRACKTRAIN_DISABLED;
		gi.linkentity(self->target_ent);
	}

	if (self->moveinfo.state == STATE_TOP)
		self->moveinfo.state = STATE_MOVEDOWN;
	else
		self->moveinfo.state = STATE_MOVEUP;

	if (self->moveinfo.sound_start)
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->moveinfo.sound_start, 1, self->attenuation, 0); // was ATTN_NORM

	self->s.sound = self->moveinfo.sound_middle;
#ifdef LOOP_SOUND_ATTENUATION
	self->s.attenuation = self->attenuation;
#endif

	self->think = trackchange_done;
	self->nextthink = level.time + time;

	gi.linkentity(self);
}

void SP_func_trackchange(edict_t *self)
{
	self->class_id = ENTITY_FUNC_TRACKCHANGE;

	self->movetype = MOVETYPE_PUSH;
	VectorClear(self->s.angles);

	// Set the axis of rotation
	VectorClear(self->movedir);
	if (self->spawnflags & SF_TRACK_XAXIS)
		self->movedir[2] = 1.0;
	else if (self->spawnflags & SF_TRACK_YAXIS)
		self->movedir[0] = 1.0;
	else // Z_AXIS
		self->movedir[1] = 1.0;

	// Rotation
	self->moveinfo.distance = st.distance;

	// Travel height
	self->viewheight = st.height;

	// Max rotation/translation speed
	if (!self->speed)
		self->speed = 100;

	VectorCopy(self->s.origin, self->pos1);
	VectorCopy(self->pos1, self->pos2);

	if (self->spawnflags & SF_TRACK_XAXIS)
		self->pos2[0] -= self->viewheight;
	else if (self->spawnflags & SF_TRACK_YAXIS)
		self->pos2[1] -= self->viewheight;
	else
		self->pos2[2] -= self->viewheight;

	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);
	self->use = trackchange_use;

	// bottom starters:
	if (self->spawnflags & SF_TRACK_STARTBOTTOM)
	{
		vec3_t temp;
		VectorCopy(self->pos1, temp);
		VectorCopy(self->pos2, self->pos1);
		VectorCopy(temp, self->pos2);
		VectorCopy(self->pos1, self->s.origin);
		VectorMA(self->s.angles, self->moveinfo.distance, self->movedir, self->s.angles);
		self->moveinfo.state = STATE_BOTTOM;
	}
	else
	{
		self->moveinfo.state = STATE_TOP;
	}

	if (self->sounds > 1 && self->sounds < 100) // custom sounds
	{
		self->moveinfo.sound_start =  gi.soundindex(va("plats/pt%02i_strt.wav", self->sounds));
		self->moveinfo.sound_middle = gi.soundindex(va("plats/pt%02i_mid.wav", self->sounds));
		self->moveinfo.sound_end =    gi.soundindex(va("plats/pt%02i_end.wav", self->sounds));
	}
	else
	{
		self->moveinfo.sound_start =  gi.soundindex("plats/pt1_strt.wav");
		self->moveinfo.sound_middle = gi.soundindex("plats/pt1_mid.wav");
		self->moveinfo.sound_end =    gi.soundindex("plats/pt1_end.wav");
	}

	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;

	gi.linkentity(self);
}


/*=====================================================================================
func_tracktrain 

target		first path_track stop
dmg			damage applied to blocker
speed		max. speed, default=64
sounds		currently unused
distance	wheelbase, determines turn rate. default=50
height		height of origin above path_tracks, default=4
roll		roll angle while turning, default=0
*/
void tracktrain_think(edict_t *self);

void tracktrain_drive(edict_t *train, edict_t *other)
{
	vec3_t	angles, offset;
	vec3_t	f1, l1, u1;
	vec3_t	forward, left;

	if (!(other->svflags & SVF_MONSTER) || train->spawnflags & (SF_TRACKTRAIN_NOCONTROL | SF_TRACKTRAIN_STARTOFF))
		return;

	// See if monster is in driving position
	VectorCopy(train->s.angles, angles);
	VectorNegate(angles, angles);
	AngleVectors(angles, f1, l1, u1);
			
	VectorSubtract(other->s.origin, train->s.origin, offset);
	VectorScale(f1,  offset[0], f1);
	VectorScale(l1, -offset[1], l1);
	VectorScale(u1,  offset[2], u1);
	VectorCopy(f1, offset);
	VectorAdd(offset, l1, offset);
	VectorAdd(offset, u1, offset);

	// Relax the constraints on driving position just a bit for monsters
	for (int i = 0; i < 3; i++)
	{
		if (offset[i] < train->bleft[i] - 16)
			return;

		if (offset[i] > train->tright[i] + 16)
			return;
	}

	train->owner = other;
	other->vehicle = train;

	// Store the offset and later keep driver at same relative position (with height adjustments for pitch)
	AngleVectors(train->s.angles, forward, left, NULL);
	VectorSubtract(other->s.origin, train->s.origin, train->offset);
	VectorScale(forward, train->offset[0], f1);
	VectorScale(left, train->offset[1], l1);
	VectorCopy(f1, train->offset);
	VectorAdd(train->offset, l1, train->offset);
	train->offset[1] = -train->offset[1];
	train->offset[2] = other->s.origin[2] - train->s.origin[2];
	gi.linkentity(other);
	gi.linkentity(train);

	other->monsterinfo.pausetime = level.time + 100000000;
	other->monsterinfo.aiflags |= AI_STAND_GROUND;
	other->monsterinfo.stand(other);

	train->moveinfo.state = FAST;
	train->moveinfo.next_speed = train->moveinfo.speed;

	if (train->sounds)
	{
		train->s.sound = gi.soundindex(va("%sspeed%d.wav", train->source, abs(train->moveinfo.state)));
#ifdef LOOP_SOUND_ATTENUATION
		train->s.attenuation = train->attenuation;
#endif
	}
	else
	{
		train->s.sound = 0;
	}

	train->think = tracktrain_think;
	train->think(train);
}

void tracktrain_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->deathtarget)
	{
		self->target = self->deathtarget;
		G_UseTargets(self, attacker);
	}

	BecomeExplosion1(self);
}

void tracktrain_disengage(edict_t *train)
{
	edict_t *driver = train->owner;
	if (!driver)
		return;

	if (driver->client)
	{
		vec3_t forward, left, up, f1, l1, u1;

		driver->movetype = MOVETYPE_WALK;
		AngleVectors(train->s.angles, forward, left, up);
		VectorScale(forward, train->offset[0], f1);
		VectorScale(left, -train->offset[1], l1);
		VectorScale(up, train->offset[2], u1);
		VectorAdd(train->s.origin, f1, driver->s.origin);
		VectorAdd(driver->s.origin, l1, driver->s.origin);
		VectorAdd(driver->s.origin, u1, driver->s.origin);
		driver->s.origin[2] += 16 * (fabs(up[0]) + fabs(up[1]));
		VectorCopy(train->velocity, driver->velocity);
		driver->client->vehicle_framenum = level.framenum;
		// turn ON client side prediction for this player
		driver->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	}
	else
	{
		if (train->moveinfo.state != STOP)
		{
			train->spawnflags |= SF_TRACKTRAIN_SLOWSTOP;
			train->moveinfo.state = STOP;
			train->moveinfo.next_speed = 0;
			train->s.sound = gi.soundindex(va("%sspeed1.wav",train->source));
	#ifdef LOOP_SOUND_ATTENUATION
			train->s.attenuation = train->attenuation;
	#endif
		}

		driver->movetype = MOVETYPE_STEP;

		if (driver->health > 0)
			VectorCopy(train->velocity, driver->velocity);
		else
			VectorClear(driver->velocity);

		driver->monsterinfo.pausetime = 0;
		driver->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	}

	gi.linkentity(driver);

	train->owner = NULL;
	driver->vehicle = NULL;
}

void tracktrain_hide(edict_t *self)
{
	self->solid = SOLID_NOT;
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

void tracktrain_think(edict_t *self)
{
	float	distance;
	vec3_t	forward, left, up, f1, l1, u1, v;
	float	speed = 0.0f;
	float	time = 100000; //mxd

	if (self->spawnflags & SF_TRACKTRAIN_OTHERMAP)
	{
		if ((self->spawnflags & SF_TRACKTRAIN_INDICATOR) && !(self->spawnflags & SF_TRACKTRAIN_ANIMATED))
			self->s.frame = 6;
		else
			self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);

		self->spawnflags |= SF_TRACKTRAIN_DISABLED;
		self->think = tracktrain_hide;
		self->nextthink = level.time + FRAMETIME;
		VectorClear(self->velocity);
		VectorClear(self->avelocity);
		self->moveinfo.state = self->moveinfo.prevstate = STOP;
		self->s.sound = 0;
		self->moveinfo.current_speed = 0;
		self->owner = NULL;
		gi.linkentity(self);

		return;
	}

	self->nextthink = level.time + FRAMETIME;

	if (!self->owner && (self->spawnflags & SF_TRACKTRAIN_DISABLED))
		return;

	if (self->spawnflags & SF_TRACKTRAIN_ANIMATED)
	{
		if (self->moveinfo.state)
		{
			if (self->spawnflags & SF_TRACKTRAIN_INDICATOR)
				self->s.effects |= EF_ANIM_ALLFAST;
			else
				self->s.effects |= EF_ANIM_ALL;
		}
		else
		{
			self->s.effects &= ~(EF_ANIM_ALL | EF_ANIM_ALLFAST);
		}
	}
	else if (self->spawnflags & SF_TRACKTRAIN_INDICATOR)
	{
		if ((self->moveinfo.state == STOP) && !self->owner)
		{
			if (self->spawnflags & SF_TRACKTRAIN_STARTOFF)
				self->s.frame = 14;
			else
				self->s.frame = 6;
		}
		else if ((level.framenum % 5) == 0)
		{
			self->s.frame = (self->moveinfo.state - RFAST) * 2 + (1 - (self->s.frame % 2));
		}
	}

	AngleVectors(self->s.angles, forward, left, up);

	if (!(self->spawnflags & SF_TRACKTRAIN_DISABLED))
	{
		VectorCopy(self->velocity, v);
		speed = VectorLength(v);

		if (self->moveinfo.state < STOP)
			speed = -speed;

		self->moveinfo.current_speed = speed;

		if (self->moveinfo.state != STOP)
			self->moveinfo.prevstate = self->moveinfo.state;
	}

	if (self->owner)
	{
		edict_t	*driver = self->owner;

		// ... then we have a driver
		if (driver->health <= 0 || driver->movetype == MOVETYPE_NOCLIP)
		{
			tracktrain_disengage(self);
			return;
		}

		if (driver->client)
		{
			if (driver->client->use)
			{
				// if he's pressing the use key, and he didn't just get on or off, disengage
				if (level.framenum - driver->client->vehicle_framenum > 2)
				{
					VectorCopy(self->velocity, driver->velocity);
					tracktrain_disengage(self);

					return;
				}
			}

			if (driver->client->ucmd.sidemove < -199 || driver->client->ucmd.sidemove > 199 || driver->client->ucmd.upmove > 0)
			{
				VectorCopy(self->velocity, driver->velocity);
				tracktrain_disengage(self);

				return;
			}
		}

		if (!(self->spawnflags & SF_TRACKTRAIN_DISABLED))
		{
			if (driver->client)
			{
				if (driver->client->ucmd.forwardmove > -199 && driver->client->ucmd.forwardmove < 199)
				{
					self->moveinfo.wait = 0;
				}
				else if (!self->moveinfo.wait)
				{
					if (driver->client->ucmd.forwardmove > 0)
					{
						if (self->moveinfo.state < FAST)
						{
							self->moveinfo.state++;
							self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
							self->moveinfo.wait = 1;

							if ((self->spawnflags & SF_TRACKTRAIN_ANIMMASK) == SF_TRACKTRAIN_INDICATOR)
								self->s.frame = (self->moveinfo.state - RFAST) * 2 + (1 - (self->s.frame % 2));

							if (self->moveinfo.state && self->sounds > 0)
							{
								self->s.sound = gi.soundindex(va("%sspeed%d.wav",self->source,abs(self->moveinfo.state)));
						#ifdef LOOP_SOUND_ATTENUATION
								self->s.attenuation = self->attenuation;
						#endif
							}
							else
							{
								self->s.sound = 0;
							}
						}
					}
					else
					{
						if (self->moveinfo.state > STOP || (self->moveinfo.state > RFAST && !(self->spawnflags & SF_TRACKTRAIN_ONEWAY)))
						{
							self->moveinfo.state--;
							self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
							self->moveinfo.wait = 1;

							if ((self->spawnflags & SF_TRACKTRAIN_ANIMMASK) == SF_TRACKTRAIN_INDICATOR)
								self->s.frame = (self->moveinfo.state - RFAST) * 2 + (1 - (self->s.frame % 2));

							if (self->moveinfo.state && self->sounds > 0)
							{
								self->s.sound = gi.soundindex(va("%sspeed%d.wav", self->source, abs(self->moveinfo.state)));
						#ifdef LOOP_SOUND_ATTENUATION
								self->s.attenuation = self->attenuation;
						#endif
							}
							else
							{
								self->s.sound = 0;
							}
						}
					}
				}
			}
			else
			{
				// Driver is monster.
				if (driver->enemy && driver->enemy->inuse)
				{
					const qboolean vis = visible(driver, driver->enemy);

					if (vis)
						self->monsterinfo.search_time = 0;

					if (self->monsterinfo.search_time == 0)
					{
						vec3_t vec;

						if (vis)
							VectorSubtract(driver->enemy->s.origin, driver->s.origin, vec);
						else
							VectorSubtract(driver->monsterinfo.last_sighting, driver->s.origin, vec);

						const float r = VectorNormalize(vec);
						const float dot = DotProduct(vec, forward);

						if (r > 2000 && dot < 0)
						{
							self->moveinfo.state = -self->moveinfo.state;
							self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
							self->monsterinfo.search_time = level.time;
							tracktrain_next(self);

							return;
						}
					}
				}
			}

			// Check for change in direction
			if ((self->moveinfo.prevstate < STOP && self->moveinfo.state > STOP)
			 || (self->moveinfo.prevstate > STOP && self->moveinfo.state < STOP))
			{
				self->moveinfo.prevstate = self->moveinfo.state;
				tracktrain_next(self);

				return;
			}

			if (self->moveinfo.current_speed < self->moveinfo.next_speed)
			{
				speed = self->moveinfo.current_speed + self->moveinfo.accel / 10;
				speed = min(self->moveinfo.next_speed, speed);
			}
			else if (self->moveinfo.current_speed > self->moveinfo.next_speed)
			{
				speed = self->moveinfo.current_speed - self->moveinfo.decel / 10;
				speed = max(self->moveinfo.next_speed, speed);
			}
			
			VectorSubtract(self->moveinfo.end_origin, self->s.origin, v);
			distance = VectorLength(v);
			if (speed != 0)
			{
				time = distance / fabsf(speed);
				time = 0.1 * ((int)(10 * time - 0.5) + 1);

				if (time > 0 && distance > 0)
					speed = distance / time;
			}
			else
			{
				time = 100000;
			}

			VectorNormalize(v);
			VectorScale(v, fabsf(speed), self->velocity);

			gi.linkentity(self);
		}

		// Set driver velocity, position, and angles
		VectorCopy(self->velocity, driver->velocity);

		VectorScale(forward, self->offset[0], f1);
		VectorScale(left, -self->offset[1], l1);
		VectorScale(up, self->offset[2], u1);
		VectorAdd(self->s.origin, f1, driver->s.origin);
		VectorAdd(driver->s.origin, l1, driver->s.origin);
		VectorAdd(driver->s.origin, u1, driver->s.origin);
		driver->s.origin[2] += 16 * (fabs(up[0]) + fabs(up[1]));

		const float yaw = self->avelocity[YAW] * FRAMETIME;
		if (yaw != 0)
		{
			driver->s.angles[YAW] += yaw;
			if (driver->client)
			{
				driver->client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(yaw);
				driver->client->ps.viewangles[YAW] += yaw;
			}
		}

		float pitch = self->avelocity[PITCH] * FRAMETIME;
		if (pitch != 0)
		{
			float delta_yaw = driver->s.angles[YAW] - self->s.angles[YAW];
			delta_yaw *= M_PI / 180.0;
			pitch *= cosf(delta_yaw);
			if (driver->client)
			{
				driver->client->ps.pmove.delta_angles[PITCH] += ANGLE2SHORT(pitch);
				driver->client->ps.viewangles[PITCH] += pitch;
			}
		}

		if (driver->client && (self->moveinfo.state != STOP || yaw != 0 || pitch != 0))
			driver->client->ps.pmove.pm_type = PM_FREEZE;

		gi.linkentity(driver);

	}
	else if (self->spawnflags & (SF_TRACKTRAIN_NOCONTROL | SF_TRACKTRAIN_STARTOFF))
	{
		// No driver, either can't be controlled or is "off"
		if (!(self->spawnflags & SF_TRACKTRAIN_DISABLED))
		{
			self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;

			if (self->moveinfo.current_speed < self->moveinfo.next_speed)
			{
				speed = self->moveinfo.current_speed + self->moveinfo.accel / 10;
				speed = min(self->moveinfo.next_speed, speed);
			}
			else if (self->moveinfo.current_speed > self->moveinfo.next_speed)
			{
				speed = self->moveinfo.current_speed - self->moveinfo.decel / 10;
				speed = max(self->moveinfo.next_speed, speed);
			}
			
			if (speed != 0)
			{
				VectorSubtract(self->moveinfo.end_origin, self->s.origin, v);
				distance = VectorLength(v);
				time = distance / fabsf(speed);
				time = 0.1 * ((int)(10 * time - 0.5) + 1);

				if (time > 0 && distance > 0)
					speed = distance / time;

				VectorNormalize(v);
				VectorScale(v, fabsf(speed), self->velocity);

				gi.linkentity(self);
			}

			if (!(self->spawnflags & SF_TRACKTRAIN_NOCONTROL) && (self->spawnflags & SF_TRACKTRAIN_STARTOFF) && self->viewmessage)
			{
				vec3_t angles, offset;

				// Check for player entering bleft/tright field of train
				VectorCopy(self->s.angles, angles);
				VectorNegate(angles, angles);
				AngleVectors(angles, f1, l1, u1);

				for (int i = 1; i <= maxclients->value; i++)
				{
					edict_t *ent = g_edicts + i;
					
					if (!ent->inuse || ent->movetype == MOVETYPE_NOCLIP)
						continue;

					VectorSubtract(ent->s.origin, self->s.origin, offset);
					VectorScale(f1, offset[0], f1);
					VectorScale(l1, -offset[1], l1);
					VectorScale(u1, offset[2], u1);
					VectorCopy(f1, offset);
					VectorAdd(offset, l1, offset);
					VectorAdd(offset, u1, offset);

					if (offset[0] < self->bleft[0])
						continue;
					if (offset[1] < self->bleft[1])
						continue;
					if (offset[2] < self->bleft[2])
						continue;
					if (offset[0] > self->tright[0])
						continue;
					if (offset[1] > self->tright[1])
						continue;
					if (offset[2] > self->tright[2])
						continue;

					safe_centerprintf(ent, "%s", self->viewmessage);
					self->viewmessage = NULL;
				}
			}

			if (!speed)
			{
				if (self->viewmessage)
				{
					time = 100000;
				}
				else
				{
					//VectorClear(self->avelocity); //TODO: mxd. Was this supposed to clear self->velocity?
					VectorClear(self->avelocity);
					self->nextthink = 0;

					if (self->movewith_next && self->movewith_next->movewith_ent == self)
						set_child_movement(self);

					gi.linkentity(self);

					return;
				}
			}
		}
	}
	else
	{
		//
		// No driver, CAN be controlled and isn't currently turned off
		//
		self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
		if (self->moveinfo.current_speed < self->moveinfo.next_speed)
		{
			if (self->spawnflags & SF_TRACKTRAIN_SLOWSTOP)
				speed = self->moveinfo.current_speed + self->moveinfo.accel / 25;
			else
				speed = self->moveinfo.current_speed + self->moveinfo.accel / 10;

			speed = min(self->moveinfo.next_speed, speed);
		}
		else if (self->moveinfo.current_speed > self->moveinfo.next_speed)
		{
			if (self->spawnflags & SF_TRACKTRAIN_SLOWSTOP)
				speed = self->moveinfo.current_speed - self->moveinfo.decel / 25;
			else
				speed = self->moveinfo.current_speed - self->moveinfo.decel / 10;

			speed = max(self->moveinfo.next_speed, speed);
		}

		if (speed != 0) 
		{
			VectorSubtract(self->moveinfo.end_origin, self->s.origin, v);
			distance = VectorNormalize(v);
			time = distance / fabsf(speed);
			time = 0.1 * ((int)(10 * time - 0.5) + 1);

			if (time > 0 && distance > 0)
				speed = distance / time;

			VectorScale(v, fabsf(speed), self->velocity);
			gi.linkentity(self);
		}
		else if (self->moveinfo.current_speed != 0)
		{
			VectorClear(self->velocity);
			self->s.sound = 0;
			gi.linkentity(self);
			time = 100000;
		}
		else
		{
			self->spawnflags &= ~SF_TRACKTRAIN_SLOWSTOP;
			time = 100000;
		}

		if (!(self->spawnflags & SF_TRACKTRAIN_NOCONTROL))
		{
			vec3_t angles, offset;

			// Check if a player is in driving position (monsters handled elsewhere)
			VectorCopy(self->s.angles, angles);
			VectorNegate(angles, angles);
			AngleVectors(angles, f1, l1, u1);
			
			// find a player
			for (int i = 1; i <= maxclients->value; i++)
			{
				edict_t *ent = g_edicts + i;
				
				if (!ent->inuse || ent->movetype == MOVETYPE_NOCLIP)
					continue;
				if (!ent->client->use && !self->message)
					continue;
				if (level.framenum - ent->client->vehicle_framenum <= 2)
					continue;

				VectorSubtract(ent->s.origin, self->s.origin, offset);
				VectorScale(f1, offset[0], f1);
				VectorScale(l1, -offset[1], l1);
				VectorScale(u1, offset[2], u1);
				VectorCopy(f1, offset);
				VectorAdd(offset, l1, offset);
				VectorAdd(offset, u1, offset);

				if (offset[0] < self->bleft[0])
					continue;
				if (offset[1] < self->bleft[1])
					continue;
				if (offset[2] < self->bleft[2])
					continue;
				if (offset[0] > self->tright[0])
					continue;
				if (offset[1] > self->tright[1])
					continue;
				if (offset[2] > self->tright[2])
					continue;

				if (self->message)
				{
					safe_centerprintf(ent, "%s", self->message);
					self->message = NULL;
				}

				if (!ent->client->use)
					continue;

				// Got a driver!
				ent->client->vehicle_framenum = level.framenum;
				self->owner = ent;
				ent->vehicle = self;
				// turn off client side prediction for this player
				ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

				// Store the offset and later keep driver at same relative position (with height adjustments for pitch)
				VectorSubtract(ent->s.origin, self->s.origin, self->offset);
				VectorScale(forward, self->offset[0], f1);
				VectorScale(left, self->offset[1], l1);
				VectorCopy(f1, self->offset);
				VectorAdd(self->offset, l1, self->offset);
				self->offset[1] = -self->offset[1];
				self->offset[2] = ent->s.origin[2] - self->s.origin[2];
				gi.linkentity(ent);
				self->moveinfo.wait = 1;

				gi.linkentity(self);
			}
		}
	}

	if (self->movewith_next && self->movewith_next->movewith_ent == self)
		set_child_movement(self);

	if (time < 1.5 * FRAMETIME && !(self->spawnflags & SF_TRACKTRAIN_DISABLED))
		self->think = tracktrain_reach_dest;
}

void tracktrain_blocked(edict_t *self, edict_t *other)
{
	vec3_t	dir;
	int		knockback;

	// Correct owner's velocity
	if (self->owner)
	{
		edict_t	*driver = self->owner;
		vec3_t	forward, left, up;
		vec3_t	f1, l1, u1;

		VectorCopy(self->velocity, driver->velocity);
		AngleVectors(self->s.angles, forward, left, up);
		VectorScale(forward, self->offset[0], f1);
		VectorScale(left, -self->offset[1], l1);
		VectorScale(up, self->offset[2], u1);
		VectorAdd(self->s.origin, f1, driver->s.origin);
		VectorAdd(driver->s.origin, l1, driver->s.origin);
		VectorAdd(driver->s.origin, u1, driver->s.origin);
		driver->s.origin[2] += 16 * (fabs(up[0]) + fabs(up[1]));

		gi.linkentity(driver);
	}

	VectorSubtract(other->s.origin, self->s.origin, dir);
	dir[2] += 16;
	VectorNormalize(dir);

	if (!(other->svflags & SVF_MONSTER) && !other->client)
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, dir, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		
		// if it's still there, nuke it
		if (other)
		{
			// Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy(other->s.origin, save);
			VectorMA(other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1(other);
		}

		return;
	}

	if (level.time < self->touch_debounce_time || !self->dmg)
		return;

	if (other->client && other->groundentity == self)
	{
		// Don't cream riders who've become embedded - just do minor damage and *maybe* help them get unstuck by pushing them up.
		knockback = 2;
		T_Damage(other, self, self, tv(0, 0, 1), other->s.origin, vec3_origin, 1, knockback, 0, MOD_CRUSH);
	}
	else
	{
		knockback = (int)(fabsf(self->moveinfo.current_speed) * other->mass / 300.0);
		T_Damage(other, self, self, dir, other->s.origin, vec3_origin, self->dmg, knockback, 0, MOD_CRUSH);
	}

	self->touch_debounce_time = level.time + 0.5;
}

void tracktrain_reach_dest(edict_t *self)
{
	edict_t	*path = self->target_ent;

	if (path && path->pathtarget)
	{
		char *savetarget = path->target;
		path->target = path->pathtarget;

		if (self->owner)
			G_UseTargets(path, self->owner);
		else
			G_UseTargets(path, self);

		path->target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self->inuse)
			return;

		if (path->spawnflags & SF_PATH_FIREONCE)
			path->pathtarget = NULL;
	}

	if (path && (path->spawnflags & SF_PATH_DISABLE_TRAIN))
	{
		self->spawnflags |= SF_TRACKTRAIN_NOCONTROL;
		if (self->owner)
			tracktrain_disengage(self);
	}

	if (path && path->speed)
	{
		if (path->spawnflags & SF_PATH_ABS_SPEED)
		{
			self->moveinfo.speed = path->speed;
			self->moveinfo.next_speed = self->moveinfo.speed;
			self->moveinfo.state = (self->moveinfo.state >= STOP ? FAST : RFAST);
		}
		else
		{
			self->moveinfo.speed = path->speed * self->speed;
			self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
		}

		self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;
	}

	tracktrain_next(self);
}

qboolean is_backing_up(edict_t *train)
{
	vec3_t forward, v_norm;

	VectorCopy(train->velocity, v_norm);
	VectorNormalize(v_norm);
	AngleVectors(train->s.angles, forward, NULL, NULL);

	return DotProduct(forward, v_norm) < 0.0;
}

edict_t *NextPathTrack(edict_t *train, edict_t *path)
{
	edict_t		*next = NULL;
	vec3_t		forward;
	vec3_t		to_next;
	qboolean	in_reverse;

	AngleVectors(train->s.angles, forward, NULL, NULL);

	if ((train->moveinfo.prevstate < STOP && train->moveinfo.state > STOP)
	 || (train->moveinfo.prevstate > STOP && train->moveinfo.state < STOP))
	{
		next = path->prevpath;
		if (next)
		{
			VectorSubtract(next->s.origin, path->s.origin, to_next);
			VectorNormalize(to_next);

			if ((train->moveinfo.state > STOP && DotProduct(forward, to_next) < 0)
			 || (train->moveinfo.state < STOP && DotProduct(forward, to_next) > 0))
			{
				next = NULL;
			}
			else
			{
				next->prevpath = path;
				return next;
			}
		}
	}

	if (train->moveinfo.state == STOP)
		in_reverse = is_backing_up(train);
	else
		in_reverse = (train->moveinfo.state < STOP);

	if (in_reverse)
	{
		if (path->spawnflags & SF_PATH_ALTPATH)
		{
			next = G_PickTarget(path->target);
			if (next)
			{
				VectorSubtract(next->s.origin, path->s.origin, to_next);
				VectorNormalize(to_next);
				if (DotProduct(forward, to_next) > 0)
					next = NULL;
			}
		}

		if (!next)
		{
			next = path->prevpath;

			if (next)
			{
				// Ensure we don't flipflop
				VectorSubtract(next->s.origin, path->s.origin, to_next);
				VectorNormalize(to_next);

				if (DotProduct(forward, to_next) > 0)
					next = NULL;
			}
			
			if (!next)
			{
				// Find path_track whose target or target2 is set to the current path_track
				for (int i = maxclients->value; i < globals.num_edicts && !next; i++)
				{
					edict_t *e = &g_edicts[i];
					if (!e->inuse || e == path || !e->classname || Q_stricmp(e->classname, "path_track"))
						continue;
					
					if (e->target && !Q_stricmp(e->target, path->targetname))
					{
						next = e;
						VectorSubtract(next->s.origin, path->s.origin, to_next);
						VectorNormalize(to_next);

						if (DotProduct(forward, to_next) > 0)
							next = NULL;
					}

					if (!next && e->target2 && !Q_stricmp(e->target2, path->targetname))
					{
						next = e;
						VectorSubtract(next->s.origin, path->s.origin, to_next);
						VectorNormalize(to_next);

						if (DotProduct(forward, to_next) > 0)
							next = NULL;
					}
				}
			}

			if (!next)
			{
				float dot;

				// Finally, check this path_track's target and target2
				if (path->target)
				{
					next = G_PickTarget(path->target);
					if (next)
					{
						VectorSubtract(next->s.origin, path->s.origin, to_next);
						VectorNormalize(to_next);
						dot = DotProduct(forward, to_next);

						if (dot > 0)
							next = NULL;
					}
				}

				if (path->target2 && !(path->spawnflags & SF_PATH_ALTPATH))
				{
					float dot2;
					
					edict_t *next2 = G_PickTarget(path->target2);
					if (next2 == path)
						next2 = NULL;
					
					if (next2)
					{
						VectorSubtract(next2->s.origin, path->s.origin, to_next);
						VectorNormalize(to_next);
						dot2 = DotProduct(forward, to_next);
						
						if (dot2 > 0)
						{
							next2 = NULL;
						}
						else if (!next)
						{
							next = next2;
							next2 = NULL;
						}
					}
					
					if (next && next2 && dot2 < dot)
						next = next2;
				}
			}
		}
	}
	else	// Moving forward
	{
		float dot;

		if (path->target)
		{
			next = G_PickTarget(path->target);
			if (next)
			{
				VectorSubtract(next->s.origin, path->s.origin, to_next);
				VectorNormalize(to_next);
				dot = DotProduct(forward, to_next);

				if (dot < 0)
					next = NULL;
			}
		}

		if (path->target2 && !(path->spawnflags & SF_PATH_ALTPATH))
		{
			float dot2;

			edict_t *next2 = G_PickTarget(path->target2);
			if (next2 == path)
				next2 = NULL;

			if (next2)
			{
				VectorSubtract(next2->s.origin, path->s.origin, to_next);
				VectorNormalize(to_next);
				dot2 = DotProduct(forward, to_next);

				if (dot2 < 0)
				{
					next2 = NULL;
				}
				else if (!next)
				{
					next = next2;
					next2 = NULL;
				}
			}

			if (next && next2 && dot2 > dot)
				next = next2;
		}

		if (next == path)
			next = NULL;

		if (!next)
		{
			// Check for path_tracks that target (or target2) this path_track.
			for (int i = maxclients->value; i < globals.num_edicts && !next; i++)
			{
				edict_t *e = &g_edicts[i];
				if (!e->inuse || e == path || !e->classname || Q_stricmp(e->classname, "path_track"))
					continue;

				if (e->target && !Q_stricmp(e->target, path->targetname))
				{
					next = e;
					VectorSubtract(next->s.origin, path->s.origin, to_next);
					VectorNormalize(to_next);
					dot = DotProduct(forward, to_next);

					if (dot < 0)
						next = NULL;
				}

				if (e->target2 && !Q_stricmp(e->target2,path->targetname))
				{
					edict_t *next2 = e;
					VectorSubtract(next2->s.origin, path->s.origin, to_next);
					VectorNormalize(to_next);
					const float dot2 = DotProduct(forward, to_next);

					if (dot2 < 0)
					{
						next2 = NULL;
					}
					else if (!next)
					{
						next = next2;
						next2 = NULL;
					}

					if (next && next2 && dot2 > dot)
						next = next2;
				}
			}
		}
	}

	if (next)
		next->prevpath = path;

	return next;
}

void LookAhead(edict_t *train, vec3_t point, float dist)
{
	vec3_t	v;
	int		n = 0;
	
	edict_t *path = train->target_ent;
	if (!path || dist < 0)
		return;

	while (dist > 0)
	{
		n++;
		if (n > 20)
		{
			gi.dprintf("WTF??? n=%d\n",n);
			return;
		}

		VectorSubtract(path->s.origin, point, v);
		const float length = VectorLength(v);

		if (length >= dist)
		{
			VectorMA(point, dist / length, v, point);
			return;
		}

		dist -= length;
		VectorCopy(path->s.origin, point);

		path = NextPathTrack(train, path);
		if (!path)
			return;
	}
}

void train_angles(edict_t *train)
{
	vec3_t v, angles;

	VectorCopy(train->s.origin, v);
	v[2] -= train->viewheight;
	LookAhead(train, v, train->moveinfo.distance);
	v[2] += train->viewheight;
	VectorSubtract(v, train->s.origin, v);
	if (train->moveinfo.state < STOP || (train->moveinfo.state == STOP && is_backing_up(train)))
		VectorNegate(v, v);

	if (VectorLength(v))
	{
		vectoangles2(v, angles);
		train->ideal_yaw = angles[YAW];
		train->ideal_pitch = angles[PITCH];

		if (train->ideal_pitch < 0)
			train->ideal_pitch += 360;
	}
	else
	{
		train->ideal_pitch = train->s.angles[PITCH];
		train->ideal_yaw = train->s.angles[YAW];
	}

	// determine angular velocitys from wheelbase and target angles
	angles[PITCH] = train->ideal_pitch - train->s.angles[PITCH];
	angles[YAW] = train->ideal_yaw - train->s.angles[YAW];
	AnglesNormalize(angles);

	// If yaw angle is > 90, we're about to flipflop (there's no way ideal_yaw can be more than 90 because the path_track selection code doesn't allow that)
	if (angles[YAW] > 90 || angles[YAW] < -90)
	{
		angles[YAW] += 180;
		if (angles[PITCH] != 0)
			angles[PITCH] += 180;

		AnglesNormalize(angles);
	}

	train->pitch_speed = fabs(angles[PITCH]) * 10;
	train->yaw_speed = fabs(angles[YAW]) * 10;
}

void tracktrain_turn(edict_t *self)
{
	edict_t *train = self->enemy;
	if (!train || !train->inuse)
		return;

	self->nextthink = level.time + FRAMETIME;

	if (train->spawnflags & (SF_TRACKTRAIN_DISABLED | SF_TRACKTRAIN_OTHERMAP))
		return;

	// Train doesn't turn if at a complete stop
	if (!VectorLength(train->velocity))
	{
		VectorClear(train->avelocity);
		gi.linkentity(train);

		return;
	}

	train_angles(train);

	float idl_pitch = train->ideal_pitch;
	if (train->spawnflags & SF_TRACKTRAIN_NOPITCH)
		idl_pitch = train->s.angles[PITCH];

	train->avelocity[YAW] = GetAngularVelocity(train->yaw_speed, train->s.angles[YAW], train->ideal_yaw); //mxd
	train->avelocity[PITCH] = GetAngularVelocity(train->pitch_speed, train->s.angles[PITCH], idl_pitch); //mxd

	if (train->roll != 0)
	{
		float roll;

		if (train->moveinfo.state < STOP)
			roll = -train->roll;
		else
			roll = train->roll;

		if (train->spawnflags & SF_TRACKTRAIN_ROLLSPEED)
			roll *= VectorLength(train->velocity) / train->moveinfo.speed;

		if (train->avelocity[YAW] < -5)
			train->avelocity[ROLL] = track_AngleDistance(track_ApproachAngle(-roll, /*train->s.angles[ROLL],*/ roll * 2), train->s.angles[ROLL]);
		else if (train->avelocity[YAW] > 5)
			train->avelocity[ROLL] = track_AngleDistance(track_ApproachAngle( roll, /*train->s.angles[ROLL],*/ roll * 2), train->s.angles[ROLL]);
		else
			train->avelocity[ROLL] = track_AngleDistance(track_ApproachAngle(0, /*train->s.angles[ROLL],*/ roll * 4), train->s.angles[ROLL]) * 4;
	}

	gi.linkentity(train);
}


void tracktrain_next(edict_t *self)
{
	vec3_t dest;

	if (!self->target_ent)
	{
		self->s.sound = 0;
		return;
	}

	edict_t *ent = NextPathTrack(self, self->target_ent);

	if (ent && (ent->spawnflags & SF_PATH_DISABLED))
		ent = NULL;

	if (!ent)
	{
		// Dead end
		if (self->owner && (self->owner->svflags & SVF_MONSTER) && !self->target_ent->deathtarget)
		{
			// For monster drivers, immediately reverse course at dead ends (but NOT at dead ends that have a "deathtarget", which is usually an indication of a trackchange)
			self->moveinfo.prevstate = self->moveinfo.state;
			self->moveinfo.state = -self->moveinfo.state;
			self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
			self->think = tracktrain_think;
			self->think(self);
			self->monsterinfo.search_time = level.time;

			return;
		}

		VectorClear(self->velocity);
		VectorClear(self->avelocity);
		self->moveinfo.prevstate = self->moveinfo.state;
		self->moveinfo.state = STOP;
		self->s.sound = 0;
		self->moveinfo.current_speed = 0;
		self->moveinfo.next_speed = 0;
		gi.linkentity(self);

		if (self->owner)
		{
			VectorClear(self->owner->velocity);
			gi.linkentity(self->owner);
		}

		if (self->target_ent->deathtarget)
		{
			char *temp = self->target_ent->target;
			self->target_ent->target = self->target_ent->deathtarget;
			G_UseTargets(self->target_ent, self);
			self->target_ent->target = temp;
		}

		self->think = tracktrain_think;
		self->think(self);

		return;
	}

	self->target_ent = ent;
	self->target = ent->targetname;

	VectorCopy(ent->s.origin, dest);
	dest[2] += self->viewheight;
	VectorCopy(dest, self->moveinfo.end_origin);

	train_angles(self);

	if (!(self->spawnflags & SF_TRACKTRAIN_NOCONTROL) || !(self->spawnflags & SF_TRACKTRAIN_STARTOFF))
	{
		self->think = tracktrain_think;
		self->think(self);
	}
}

void func_tracktrain_find(edict_t *self)
{
	vec3_t vec;

	if (!self->target)
	{
		gi.dprintf("tracktrain_find: no target\n");
		return;
	}

	edict_t *ent = G_PickTarget(self->target);
	if (!ent)
	{
		gi.dprintf("tracktrain_find: target %s not found\n", self->target);
		return;
	}

	if (ent->speed)
	{
		self->moveinfo.speed = ent->speed * self->speed;
		self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;
		self->moveinfo.next_speed = self->moveinfo.state * self->moveinfo.speed / 3;
	}
	
	self->target_ent = ent;

	// Get angles to next path_track
	edict_t *next = G_PickTarget(ent->target);
	if (!next)
	{
		gi.dprintf("tracktrain_find: next target %s not found\n", ent->target);
		return;
	}

	VectorSubtract(next->s.origin, ent->s.origin, vec);
	vectoangles2(vec, self->s.angles);

	ent->think = tracktrain_turn;
	ent->enemy = self;
	ent->nextthink = level.time + FRAMETIME;

	VectorCopy(ent->s.origin, self->s.origin);
	self->s.origin[2] += self->viewheight;

	if (self->spawnflags & SF_TRACKTRAIN_OTHERMAP)
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
		self->spawnflags |= SF_TRACKTRAIN_DISABLED;
		self->nextthink = 0;
	}
	else
	{
		self->nextthink = level.time + FRAMETIME;
		self->think = tracktrain_next;
	}

	gi.linkentity(self);
}

void tracktrain_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & SF_TRACKTRAIN_STARTOFF)
	{
		if (self->spawnflags & SF_TRACKTRAIN_NOCONTROL)
		{
			self->moveinfo.state = FAST;
			self->moveinfo.next_speed = self->moveinfo.speed;

			if (self->sounds)
			{
				self->s.sound = gi.soundindex(va("%sspeed%d.wav", self->source, abs(self->moveinfo.state)));
		#ifdef LOOP_SOUND_ATTENUATION
				self->s.attenuation = self->attenuation;
		#endif
			}
			else
			{
				self->s.sound = 0;
			}
		}

		self->spawnflags &= ~SF_TRACKTRAIN_STARTOFF;
		self->think = tracktrain_think;
		self->think(self);
	}
	else
	{
		if (self->owner)
			tracktrain_disengage(self);

		self->moveinfo.state = STOP;
		self->moveinfo.next_speed = 0;
		self->s.sound = 0;
		self->think = NULL;
		self->nextthink = 0;
		self->spawnflags |= SF_TRACKTRAIN_STARTOFF;
	}
}

void SP_func_tracktrain(edict_t *self)
{
	self->class_id = ENTITY_FUNC_TRACKTRAIN;

	self->movetype = MOVETYPE_PUSH;
	self->flags |= FL_TRACKTRAIN;

	VectorClear(self->s.angles);
	self->blocked = tracktrain_blocked;

	if (!self->dmg)
		self->dmg = 100;

	// Wheelbase determines angular velocities
	if (st.distance)
		self->moveinfo.distance = st.distance;
	else
		self->moveinfo.distance = 50;

	// Origin rides by "height" above path_tracks
	if (st.height)
		self->viewheight = st.height;
	else
		self->viewheight = 4;

	// Default mass for collisions:
	self->mass = 2000;

	// Driving position
	if (VectorLength(self->bleft) == 0 && VectorLength(self->tright) == 0)
	{
		VectorSet(self->bleft, -8, -8, -8);
		VectorSet(self->tright, 8, 8, 8);
	}

	VectorAdd(self->bleft, self->tright, self->move_origin);
	VectorScale(self->move_origin, 0.5f, self->move_origin);

	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	// usermodel (for alias model trains) goes on modelindex2
	if (self->usermodel && strlen(self->usermodel))
	{
		char modelname[256];

		// check for "models/" already in path
		if (!strncmp(self->usermodel, "models/", 7))
			Com_sprintf(modelname, sizeof(modelname), "%s", self->usermodel);
		else
			Com_sprintf(modelname, sizeof(modelname), "models/%s", self->usermodel);

		self->s.modelindex2 = gi.modelindex(modelname);
	}

	if (!self->speed)
		self->speed = 100;

	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

	if (self->roll_speed)
	{
		self->roll = self->roll_speed;
		self->roll_speed = 0;
		self->spawnflags |= SF_TRACKTRAIN_ROLLSPEED;
	}

	if (self->health)
	{
		self->die = tracktrain_die;
		self->takedamage = DAMAGE_YES;
	}
	else
	{
		self->die = NULL;
		self->takedamage = DAMAGE_NO;
	}

	self->spawnflags &= ~SF_TRACKTRAIN_DISABLED; // insurance
	if (self->spawnflags & SF_TRACKTRAIN_NOCONTROL)
		self->spawnflags |= SF_TRACKTRAIN_STARTOFF;

	self->use = tracktrain_use;
	self->moveinfo.current_speed = 0;
	self->moveinfo.state = STOP;
	self->moveinfo.prevstate = STOP + 1; // Assumed, so that initial reverse works correctly
	self->s.sound = 0;
	self->turn_rider = 1;
	VectorClear(self->s.angles);

	if (self->target)
	{
		self->nextthink = level.time + FRAMETIME;
		self->think = func_tracktrain_find;
	}
	else if (!(self->spawnflags & SF_TRACKTRAIN_OTHERMAP))
	{
		gi.dprintf("func_tracktrain without a target at %s\n", vtos(self->absmin));
		G_FreeEdict(self);

		return;
	}

	if (!self->sounds)
		self->sounds = 1;

	if (self->sounds > 0)
	{
		self->sounds = min(9, self->sounds);
		self->source = gi.TagMalloc(10, TAG_LEVEL);
		Com_sprintf(self->source, 10, "train/%d/", self->sounds);
		gi.soundindex(va("%sspeed1.wav", self->source));
		gi.soundindex(va("%sspeed2.wav", self->source));
		gi.soundindex(va("%sspeed3.wav", self->source));
	}

	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;

	gi.linkentity(self);

}

void find_tracktrain(edict_t *self)
{
	// This gives game a chance to put player in place before restarting train
	if (!g_edicts[1].linkcount)
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	qboolean train_found = false;
	edict_t *train = G_Find(NULL, FOFS(targetname), self->targetname);
	while (train && !train_found)
	{
		if (!Q_stricmp(train->classname, "func_tracktrain"))
			train_found = true;
		else
			train = G_Find(train, FOFS(targetname), self->targetname);
	}

	if (!train_found)
	{
		gi.dprintf("find_tracktrain: no matching func_tracktrain with targetname=%s\n", self->targetname);
		G_FreeEdict(self);

		return;
	}

	train->solid = SOLID_BSP;
	train->svflags &= ~SVF_NOCLIENT;
	train->spawnflags = self->spawnflags;
	train->spawnflags &= ~(SF_TRACKTRAIN_OTHERMAP | SF_TRACKTRAIN_DISABLED);
	VectorCopy(self->s.origin, train->s.origin);
	VectorCopy(self->s.angles, train->s.angles);
	VectorCopy(self->bleft, train->bleft);
	VectorCopy(self->tright, train->tright);
	VectorClear(train->avelocity);
	train->viewheight = self->viewheight;
	train->speed = self->speed;
	train->moveinfo.distance = self->radius;
	train->moveinfo.accel = train->moveinfo.decel = train->moveinfo.speed = train->speed;
	train->moveinfo.state = train->moveinfo.prevstate = self->count;
	train->sounds = self->sounds;

	if (train->sounds > 0)
	{
		train->source = gi.TagMalloc(10, TAG_LEVEL);
		Com_sprintf(train->source, 10, "train/%d/", train->sounds);
	}

	if (train->moveinfo.state && train->sounds > 0)
	{
		train->s.sound = gi.soundindex(va("%sspeed%d.wav", train->source, abs(train->moveinfo.state)));
#ifdef LOOP_SOUND_ATTENUATION
		train->s.attenuation = train->attenuation;
#endif
	}
	else
	{
		train->s.sound = 0;
	}

	train->moveinfo.next_speed = train->moveinfo.state * train->moveinfo.speed / 3;
	
	// Assume train was already at it's "next_speed" when the level change occurred
	vec3_t forward;
	AngleVectors(train->s.angles, forward, NULL, NULL);
	VectorScale(forward, train->moveinfo.next_speed, train->velocity);
	
	// Force a wait before taking player input
	train->moveinfo.wait = 1;

	if (self->style && self->style <= game.maxclients && g_edicts[self->style].inuse)
	{
		train->owner = &g_edicts[self->style];
		train->owner->vehicle = train;

		if (train->owner->client)
		{
			train->owner->client->vehicle_framenum = level.framenum;
			train->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
		}

		VectorCopy(self->offset, train->offset);
	}
	else
	{
		train->owner = NULL;
	}

	gi.linkentity(train);

	if (self->target)
	{
		vec3_t dest;

		train->target = self->target;
		train->target_ent = G_Find(NULL, FOFS(targetname), train->target);
		VectorCopy(train->target_ent->s.origin, dest);
		dest[2] += train->viewheight;
		VectorCopy(dest, train->moveinfo.end_origin);
		train_angles(train);

		if (!(train->spawnflags & SF_TRACKTRAIN_NOCONTROL) || !(train->spawnflags & SF_TRACKTRAIN_STARTOFF))
		{
			train->think = tracktrain_think;
			train->think(train);
		}
	}
	else
	{
		gi.dprintf("info_train_start with no target\n");
	}

	G_FreeEdict(self);
}

void SP_info_train_start(edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf("crosslevel train with no targetname\n");
		G_FreeEdict(self);
	}

	self->class_id = ENTITY_INFO_TRAIN_START;
	self->think = find_tracktrain;
	self->nextthink = level.time + 1;
}