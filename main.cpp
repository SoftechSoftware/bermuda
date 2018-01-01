/*
 * Bermuda Syndrome engine rewrite
 * Copyright (C) 2007-2011 Gregory Montoir
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "game.h"
#include "systemstub.h"
#ifdef BERMUDA_VITA
#include <psp2/power.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
int _newlib_heap_size_user = 192 * 1024 * 1024;
#endif

static const char *USAGE =
	"Bermuda Syndrome\n"
	"Usage: bs [OPTIONS]...\n"
	"  --datapath=PATH   Path to data files (default 'DATA')\n"
	"  --savepath=PATH   Path to save files (default '.')\n"
	"  --musicpath=PATH  Path to music files (default 'MUSIC')\n";

static bool parseOption(const char *arg, const char *longCmd, const char **opt) {
	bool handled = false;
	if (arg[0] == '-' && arg[1] == '-') {
		if (strncmp(arg + 2, longCmd, strlen(longCmd)) == 0) {
			*opt = arg + 2 + strlen(longCmd);
			handled = true;
		}
	}
	return handled;
}

static Game *g_game;
static SystemStub *g_stub;

static void init(const char *dataPath, const char *savePath, const char *musicPath) {
	g_stub = SystemStub_SDL_create();
	g_game = new Game(g_stub, dataPath, savePath, musicPath);
	g_game->init();
}

static void fini() {
	g_game->fini();
	delete g_game;
	g_stub->destroy();
	delete g_stub;
	g_stub = 0;
}

#ifdef __EMSCRIPTEN__
static void mainLoop() {
	if (!g_stub->_quit) {
		g_game->mainLoop();
	}
}
#endif

#undef main
int main(int argc, char *argv[]) {
#ifdef BERMUDA_VITA
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	scePowerSetArmClockFrequency(444);
	// Initialze File System Factory
	sceIoMkdir("ux0:data", 0755);
	sceIoMkdir("ux0:data/bermuda", 0755);
	sceIoMkdir("ux0:data/bermuda/DATA", 0755);
	sceIoMkdir("ux0:data/bermuda/SAVE", 0755);
	sceIoMkdir("ux0:data/bermuda/MUSIC", 0755);

	const char *dataPath = "ux0:data/bermuda/DATA";
	const char *savePath = "ux0:data/bermuda/SAVE";
	const char *musicPath = "ux0:data/bermuda/MUSIC";
#else
	const char *dataPath = "DATA";
	const char *savePath = ".";
	const char *musicPath = "MUSIC";
#endif
	for (int i = 1; i < argc; ++i) {
		bool opt = false;
		if (strlen(argv[i]) >= 2) {
			opt |= parseOption(argv[i], "datapath=", &dataPath);
			opt |= parseOption(argv[i], "savepath=", &savePath);
			opt |= parseOption(argv[i], "musicpath=", &musicPath);
		}
		if (!opt) {
			printf("%s", USAGE);
			return 0;
		}
	}
	g_debugMask = DBG_INFO; // | DBG_GAME | DBG_OPCODES | DBG_DIALOGUE;
	init(dataPath, savePath, musicPath);
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(mainLoop, kCycleDelay, 0);
#elif BERMUDA_VITA
	while (!g_stub->_quit) {
		g_game->mainLoop();
		g_stub->processEvents();
	}
	fini();
#else
	uint32_t lastFrameTimeStamp = g_stub->getTimeStamp();
	while (!g_stub->_quit) {
		g_game->mainLoop();
		const uint32_t end = lastFrameTimeStamp + kCycleDelay;
		do {
			g_stub->sleep(10);
			g_stub->processEvents();
		} while (!g_stub->_pi.fastMode && g_stub->getTimeStamp() < end);
		lastFrameTimeStamp = g_stub->getTimeStamp();
	}
	fini();
#endif
#ifdef BERMUDA_VITA
	sceKernelExitProcess(0);
#endif
	return 0;
}
