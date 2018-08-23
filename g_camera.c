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

// g_camera.c (func_monitor)

#include "g_local.h"

void camera_off (edict_t *ent)
{
	if (!ent->client || !ent->client->spycam)
		return;

	if (ent->client->spycam->viewer == ent)
		ent->client->spycam->viewer = NULL;

	ent->client->spycam->flags &= ~FL_ROBOT;

	if (ent->client->spycam->svflags & SVF_MONSTER)
		ent->client->spycam->svflags &= ~SVF_NOCLIENT;

	VectorCopy(ent->client->camplayer->s.origin,ent->s.origin);
	gi.TagFree(ent->client->camplayer->client); 
	G_FreeEdict(ent->client->camplayer); 

	// set angles
	ent->movetype = MOVETYPE_WALK;
	ent->client->ps.pmove.pm_type = PM_NORMAL;

	for (int i = 0; i < 3; i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->org_viewangles[i] - ent->client->resp.cmd_angles[i]);

	VectorCopy(ent->client->org_viewangles, ent->client->resp.cmd_angles);
	VectorCopy(ent->client->org_viewangles, ent->s.angles);
	VectorCopy(ent->client->org_viewangles, ent->client->ps.viewangles);
	VectorCopy(ent->client->org_viewangles, ent->client->v_angle);
	
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
#ifdef KMQUAKE2_ENGINE_MOD
	ent->client->ps.rdflags &= ~(RDF_CAMERAEFFECT|RDF_LETTERBOX); // Knightmare- letterboxing
#endif
	ent->svflags &= ~SVF_NOCLIENT;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->solid = SOLID_BBOX;
	ent->client->camplayer = NULL;
	ent->client->spycam = NULL;
	gi.linkentity(ent);
	
	// if we were previously in third person view, restore it
	if (tpp->value)
		Cmd_Chasecam_Toggle(ent);

#ifdef KMQUAKE2_ENGINE_MOD
	// Knightmare- check if client-side chasecam was previously on
	if (ent->style & 2)
	{
		gi.cvar_forceset(CLIENT_THIRDPERSON_CVAR, "1");
		ent->style &= ~2;
	}
#endif
}

