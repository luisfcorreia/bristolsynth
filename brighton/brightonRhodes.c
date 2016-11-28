
/*
 *  Diverse Bristol audio routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <fcntl.h>

#include "brighton.h"
#include "brightonMini.h"
#include "brightoninternals.h"

static int rhodesInit();
static int rhodesConfigure();
static int rhodesCallback(brightonWindow *, int, int, float);
static int rhodesMidiCallback(brightonWindow *, int, int, float);

extern guimain global;

#include "brightonKeys.h"

#define OPTS_PANEL 0
#define MOD_PANEL 1
#define MEM_PANEL 2
#define KEY_PANEL 3

#define FIRST_DEV 0

#define MOD_COUNT 4
#define OPTS_COUNT 12
#define MEM_COUNT 17

#define OPTS_START 0
#define MOD_START OPTS_COUNT
#define MEM_START (MOD_COUNT + MOD_START)

#define ACTIVE_DEVS (OPTS_COUNT + MOD_COUNT - 1)
#define DEVICE_COUNT (MOD_COUNT + OPTS_COUNT + MEM_COUNT)
#define DISPLAY_DEV (MEM_COUNT - 1)
#define MEM_MGT ACTIVE_DEVS

#define MIDI_MGT (MEM_MGT + 12)

extern int memCallback(brightonWindow * , int, int, float);
extern brightonLocations mem[];

/*
 * This structure is for device definition. The structure is defined in 
 * include/brighton.h, further definitions in brighton/brightonDevtable.h and
 * include/brightoninternals.h
 *
 *	typedef int (*brightonCallback)(int, float);
 *	typedef struct BrightonLocations {
 *		int device; 0=rotary, 1=scale, etc.
 *		float relx, rely; relative position with regards to 1000 by 1000 window
 *		float relw, relh; relative height.
 *		int from, to;
 *		brightonCallback callback; specific to this dev
 *		char *image; bitmap. If zero take a device default.
 *		int flags;
 *	} brightonLocations;
 *
 * This example is for a rhodesBristol type synth interface.
 */

#define R1 0

#define W1 100
#define L1 600

#define C1 40
#define C2 190
#define C3 340
#define C4 490
#define C5 700
#define C6 850

static brightonLocations locations[MOD_COUNT] = {
	{"Bass", 0, 90, 100, 300, 600, 0, 1, 0, 0, 0, 0},
	{"Volume", 0, 123, 100, 300, 600, 0, 1, 0, 0, 0, 0},
	{"", 2, 50, 200, 30, 400, 0, 1, 0,
		"bitmaps/buttons/rockerwhite.xpm", 0, 0},
	{"Chorus", 0, 156, 100, 300, 600, 0, 1, 0, 0, 0, 0},
};

#define S1 200

