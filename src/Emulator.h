#pragma once

#include <SDL.h>
#include <string>

#include "ROM.h"
#include "MMU.h"
#include "CPU.h"
#include "GPU.h"

class Emulator {
	friend class Debugger;
private:
	SDL_Window* window;
	SDL_Renderer* renderer;

	uint64_t frameCycles;

	bool initSDL();
public:
	ROM rom;
	GPU gpu;
	MMU mmu;
	CPU cpu;

	bool running;

	explicit Emulator(const std::string& romfile, const bool graphics = true);
	~Emulator();

	void Run();
	void Step();
	void CheckUpdate();
	void Update();
};