void faker_animate(edict_t *self)
{
	if (self->s.frame < 0 || self->s.frame > 39)
	{
		self->s.frame = 0;
	}
	else
	{
		self->s.frame++;
		if (self->s.frame > 39)
			self->s.frame = 0;
	}

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

void camera_on (edict_t *ent)
{
	edict_t		*monster;
	int			i;
	vec3_t		forward, left, up;

	if (!ent->client || !ent->client->spycam)
		return;

	// "viewer" can control camera aim (2nd player to come along and use camera cannot)
	edict_t *camera = ent->client->spycam;
	if (camera->monsterinfo.aiflags & AI_ACTOR)
		camera->flags |= FL_ROBOT;
	if (!camera->viewer)
		camera->viewer = ent;

	// Save current viewangles and restore them with camera_off
	VectorCopy(ent->client->v_angle, ent->client->org_viewangles);

	// Copy over all important player data to fake player
	ent->client->camplayer = G_Spawn();
	edict_t *faker = ent->client->camplayer;
	faker->s.frame = ent->s.frame; 
	VectorCopy(ent->s.origin, faker->s.origin); 
	VectorCopy(ent->velocity, faker->velocity); 
	VectorCopy(ent->s.angles, faker->s.angles); 
	faker->s = ent->s;
	faker->takedamage   = DAMAGE_AIM;
	faker->movetype     = MOVETYPE_WALK;
	faker->groundentity = ent->groundentity;
	faker->viewheight   = ent->viewheight;
	faker->inuse        = true;
	faker->classname    = "camplayer";
	faker->mass         = ent->mass;
	faker->solid        = SOLID_BBOX;
	faker->deadflag     = DEAD_NO;
	faker->clipmask     = MASK_PLAYERSOLID;
	faker->health       = ent->health;
	faker->light_level  = ent->light_level;
	faker->think        = faker_animate;
	faker->nextthink    = level.time + FRAMETIME;
	VectorCopy(ent->mins, faker->mins);
	VectorCopy(ent->maxs, faker->maxs);
	
	// create a client so you can pick up items/be shot/etc while in camera
	gclient_t *cl = (gclient_t *)gi.TagMalloc(sizeof(gclient_t), TAG_LEVEL);
	memset(cl, 0, sizeof(gclient_t));
	ent->client->camplayer->client = cl; 
	ent->client->camplayer->target_ent = ent;
	gi.linkentity(faker); 

	AngleVectors(camera->s.angles, forward, left, up);

	VectorMA(camera->s.origin, camera->move_origin[0], forward, ent->s.origin);
	VectorMA(ent->s.origin,   -camera->move_origin[1], left,    ent->s.origin);
	VectorMA(ent->s.origin,    camera->move_origin[2], up,      ent->s.origin);

	ent->movetype = MOVETYPE_NOCLIP;
	ent->clipmask = 0;
	ent->solid    = SOLID_NOT;
	VectorClear(ent->velocity);
	ent->client->ps.gunindex = 0; 
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

#ifdef KMQUAKE2_ENGINE_MOD
	// Knightmare- Camera effect and letterboxing
	if (ent->client->monitor->spawnflags & 64)
		ent->client->ps.rdflags |= RDF_CAMERAEFFECT;
	if (ent->client->monitor->spawnflags & 128)
		ent->client->ps.rdflags |= RDF_LETTERBOX;
#endif

	ent->svflags |= SVF_NOCLIENT;

	// Check to see if we're the enemy of any monster. If so, make the faker the enemy
	for (i = maxclients->value + 1, monster = g_edicts + i; i < globals.num_edicts; i++, monster++)
	{
		if (!monster->inuse || !(monster->svflags & SVF_MONSTER))
			continue;

		if (monster->enemy == ent)
		{
			monster->enemy = faker;
			FoundTarget(monster);
		}
	}

	if (ent->client->spycam->viewmessage)
		safe_centerprintf(ent, ent->client->spycam->viewmessage);
}


edict_t *G_FindNextCamera (edict_t *camera, edict_t *monitor)
{
	if (!monitor->target)
		return NULL;

	// If we already have a camera that's a monster, make it visible now
	if (camera && (camera->svflags & SVF_MONSTER))
	{
		camera->svflags &= ~SVF_NOCLIENT;
		gi.linkentity(camera);
	}

	// First, determine if we're going to use the "count" to get the next camera, or just scan through the list of entities.
	// If count for the first camera in the map is 0, then we'll just use the map order.
	edict_t *next = G_Find(NULL, FOFS(targetname), monitor->target);
	if (!next)
		return NULL;

	if (!next->count)
	{
		if (camera)
		{
			next = camera;
			next++;
		}
		else
		{
			next = g_edicts;
		}
		
		for ( ; next < &g_edicts[globals.num_edicts]; next++)
		{
			if (next == camera || !next->inuse || next->deadflag == DEAD_DEAD || !next->targetname)
				continue;

			// Don't select "inactive" cameras
			if (!Q_stricmp(next->classname, "turret_breach") && (next->spawnflags & 16))
				continue;

			if (!Q_stricmp(next->targetname, monitor->target))
				goto found_one;
		}

		next = g_edicts;
		for ( ; next < camera; next++)
		{
			if (next == camera || !next->inuse || next->deadflag == DEAD_DEAD || !next->targetname)
				continue;

			// don't select "inactive" cameras
			if (!Q_stricmp(next->classname, "turret_breach") && (next->spawnflags & 16))
				continue;

			if (!Q_stricmp(next->targetname, monitor->target))
				goto found_one;
		}
	}
	else
	{
		int which = 1;
		if (camera)
		{
			which = camera->count + 1;

			if (which > monitor->count) 
				which = 1;
		}

		const int start = which;
		next = g_edicts + 1;

		while (true)
		{
			if (next->targetname && !Q_stricmp(next->targetname, monitor->target) && next->count == which)
			{
				if (!next->inuse || next->deadflag == DEAD_DEAD || (!Q_stricmp(next->classname, "turret_breach") && (next->spawnflags & 16)))
				{
					next = g_edicts;
					which++;

					if (which > monitor->count)
						which = 1;

					if (which == start)
						return NULL;
				}
				else
				{
					goto found_one;
				}
			}

			if (next == &g_edicts[globals.num_edicts - 1])
			{
				next = g_edicts;
				which++;

				if (which > monitor->count)
					which = 1;

				if (which == start)
					return NULL;
			}

			next++;
		}
	}

	return NULL;

found_one:
	if (!(monitor->spawnflags & 32) && (next->svflags & SVF_MONSTER))
		next->svflags |= SVF_NOCLIENT;

	return next;
}

edict_t *G_FindPrevCamera (edict_t *camera, edict_t *monitor)
{
	edict_t	*newcamera;

	if (!monitor->target) return NULL;

	// If we already have a camera that's a monster, make it visible now
	if (camera && (camera->svflags & SVF_MONSTER))
	{
		camera->svflags &= ~SVF_NOCLIENT;
		gi.linkentity(camera);
	}

	// First, determine if we're going to use the "count" to get the next camera, or just scan through the list of entities.
	// If count for the first camera in the map is 0, then we'll just use the map order.
	edict_t *prev = G_Find(NULL, FOFS(targetname), monitor->target);
	if (!prev)
		return NULL;

	if (!prev->count)
	{
		newcamera = NULL;
		for (prev = g_edicts ; prev < &g_edicts[globals.num_edicts] ; prev++)
		{
			if (prev == camera)
			{
				if (newcamera)
					goto found_one;

				continue;
			}

			if (!prev->inuse || prev->deadflag == DEAD_DEAD || !prev->targetname)
				continue;

			// don't select "inactive" cameras
			if (!Q_stricmp(prev->classname,"turret_breach") && (prev->spawnflags & 16))
				continue;

			if (!Q_stricmp(prev->targetname, monitor->target))
				newcamera = prev;
		}
	}
	else
	{
		int which = monitor->count;
		if (camera)
		{
			which = camera->count - 1;

			if (which <= 0) 
				which = monitor->count;
		}

		const int start = which;
		prev = g_edicts + 1;

		while (true)
		{
			if (prev->targetname && !Q_stricmp(prev->targetname, monitor->target) && prev->count == which)
			{
				if (!prev->inuse || prev->deadflag == DEAD_DEAD || (!Q_stricmp(prev->classname, "turret_breach") && (prev->spawnflags & 16)))
				{
					prev = g_edicts;
					which--;

					if (which <= 0)
						which = monitor->count;

					if (which == start)
						return NULL;
				}
				else
				{
					newcamera = prev;
					goto found_one;
				}
			}

			if (prev == &g_edicts[globals.num_edicts - 1])
			{
				prev = g_edicts;
				which--;

				if (which <= 0)
					which = monitor->count;

				if (which == start)
					return NULL;
			}

			prev++;
		}
	}

found_one:
	if (newcamera && !(monitor->spawnflags & 32) && (newcamera->svflags & SVF_MONSTER)) // Knightmare added- check this pointer!
		newcamera->svflags |= SVF_NOCLIENT;

	return newcamera;
}

void use_camera (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!activator->client || activator->client->spycam)
		return; // already using camera

	edict_t *target = G_FindNextCamera(NULL, self);
	if (!target)
		return;

	// If currently in thirdperson, turn that sucker off
	if (tpp->value && activator->client->chasetoggle)
		Cmd_Chasecam_Toggle(activator);

#ifdef KMQUAKE2_ENGINE_MOD
	// Knightmare- check for client-side chasecam
	if (!deathmatch->value && !coop->value && cl_thirdperson->value)
	{
		gi.cvar_forceset(CLIENT_THIRDPERSON_CVAR, "0");
		activator->style |= 2;
	}
#endif

	activator->client->spycam = target;
	activator->client->monitor = self;
	camera_on(activator);
}

void func_monitor_init (edict_t *self)
{
	self->count = 0;
	edict_t *camera = NULL;
	while ((camera = G_Find(camera, FOFS(targetname), self->target)) != NULL)
		self->count++;

	if (!self->count)
		self->s.effects = 0; // don't animate a func_monitor that has no cameras
}

void SP_func_monitor (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("func_monitor without a target at %s\n", vtos(self->s.origin));
		G_FreeEdict(self);

		return;
	}

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	gi.setmodel(self, self->model);
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BSP;
	self->use = use_camera;
	self->think = func_monitor_init;
	self->nextthink = level.time + 2 * FRAMETIME;

	gi.linkentity(self);
}