/* Options should only be a few piano selectors, no programmers. */
static brightonLocations options[OPTS_COUNT] = {
	{"", 2, 100, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 200, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 300, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 400, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 500, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 600, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 700, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 2, 800, 500, 50, 200, 0, 1, 0,
		"bitmaps/buttons/pressoff.xpm", "bitmaps/buttons/presson.xpm", 0},
	{"", 0, 100, 200, 60, 300, 0, 1, 0, "bitmaps/knobs/knobrednew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, 200, 200, 60, 300, 0, 1, 0, "bitmaps/knobs/knobyellownew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, 300, 200, 60, 300, 0, 1, 0, "bitmaps/knobs/knobgreennew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},
	{"", 0, 400, 200, 60, 300, 0, 1, 0, "bitmaps/knobs/knobbluenew.xpm",
		"bitmaps/knobs/alpharotary.xpm", 0},
};

/*
 * This is a set of globals for the main window rendering. Again taken from
 * include/brighton.h
 */
brightonApp rhodesApp = {
	"rhodes",
	0, /* no blueprint on wood background. */
	"bitmaps/textures/leather.xpm",
	0,
	rhodesInit,
	rhodesConfigure, /* 3 callbacks */
	rhodesMidiCallback,
	0,
	{16, 0, 2, 2, 5, 520, 0, 1},
	750, 250, 0, 0,
	7, /* panel count */
	{
		{
			"Opts",
			"bitmaps/blueprints/rhodesopts.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			rhodesCallback,
			19, 40, 600, 510,
			OPTS_COUNT,
			options
		},
		{
			"Mods",
			"bitmaps/blueprints/rhodes.xpm",
			"bitmaps/images/rhodesplate.xpm",
			BRIGHTON_STRETCH,
			0,
			0,
			rhodesCallback,
			12, 559, 976, 100,
			MOD_COUNT,
			locations
		},
		{
			"Mem",
			"bitmaps/blueprints/genmem.xpm",
			"bitmaps/textures/metal5.xpm", /* flags */
			0x020,
			0,
			0,
			memCallback,
			619, 40, 362, 510,
			MEM_COUNT,
			mem
		},
		{
			"Keyboard",
			0,
			"bitmaps/newkeys/dkbg.xpm", /* flags */
			0x020|BRIGHTON_STRETCH,
			0,
			0,
			keyCallback,
			52, 662, 900, 305,
			KEY_COUNT_6_OCTAVE,
			keys6octave
		},
		{
			"Rhodes",
			0,
			"bitmaps/images/rhodes.xpm",
			BRIGHTON_STRETCH, /* flags */
			0,
			0,
			0,
			12, 30, 976, 530,
			0,
			0
		},
		{
			"backing",
			0,
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			0,
			953, 662, 35, 310,
			0,
			0
		},
		{
			"backing",
			0,
			"bitmaps/textures/metal6.xpm", /* flags */
			BRIGHTON_STRETCH,
			0,
			0,
			0,
			12, 662, 40, 310,
			0,
			0
		},
	}
};

/*static dispatcher dispatch[DEVICE_COUNT]; */

static int
rhodesMidiSendMsg(void *synth, int fd, int chan, int c, int o, int v)
{
/*printf("rhodesMidiSendMsg(%i, %i, %i)\n", c, o, v); */
	bristolMidiSendMsg(fd, chan, c, o, v);
	return(0);
}

/*
 * For the sake of ease of use, links have been placed here to be called
 * by any of the devices created. They would be better in some other file,
 * perhaps with this as a dispatch.
 *
 * Param refers to the device index in the locations table given below.
 */
static int
rhodesCallback(brightonWindow * win, int panel, int index, float value)
{
	guiSynth *synth = findSynth(global.synths, win);
	int sendvalue;

/*	printf("rhodesCallback(%i, %i, %f): %x\n", panel, index, value, synth); */

	if (synth == 0)
		return(0);

	if ((index >= DEVICE_COUNT) || ((synth->flags & OPERATIONAL) == 0))
		return(0);

	if (rhodesApp.resources[panel].devlocn[index].to == 1)
		sendvalue = value * C_RANGE_MIN_1;
	else
		sendvalue = value;

	switch (panel) {
		case OPTS_PANEL:
			index += OPTS_START;
			break;
		case MOD_PANEL:
			index += MOD_START;
			break;
		case MEM_PANEL:
			index += MEM_START;
			break;
	}

	synth->mem.param[index] = value;

	if ((!global.libtest) || (index >= ACTIVE_DEVS))
		synth->dispatch[index].routine(synth,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#ifdef DEBUG
	else
		printf("dispatch[%p,%i](%i, %i, %i, %i, %i)\n", synth, index,
			global.controlfd, synth->sid,
			synth->dispatch[index].controller,
			synth->dispatch[index].operator,
			sendvalue);
#endif

	return(0);
}

static void
rhodesProgramme(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

	if (synth->dispatch[OPTS_START].other2)
	{
		synth->dispatch[OPTS_START].other2 = 0;
		synth->mem.param[synth->dispatch[OPTS_START].other1] = 0;
		return;
	}

	if ((synth->flags & MEM_LOADING) == 0)
	{
		if (synth->dispatch[OPTS_START].other1 != -1)
		{
			synth->dispatch[OPTS_START].other2 = 1;

			if (synth->dispatch[OPTS_START].other1 != op)
				event.value = 0;
			else
				event.value = 1;

			brightonParamChange(synth->win, OPTS_PANEL,
				synth->dispatch[OPTS_START].other1, &event);
		}
		synth->dispatch[OPTS_START].other1 = op;
		synth->mem.param[op] = 1;
	} else
		return;

	if (synth->flags & SUPPRESS)
		return;

	if (value != 0)
	{
		float *memhold;
		float mem[256];
		int optr, ind;

/*		printf("rhodesProgramme(%x, %i, %i, %i, %i, %i)\n", */
/*			synth, fd, chan, cont, op, value); */

		/*
		 * Each of the 8 voices has an associated set of DX parameters. We
		 * need to get hold of these, and send them directly over our config
		 * interface tap - we cannot forward via the normal GUi interface since
		 * we do not have these parameters mapped.
		 *
		 * The memories are mapped as 6 operators, each with 20 parameters,
		 * of which 13 are active, and then we have 4 additional parameters for
		 * algo, pitch, glide and volume.
		 */
		memhold = &synth->mem.param[0];

		synth->mem.param = &mem[0];

		ind = 1000 + op;

		/*
		 * Load the memory parameters with no callbacks. We have to force the
		 * loading since we are using 'rhodes' as our type, but the memories
		 * are actually typed as DX.
		 */
		if (loadMemory(synth, "rhodes", 0, ind, 148, 0,
			BRISTOL_NOCALLS|BRISTOL_FORCE) < 0)
		{
			printf("Memory not found, returning\n");
			synth->mem.param = memhold;
			synth->dispatch[OPTS_START].other1 = op;
			return;
		}

		for (optr = 0; optr < 6; optr++)
		{
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				126, optr * 10, mem[optr * 20] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 1, mem[optr * 20 + 1] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 0, mem[optr * 20 + 2]);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 6, mem[optr * 20 + 3] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				126, optr * 10 + 1, mem[optr * 20 + 4] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 2, mem[optr * 20 + 5] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 3, mem[optr * 20 + 6] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 4, mem[optr * 20 + 7] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 5, mem[optr * 20 + 8] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 7, mem[optr * 20 + 9] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				126, optr * 10 + 2, mem[optr * 20 + 10] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				126, optr * 10 + 3, mem[optr * 20 + 11] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 9, mem[optr * 20 + 12] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 10, mem[optr * 20 + 13] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 11, mem[optr * 20 + 14] * C_RANGE_MIN_1);
			rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
				optr, 12, mem[optr * 20 + 15] * C_RANGE_MIN_1);
		}

		/*
		 * These go through all the algorithms, looking for a single nonzero
		 */
		for (ind = 0; ind < 24; ind++)
		{
			if (mem[120 + ind] != 0)
			{
				bristolMidiSendMsg(global.controlfd, synth->sid, 126, 101, ind);
				break;
			}
		}

		rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
			126, 99, 0);
		rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
			126, 100, 0);
		rhodesMidiSendMsg(synth, global.controlfd, synth->sid,
			126, 102, C_RANGE_MIN_1);

		synth->mem.param = memhold;
		synth->dispatch[OPTS_START].other1 = op;
	}
}

