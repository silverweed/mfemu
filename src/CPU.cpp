#include "CPU.h"

// Gameboy bootstrap ROM
const uint8_t bootstrap[] = {
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21,
	0x26, 0xFF, 0x0E, 0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32,
	0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0, 0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80,
	0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B, 0xFE, 0x34, 0x20, 0xF3,
	0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9, 0x3E,
	0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32,
	0x0D, 0x20, 0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42,
	0x3E, 0x91, 0xE0, 0x40, 0x04, 0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90,
	0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2, 0x0E, 0x13, 0x24, 0x7C, 0x1E,
	0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06, 0x7B, 0xE2,
	0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05,
	0x20, 0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17,
	0xC1, 0xCB, 0x11, 0x17, 0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE,
	0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C,
	0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E,
	0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9,
	0xA5, 0x42, 0x3C, 0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20,
	0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20, 0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05,
	0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

uint8_t CPU::Read(uint16_t location) {
	// 0000 - 0100 => Bootstrap ROM (only if turned on)
	if (usingBootstrap && location < 0x0100) {
		return bootstrap[location];
	}


	// 0100 - 3fff => Fixed bank from cartridge
	if (location < 0x4000) {
		return rom->fixed.bytes[location];
	}

	// 4000 - 7fff => Switchable bank from cartridge
	if (location < 0x8000) {
		return rom->banks[rom->romBankId].bytes[location - 0x4000];
	}

	// 8000 - 9fff => VRAM bank (switchable in GBC)
	if (location < 0xa000) {
		return VRAM[VRAMbankId].bytes[location - 0x8000];
	}

	// a000 - bfff => External RAM (switchable)
	if (location < 0xc000) {
		return rom->ram[rom->ramBankId].bytes[location - 0xa000];
	}

	// c000 - cfff => Work RAM fixed bank
	if (location < 0xd000) {
		return WRAM.bytes[location - 0xc000];
	}

	// d000 - dfff => Switchable Work RAM bank
	if (location < 0xe000) {
		return WRAMbanks[WRAMbankId].bytes[location - 0xd000];
	}

	// e000 - fdff => Mirror of c000 - ddff
	if (location < 0xfe00) {
		return Read(location - 0x2000);
	}

	// fe00 - fe9f => Sprite attribute table
	if (location < 0xfea0) {
		//TODO
		return 0;
	}

	// fea0 - feff => Not usable
	if (location < 0xff00) {
		return 0;
	}

	// ff00 - ff7f => I/O Registers
	if (location < 0xff80) {
		//TODO
		return 0;
	}

	// ff80 - fffe => High RAM (HRAM)
	if (location < 0xffff) {
		return ZRAM.bytes[location - 0xff80];
	}

	// ffff is IME, IME is not accessible
	return 0;
}

void CPU::Write(uint16_t location, uint8_t value) {
	// 0000 - 7fff => ROM (Not writable)
	if (location < 0x8000) { return; }

	// 8000 - 9fff => VRAM bank (switchable in GBC)
	if (location < 0xa000) {
		VRAM[VRAMbankId].bytes[location - 0x8000] = value;
		return;
	}

	// a000 - bfff => External RAM (switchable)
	if (location < 0xc000) {
		rom->ram[rom->ramBankId].bytes[location - 0xa000] = value;
		return;
	}

	// c000 - cfff => Work RAM fixed bank
	if (location < 0xd000) {
		WRAM.bytes[location - 0xc000] = value;
		return;
	}

	// d000 - dfff => Switchable Work RAM bank
	if (location < 0xe000) {
		WRAMbanks[WRAMbankId].bytes[location - 0xd000] = value;
		return;
	}

	// e000 - fdff => Mirror of c000 - ddff (Not writable)
	if (location < 0xfe00) { return; }

	// fe00 - fe9f => Sprite attribute table
	if (location < 0xfea0) {
		//TODO
		return;
	}

	// fea0 - feff => Not usable
	if (location < 0xff00) { return; }

	// ff00 - ff7f => I/O Registers
	if (location < 0xff80) {
		//TODO
		return;
	}

	// ff80 - fffe => High RAM (HRAM)
	if (location < 0xffff) {
		ZRAM.bytes[location - 0xff80] = value;
		return;
	}
}

void CPU::Step() {
	if (usingBootstrap && PC >= 0x0100) {
		usingBootstrap = false;
	}

	uint8_t opcode = Read(PC);
	Execute(opcode);
	PC += 1;
}

CPU::CPU(ROM* _rom)
	: cycles({0,0}) {
	// Setup variables
	rom = _rom;
	running = true;
	PC = 0;
	maskable = true;
	usingBootstrap = true;

	// Push at least one ram bank (GB classic)
	VRAMBank bank1;
	VRAM.push_back(bank1);
}

CPU::~CPU() {}
