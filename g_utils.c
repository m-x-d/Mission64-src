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

// g_utils.c -- misc utility functions for game module

#include "g_local.h"


void G_ProjectSource(const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

void G_ProjectSource2 (const vec3_t point, const vec3_t distance, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1] + up[0] * distance[2];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1] + up[1] * distance[2];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + up[2] * distance[2];
}

/*
=============
G_Find

Searches all active entities for the next one that holds the matching string at fieldofs (use the FOFS() macro) in the structure.
Searches beginning at the edict after from, or the beginning if NULL.
NULL will be returned if the end of the list is reached.
=============
*/
edict_t *G_Find(edict_t *from, int fieldofs, char *match)
{
	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;

		char *s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;

		if (!Q_stricmp(s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area
=================
*/
edict_t *findradius (edict_t *from, const vec3_t org, float rad)
{
	vec3_t	eorg;

	if (!from)
		from = g_edicts;
	else
		from++;

	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse || from->solid == SOLID_NOT)
			continue;

		for (int j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);

		if (VectorLength(eorg) > rad)
			continue;

		return from;
	}

	return NULL;
}


/*
=============
G_PickTarget

Searches all active entities for the next one that holds the matching string at fieldofs (use the FOFS() macro) in the structure.
Searches beginning at the edict after from, or the beginning if NULL.
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES	8

edict_t *G_PickTarget(char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(true)
	{
		ent = G_Find(ent, FOFS(targetname), targetname);
		if (!ent)
			break;

		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.dprintf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}



void Think_Delay (edict_t *ent)
{
	G_UseTargets(ent, ent->activator);
	G_FreeEdict(ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.
If self.delay is set, a DelayedUse entity will be created that will actually do the SUB_UseTargets after that many seconds have passed.
Centerprints any self.message to the activator.
Search for (string)targetname in all entities that match (string)self.target and call their .use function
==============================
*/
void G_UseTargets(edict_t *ent, edict_t *activator)
{
//
// check for a delay
//
	if (ent->delay)
	{
		// create a temp object to fire at a later time
		edict_t *t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;

		if (!activator)
			gi.dprintf("Think_Delay with no activator for %s\n", ent->classname);

		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		t->noise_index = ent->noise_index;

		return;
	}
	
	
//
// print the message
//
	if (ent->message && !(activator->svflags & SVF_MONSTER))
	{
//		Lazarus - change so that noise_index < 0 means no sound
		safe_centerprintf (activator, "%s%s", "^3", ent->message); //mxd. Added text colouring

		if (ent->noise_index > 0)
			gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else if (ent->noise_index == 0)
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent->killtarget)
	{
		edict_t *t = NULL;
		while ((t = G_Find(t, FOFS(targetname), ent->killtarget)))
		{
			// Lazarus: remove LIVE killtargeted monsters from total_monsters
			if ((t->svflags & SVF_MONSTER) && (t->deadflag == DEAD_NO))
			{
				if (!t->dmgteam || strcmp(t->dmgteam, "player"))
					if (!(t->monsterinfo.aiflags & AI_GOOD_GUY))
						level.total_monsters--;
			}
			// and decrement secret count if target_secret is removed
			else if (!Q_stricmp(t->classname, "target_secret"))
			{
				level.total_secrets--;
			}
			// same deal with target_goal, but also turn off CD music if applicable
			else if (!Q_stricmp(t->classname,"target_goal"))
			{
				level.total_goals--;
				if (level.found_goals >= level.total_goals)
				gi.configstring(CS_CDTRACK, "0");
			}

			G_FreeEdict(t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

//
// fire targets
//
	if (ent->target)
	{
		edict_t *t = NULL;
		while ((t = G_Find(t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") && (!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating") || !Q_stricmp(ent->classname, "func_door_rot_dh")))
				continue;

			if (t == ent)
				gi.dprintf("WARNING: Entity used itself.\n");
			else if (t->use)
				t->use(t, ent, activator);

			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}


/*
=============
TempVector

This is just a convenience function for making temporary vectors for function calls
=============
*/
float *tv (float x, float y, float z)
{
	static	int		index;
	static	vec3_t	vecs[8];

	// use an array so that multiple tempvectors won't collide for a while
	float *v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function for printing vectors
=============
*/
char *vtos (vec3_t v)
{
	static	int		index;
	static	char	str[8][32];

	// use an array so that multiple vtos won't collide
	char *s = str[index];
	index = (index + 1)&7;

	Com_sprintf(s, 32, "(%6.2f %6.2f %6.2f)", v[0], v[1], v[2]); //mxd. Let's have a bit more precision

	return s;
}


vec3_t VEC_UP		= {0, -1, 0};
vec3_t MOVEDIR_UP	= {0, 0, 1};
vec3_t VEC_DOWN		= {0, -2, 0};
vec3_t MOVEDIR_DOWN	= {0, 0, -1};

void G_SetMovedir (vec3_t angles, vec3_t movedir)
{
	if (VectorCompare(angles, VEC_UP))
		VectorCopy(MOVEDIR_UP, movedir);
	else if (VectorCompare(angles, VEC_DOWN))
		VectorCopy(MOVEDIR_DOWN, movedir);
	else
		AngleVectors(angles, movedir, NULL, NULL);

	VectorClear(angles);
}


float vectoyaw2 (vec3_t vec)
{
	if (vec[PITCH] == 0)
	{
		if (vec[YAW] == 0)
			return 0;

		if (vec[YAW] > 0)
			return 90;

		return 270;
	} 

	float yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float vectoyaw(vec3_t vec)
{
	return (int)vectoyaw2(vec); //mxd
}


void vectoangles2 (vec3_t value1, vec3_t angles)
{
	float yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		const float forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


void vectoangles(vec3_t value1, vec3_t angles)
{
	vectoangles2(value1, angles); //mxd
	angles[PITCH] = floorf(angles[PITCH]);
	angles[YAW] =	floorf(angles[YAW]);
}


char *G_CopyString (char *in)
{
	char *out = gi.TagMalloc(strlen(in) + 1, TAG_LEVEL);
	strcpy(out, in);
	return out;
}


void G_InitEdict (edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;
	e->org_movetype = -1;
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated angles and bad trails.
=================
*/
edict_t *G_Spawn(void)
{
	int i;
	edict_t *e = &g_edicts[(int)maxclients->value + 1];

	for (i = maxclients->value + 1; i < globals.num_edicts; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of freeing and allocating, so relax the replacement policy
		if (!e->inuse && (e->freetime < 2 || level.time - e->freetime > 0.5))
		{
			G_InitEdict (e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.error("ED_Alloc: no free edicts");

	globals.num_edicts++;

	if (developer->value && readout->value)
		gi.dprintf("num_edicts = %d\n", globals.num_edicts);

	G_InitEdict(e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict(edict_t *ed)
{
	// Lazarus - if part of a movewith chain, remove from the chain and repair broken links
	if (ed->movewith)
	{
		edict_t	*parent = NULL;

		for (int i = 1; i < globals.num_edicts && !parent; i++)
		{
			edict_t *e = g_edicts + i;
			if (e->movewith_next == ed)
				parent = e;
		}

		if (parent)
			parent->movewith_next = ed->movewith_next;
	}

	gi.unlinkentity(ed); // unlink from world

	// Lazarus: In SP we no longer reserve slots for bodyque's
	int numclients = maxclients->value;
	if (deathmatch->value || coop->value)
		numclients += BODY_QUEUE_SIZE;
	
	if (ed - g_edicts <= numclients)
		return;

	// Lazarus: actor muzzle flash
	if (ed->flash)
	{
		memset(ed->flash, 0, sizeof(*ed));
		ed->flash->classname = "freed";
		ed->flash->freetime = level.time;
		ed->flash->inuse = false;
	}

	// Lazarus: reflections
	if (!(ed->flags & FL_REFLECT))
		DeleteReflection(ed, -1);

	memset(ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}

/*
============
G_TouchTriggers
============
*/
void G_TouchTriggers(edict_t *ent)
{
	edict_t *touch[MAX_EDICTS];

	// Lazarus: nothing touches anything if game is frozen
	if (level.freeze)
		return;

	// dead things don't activate triggers!
	if ((ent->client || (ent->svflags & SVF_MONSTER)) && ent->health <= 0)
		return;

	const int num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this list removed before we get to it (killtriggered)
	for (int i = 0; i < num; i++)
	{
		edict_t *hit = touch[i];

		if (!hit->inuse || !hit->touch)
			continue;

		if (ent->client && ent->client->spycam && !(hit->svflags & SVF_TRIGGER_CAMOWNER))
			continue;

		hit->touch(hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay to force all entities it covers to immediately touch it
============
*/
void G_TouchSolids (edict_t *ent)
{
	edict_t *touch[MAX_EDICTS];

	const int num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this list removed before we get to it (killtriggered)
	for (int i = 0; i < num; i++)
	{
		edict_t *hit = touch[i];

		if (!hit->inuse)
			continue;

		if (ent->touch)
			ent->touch(hit, ent, NULL, NULL);

		if (!ent->inuse)
			break;
	}
}


/*
=================
KillBox

Kills all entities that would touch the proposed new positioning of ent.  Ent should be unlinked before calling this!
=================
*/
qboolean KillBox(edict_t *ent)
{
	while (true)
	{
		const trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;

		// nail it
		T_Damage(tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent->solid)
			return false;
	}

	return true; // all clear
}

void AnglesNormalize(vec3_t vec)
{
	while (vec[0] > 180)
		vec[0] -= 360;
	while (vec[0] < -180)
		vec[0] += 360;
	while (vec[1] > 360)
		vec[1] -= 360;
	while (vec[1] < 0)
		vec[1] += 360;
}

float SnapToEights(float x)
{
	x *= 8.0;

	if (x > 0.0)
		x += 0.5;
	else
		x -= 0.5;

	return 0.125 * (int)x;
}


/* Lazarus - added functions */

void stuffcmd(edict_t *pent, char *pszCommand)
{
	gi.WriteByte(svc_stufftext);
	gi.WriteString(pszCommand);
	gi.unicast(pent, true);
}

qboolean point_infront(edict_t *self, const vec3_t point)
{
	vec3_t	vec;
	vec3_t	forward;
	
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(point, self->s.origin, vec);
	VectorNormalize(vec);
	const float dot = DotProduct(vec, forward);
	
	return (dot > 0.3f);
}

float AtLeast(float x, float dx)
{
	float xx = (floorf(x / dx - 0.5f) + 1.0f) * dx;
	if (xx < x)
		xx += dx;
	return xx;
}

edict_t	*LookingAt(edict_t *ent, int filter, vec3_t endpos, float *range)
{
	edict_t		*trigger[MAX_EDICTS];
	edict_t		*ignore;
	vec3_t		end, forward, start;
	vec3_t		dir, entp, mins, maxs;

	if (!ent->client)
	{
		if (endpos) VectorClear(endpos);
		if (range) *range = 0;
		return NULL;
	}

	VectorClear(end);

	if (ent->client->chasetoggle)
	{
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->client->chasecam->s.origin, start);
		ignore = ent->client->chasecam;
	}
	else if (ent->client->spycam)
	{
		AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
		VectorCopy(ent->s.origin, start);
		ignore = ent->client->spycam;
	}
	else
	{
		AngleVectors(ent->client->v_angle, forward, NULL, NULL);
		VectorCopy(ent->s.origin, start);
		start[2] += ent->viewheight;
		ignore = ent;
	}

	VectorMA(start, 8192, forward, end);
	
	// First check for looking directly at a pickup item
	VectorSet(mins,-4096,-4096,-4096);
	VectorSet(maxs, 4096, 4096, 4096);
	const int num = gi.BoxEdicts(mins, maxs, trigger, MAX_EDICTS, AREA_TRIGGERS);

	for (int i = 0; i < num; i++)
	{
		edict_t *who = trigger[i];

		if (!who->inuse || !who->item || !visible(ent, who) || !infront(ent, who))
			continue;

		VectorSubtract(who->s.origin, start, dir);
		const vec_t r = VectorLength(dir);
		VectorMA(start, r, forward, entp);

		if (entp[0] < who->s.origin[0] - 17) continue;
		if (entp[1] < who->s.origin[1] - 17) continue;
		if (entp[2] < who->s.origin[2] - 17) continue;
		if (entp[0] > who->s.origin[0] + 17) continue;
		if (entp[1] > who->s.origin[1] + 17) continue;
		if (entp[2] > who->s.origin[2] + 17) continue;

		if (endpos)
			VectorCopy(who->s.origin,endpos);

		if (range)
			*range = r;

		return who;
	}

	const trace_t tr = gi.trace(start, NULL, NULL, end, ignore, MASK_SHOT);
	if (tr.fraction == 1.0f)
	{
		// too far away
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if (!tr.ent)
	{
		// no hit
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if (!tr.ent->classname)
	{
		// should never happen
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if (strstr(tr.ent->classname, "func_") != NULL && (filter & LOOKAT_NOBRUSHMODELS))
	{
		// don't hit on brush models
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if (Q_stricmp(tr.ent->classname, "worldspawn") == 0 && (filter & LOOKAT_NOWORLD))
	{
		// world brush
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		return NULL;
	}

	if (endpos)
		VectorCopy(tr.endpos, endpos);

	if (range)
	{
		VectorSubtract(tr.endpos, start, start);
		*range = VectorLength(start);
	}

	return tr.ent;
}

void GameDirRelativePath(char *filename, char *output)
{
	cvar_t *basedir = gi.cvar("basedir", "", 0);
	cvar_t *gamedir = gi.cvar("gamedir", "", 0);

	if (strlen(gamedir->string))
		sprintf(output, "%s/%s/%s", basedir->string, gamedir->string, filename);
	else
		sprintf(output, "%s/%s", basedir->string, filename);
}

/* Lazarus: G_UseTarget is similar to G_UseTargets, but only triggers a single target rather than all entities matching target
			criteria. It *does*, however, kill all killtargets */

void Think_Delay_Single (edict_t *ent)
{
	G_UseTarget(ent, ent->activator, ent->target_ent);
	G_FreeEdict(ent);
}

void G_UseTarget (edict_t *ent, edict_t *activator, edict_t *target)
{
	// Check for a delay
	if (ent->delay)
	{
		// Create a temp object to fire at a later time
		edict_t *t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay_Single;
		t->activator = activator;
		t->target_ent = target;

		if (!activator)
			gi.dprintf("Think_Delay_Single with no activator\n");

		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		t->noise_index = ent->noise_index;

		return;
	}

	// Print the message
	if (ent->message && !(activator->svflags & SVF_MONSTER))
	{
		safe_centerprintf(activator, "%s", ent->message);

		if (ent->noise_index > 0)
			gi.sound(activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else if (ent->noise_index == 0)
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

	// Kill killtargets
	if (ent->killtarget)
	{
		edict_t *t = NULL;
		while ((t = G_Find(t, FOFS(targetname), ent->killtarget)))
		{
			// Lazarus: remove killtargeted monsters from total_monsters
			if (t->svflags & SVF_MONSTER)
			{
				if ((!t->dmgteam || strcmp(t->dmgteam, "player")) && !(t->monsterinfo.aiflags & AI_GOOD_GUY))
					level.total_monsters--;
			}

			G_FreeEdict(t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

	// Fire target
	if (target)
	{
		// doors fire area portals in a specific way
		if (!Q_stricmp(target->classname, "func_areaportal") && (!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating") || !Q_stricmp(ent->classname, "func_door_rot_dh")))
			return;

		if (target == ent)
			gi.dprintf("WARNING: Entity used itself.\n");
		else if (target->use)
			target->use(target, ent, activator);

		if (!ent->inuse)
			gi.dprintf("entity was removed while using target\n");
	}
}

/*
====================
IsIdMap
====================
*/

char *idmapnames[] = { "base1", "base2", "base3", "biggun", "boss1", "boss2", "bunk1", "city1", "city2", "city3", "command", "cool1", "fact1", "fact2", "fact3", "hangar1", "hangar2",
						  "jail1", "jail2", "jail3", "jail4", "jail5", "lab", "mine1", "mine2", "mine3", "mine4", "mintro", "power1", "power2", "security", "space","strike", "train",
						  "ware1", "ware2", "waste1", "waste2", "waste3", "q2dm1", "q2dm2", "q2dm3", "q2dm4", "q2dm5", "q2dm6", "q2dm7", "q2dm8", "base64", "city64", "sewer64", NULL }; //mxd

//Knightmare- IsIdMap checks if the current map is a stock id map, this is used for certain hacks.
qboolean IsIdMap (void)
{
	int counter = 0;
	while (idmapnames[counter])
	{
		if (Q_stricmp(level.mapname, idmapnames[counter]) == 0)
			return true;
		
		counter++;
	}

	return false;
}

void my_bprintf (int printlevel, char *fmt, ...)
{
	char	bigbuffer[0x10000];
	va_list	argptr;

	va_start(argptr, fmt);
	Q_vsnprintf(bigbuffer, sizeof(bigbuffer), fmt, argptr);
	va_end(argptr);

	if (dedicated->value)
		safe_cprintf(NULL, printlevel, bigbuffer);

	for (int i = 0; i < maxclients->value; i++)
	{
		edict_t *cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;

		safe_cprintf(cl_ent, printlevel, bigbuffer);
	}
}