static int
rhodesMidiCallback(brightonWindow *win, int command, int value, float v)
{
	guiSynth *synth = findSynth(global.synths, win);

	printf("midi callback: %x, %i\n", command, value);

	switch(command)
	{
		case MIDI_PROGRAM:
			printf("midi program: %x, %i\n", command, value);
			synth->location = value;

			rhodesProgramme(synth, global.controlfd, synth->sid,
				synth->dispatch[OPTS_START].controller,
				synth->dispatch[OPTS_START].operator, 1.0f);

			break;
		case MIDI_BANK_SELECT:
			printf("midi banksel: %x, %i\n", command, value);
			synth->bank = value;
			break;
	}
	return(0);
}

static void
rhodesPanelSwitch(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
	brightonEvent event;

	id->flags |= SUPPRESS;
/*printf("rhodesPanelSwitch()\n"); */
	/* 
	 * If the sendvalue is zero, then withdraw the opts window, draw the
	 * slider window, and vice versa.
	 */
	if (value == 0)
	{
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 0, -1, &event);
		event.intvalue = 0;
		brightonParamChange(id->win, 2, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 4, -1, &event);
	} else {
		event.type = BRIGHTON_EXPOSE;
		event.intvalue = 0;
		brightonParamChange(id->win, 4, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 0, -1, &event);
		event.intvalue = 1;
		brightonParamChange(id->win, 2, -1, &event);
	}
	id->flags &= ~SUPPRESS;
}

static void
rhodesVolume(guiSynth *id, int fd, int chan, int cont, int op, int value)
{
/*printf("rhodesVolume(%i)\n", value); */
	bristolMidiSendMsg(global.controlfd, chan, 126, 102, value);
}

static void
rhodesBoost(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
/*printf("rhodesBoost(%i)\n", value); */
	bristolMidiControl(global.controlfd, synth->sid, 0, 1, value >> 7);
}

static void
rhodesChorus(guiSynth *synth, int fd, int chan, int cont, int op, int value)
{
/*printf("Rhodes chorus: %i\n", value); */
	bristolMidiSendMsg(global.controlfd, synth->sid, 123, 0, value);
	bristolMidiSendMsg(global.controlfd, synth->sid, 123, 3, value);
}

/*
 * Any location initialisation required to run the callbacks. For bristol, this
 * will connect to the engine, and give it some base parameters.
 * May need to generate some application specific menus.
 * Will also then make specific requests to some of the devices to alter their
 * rendering.
 */
static int
rhodesInit(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	dispatcher *dispatch;
	int i;

	if (synth == 0)
	{
		synth = findSynth(global.synths, 0);
		if (synth == 0)
		{
			printf("cannot init\n");
			return(0);
		}
	}

	synth->win = win;

	printf("Initialise the rhodes link to bristol: %p\n", synth->win);

	synth->mem.param = (float *) brightonmalloc(DEVICE_COUNT * sizeof(float));
	synth->mem.count = DEVICE_COUNT;
	synth->mem.active = ACTIVE_DEVS;
	synth->dispatch = (dispatcher *)
		brightonmalloc(DEVICE_COUNT * sizeof(dispatcher));
	dispatch = synth->dispatch;

	/*
	 * The rhodes is actually a DX algorithm but it may eventually use a 
	 * wrapper in the engine such that the FM coding is fed into a chorus.
	synth->synthtype = BRISTOL_DX;
	 */

	/*
	 * We really want to have three connection mechanisms. These should be
	 *	1. Unix named sockets.
	 *	2. UDP sockets.
	 *	3. MIDI pipe.
	 */
	if (!global.libtest)
		if ((synth->sid = initConnection(&global, synth)) < 0)
			return(-1);

	for (i = 0; i < DEVICE_COUNT; i++)
		synth->dispatch[i].routine = rhodesMidiSendMsg;

	synth->dispatch[MOD_START + 0].routine = (synthRoutine) rhodesBoost;
	synth->dispatch[MOD_START + 1].routine = (synthRoutine) rhodesVolume;
	synth->dispatch[MOD_START + 2].routine = (synthRoutine) rhodesPanelSwitch;
	synth->dispatch[MOD_START + 3].controller = 123;
	synth->dispatch[MOD_START + 3].operator = 0;
	synth->dispatch[MOD_START + 3].routine = (synthRoutine) rhodesChorus;

	synth->dispatch[OPTS_START + 0].routine
		= synth->dispatch[OPTS_START + 1].routine
		= synth->dispatch[OPTS_START + 2].routine
		= synth->dispatch[OPTS_START + 3].routine
		= synth->dispatch[OPTS_START + 4].routine
		= synth->dispatch[OPTS_START + 5].routine
		= synth->dispatch[OPTS_START + 6].routine
		= synth->dispatch[OPTS_START + 7].routine
		= (synthRoutine) rhodesProgramme;
	synth->dispatch[OPTS_START + 0].operator = 0;
	synth->dispatch[OPTS_START + 1].operator = 1;
	synth->dispatch[OPTS_START + 2].operator = 2;
	synth->dispatch[OPTS_START + 3].operator = 3;
	synth->dispatch[OPTS_START + 4].operator = 4;
	synth->dispatch[OPTS_START + 5].operator = 5;
	synth->dispatch[OPTS_START + 6].operator = 6;
	synth->dispatch[OPTS_START + 7].operator = 7;

	synth->dispatch[OPTS_START + 8].controller
		= synth->dispatch[OPTS_START + 9].controller
		= synth->dispatch[OPTS_START + 10].controller
		= synth->dispatch[OPTS_START + 11].controller = 123;
	synth->dispatch[OPTS_START + 8].operator = 0;
	synth->dispatch[OPTS_START + 9].operator = 1;
	synth->dispatch[OPTS_START + 10].operator = 2;
	synth->dispatch[OPTS_START + 11].operator = 3;

	return(0);
}

/*
 * This will be called to make any routine specific parameters available.
 */
static int
rhodesConfigure(brightonWindow *win)
{
	guiSynth *synth = findSynth(global.synths, win);
	brightonEvent event;

	if (synth == 0)
	{
		printf("problems going operational\n");
		return(-1);
	}

	if (synth->flags & OPERATIONAL)
		return(0);

printf("going operational\n");

	synth->flags |= OPERATIONAL;
	synth->keypanel = 3;
	synth->keypanel2 = -1;
	synth->transpose = 36;
/*	loadMemory(synth, "rhodes", 0, synth->location, synth->mem.active, */
/*		FIRST_DEV, BRISTOL_NOCALLS|BRISTOL_FORCE); */

	rhodesProgramme(synth, global.controlfd, synth->sid,
		synth->dispatch[OPTS_START].controller,
		synth->dispatch[OPTS_START].operator, 1.0f);

	displayPanelText(synth, "PRG", synth->location, MEM_PANEL, MEM_COUNT - 1);

	/*
	 * Hm. This is a hack for a few bits of bad rendering of a keyboard. Only
	 * occurs on first paint, so we suppress the first paint, and then request
	 * an expose here.
	 */
	event.type = BRIGHTON_EXPOSE;
	event.intvalue = 1;
	brightonParamChange(synth->win, KEY_PANEL, -1, &event);
	event.type = BRIGHTON_FLOAT;
	event.value = 1;
	brightonParamChange(synth->win, 0, 0, &event);
	configureGlobals(synth);

	event.type = BRIGHTON_FLOAT;
	event.value = 0.1;
	brightonParamChange(synth->win, OPTS_PANEL, 8, &event);
	brightonParamChange(synth->win, OPTS_PANEL, 9, &event);
	brightonParamChange(synth->win, OPTS_PANEL, 10, &event);
	event.value = 0.5;
	brightonParamChange(synth->win, OPTS_PANEL, 11, &event);

	brightonPut(synth->win, "bitmaps/blueprints/rhodesshade.xpm",
		0, 0, synth->win->width, synth->win->height);

	return(0);
}

