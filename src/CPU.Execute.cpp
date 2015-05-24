#include "CPU.h"
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>

typedef std::function<void(CPU* cpu)> CPUHandler;

enum RID {
	A, B, C, D, E, H, L
};
enum PID {
	AF, BC, DE, HL, SP, PC
};
enum JumpCondition {
	NO, NZ, ZE, NC, CA
};
enum RotationType {
	ThC, Shf, Rep, Rot
};
enum DebugRegisterType {
	Direct, Indirect
};
enum DebugFlags {
	Comma, IndStart, IndFinish
};


uint8_t* getRegister(CPU* cpu, const RID id) {
	switch (id) {
	case A: return &(cpu->AF.Single.A);
	case B: return &(cpu->BC.Single.B);
	case C: return &(cpu->BC.Single.C);
	case D: return &(cpu->DE.Single.D);
	case E: return &(cpu->DE.Single.E);
	case H: return &(cpu->HL.Single.H);
	case L: return &(cpu->HL.Single.L);
	}
	return nullptr;
}

uint16_t* getPair(CPU* cpu, const PID id) {
	switch (id) {
	case AF: return &(cpu->AF.Pair);
	case BC: return &(cpu->BC.Pair);
	case DE: return &(cpu->DE.Pair);
	case HL: return &(cpu->HL.Pair);
	case SP: return &(cpu->SP);
	case PC: return &(cpu->PC);
	}
	return nullptr;
}

bool shouldJump(CPU* cpu, const JumpCondition condition) {
	switch (condition) {
	case NO: return true;
	case NZ: return cpu->Flags().Zero == 0;
	case ZE: return cpu->Flags().Zero == 1;
	case NC: return cpu->Flags().Carry == 0;
	case CA: return cpu->Flags().Carry == 1;
	}
	return false;
}


#if DEBUG_OPS

std::string getRegisterName(const RID id) {
	switch (id) {
	case A: return "A";
	case B: return "B";
	case C: return "C";
	case D: return "D";
	case E: return "E";
	case H: return "H";
	case L: return "L";
	}
	return "<REG>";
}

std::string getPairName(const PID id) {
	switch (id) {
	case AF: return "AF";
	case BC: return "BC";
	case DE: return "DE";
	case HL: return "HL";
	case SP: return "SP";
	case PC: return "PC";
	}
	return "<PAIR>";
}

std::string getJumpConditionName(const JumpCondition condition) {
	switch (condition) {
	case CA: return " C";
	case NC: return " NC";
	case ZE: return " Z";
	case NZ: return " NZ";
	case NO: default: return "";
	}
}

void debugPrintArgument(CPU* cpu) {
	std::cout << std::endl;
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const DebugRegisterType type, const RID registerId, const Args... args) {
	std::string registerName = getRegisterName(registerId);
	if (type == Indirect) {
		registerName = "(" + registerName + ")";
	}
	std::cout << " " << registerName;
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const DebugRegisterType type, const PID pairId, const Args... args) {
	std::string pairName = getPairName(pairId);
	if (type == Indirect) {
		pairName = "(" + pairName + ")";
	}
	std::cout << " " << pairName;
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const DebugFlags flag, const Args... args) {
	switch (flag) {
	case Comma: std::cout << ","; break;
	case IndStart: std::cout << "("; break;
	case IndFinish: std::cout << " )"; break;
	}
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const JumpCondition condition, const Args... args) {
	std::cout << getJumpConditionName(condition);
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const uint8_t absolute, const Args... args) {
	std::cout << " $" << std::setfill('0') << std::setw(4) << std::hex << absolute;
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const uint16_t absolute, const Args... args) {
	std::cout << " $" << std::setfill('0') << std::setw(4) << std::hex << absolute;
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const int absolute, const Args... args) {
	std::cout << " " << absolute;
	debugPrintArgument(cpu, args...);
}

template<typename... Args>
void debugPrintArgument(CPU* cpu, const std::string absolute, const Args... args) {
	std::cout << " " << absolute;
	debugPrintArgument(cpu, args...);
}
#endif

template<typename... Args>
void debugPrintInstruction(CPU* cpu, const Args... args) {
#if DEBUG_OPS
	std::cout << std::setfill('0') << std::setw(4) << std::hex << cpu->PC << " |";
	debugPrintArgument(cpu, args...);
#endif
}

// Do nothing
void Nop(CPU* cpu) {
	cpu->cycles.add(1, 4);

	debugPrintInstruction(cpu, "NOP");
}

// Stop or halt the processor
CPUHandler Halt(const bool waitInterrupt) {
	//Todo handle waitInterrupt (for HALT)
	return [waitInterrupt](CPU *cpu) {
		// STOP takes two machine cycles
		int mcycles = waitInterrupt ? 1 : 2;

		cpu->cycles.add(mcycles, 4);
		cpu->running = false;

		debugPrintInstruction(cpu, waitInterrupt ? "HALT" : "STOP");
	};
}

// Direct Load (8bit Register to 8bit Register)
CPUHandler LoadDirect(const RID dst, const RID src) {
	return [src, dst](CPU* cpu) {
		uint8_t* srcRes = getRegister(cpu, src);
		uint8_t* dstRes = getRegister(cpu, dst);
		*dstRes = *srcRes;
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, Direct, src);
	};
}

// Direct Load (16bit Register to 16bit Register)
CPUHandler LoadDirect(const PID dst, const PID src) {
	return [src, dst](CPU* cpu) {
		uint16_t* srcRes = getPair(cpu, src);
		uint16_t* dstRes = getPair(cpu, dst);
		*dstRes = *srcRes;
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, Direct, src);
	};
}

// Indirect Load (16bit Register offset to Register)
CPUHandler LoadIndirect(const RID dst, const PID ind) {
	return [dst, ind](CPU* cpu) {
		uint8_t* res = getRegister(cpu, dst);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t value = cpu->Read(*addr);
		*res = value;
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, Indirect, ind);
	};
}

// Indirect Load (Register to 16bit Register offset)
CPUHandler LoadIndirect(const PID dst, const RID src) {
	return [dst, src](CPU* cpu) {
		uint8_t* value = getRegister(cpu, src);
		uint16_t* addr = getPair(cpu, dst);
		cpu->Write(*addr, *value);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "LD ", Indirect, dst, Comma, Direct, src);
	};
}

// Indirect Load (Register to 8bit Register offset)
CPUHandler LoadHighMem(const RID ind, const RID reg) {
	return [ind, reg](CPU* cpu) {
		uint8_t* value = getRegister(cpu, reg);
		uint8_t* offset = getRegister(cpu, ind);

		cpu->Write(0xff00 + *offset, *value);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "LD ", IndStart, (uint16_t) 0xff00, "+", Direct, ind, IndFinish, Comma, Direct, reg);
	};
}

// Indirect Load (8bit Register offset to Register)
CPUHandler LoadHighReg(const RID dst, const RID ind) {
	return [dst, ind](CPU* cpu) {
		uint8_t* reg = getRegister(cpu, dst);
		uint8_t* offset = getRegister(cpu, ind);

		uint8_t value = cpu->Read(0xff00 + *offset);
		*reg = value;
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, IndStart, (uint16_t) 0xff00, "+", Direct, ind, IndFinish);
	};
}

// Indirect Load (8bit constant offset to Register)
CPUHandler LoadHighReg(const RID dst) {
	return [dst](CPU* cpu) {
		uint8_t* reg = getRegister(cpu, dst);
		uint8_t addr = cpu->Read(++cpu->PC);
		uint8_t value = cpu->Read(0xff00 + addr);
		*reg = value;
		cpu->cycles.add(2, 12);

		debugPrintInstruction(cpu, "LDH", Direct, dst, Comma, IndStart, (uint16_t) 0xff00, "+", addr, IndFinish);
	};
}

// Indirect Load (8bit constant offset to Register)
CPUHandler LoadHighAbs(const RID src) {
	return [src](CPU* cpu) {
		uint8_t* reg = getRegister(cpu, src);
		uint8_t addr = cpu->Read(++cpu->PC);
		cpu->Write(0xff00 + addr, *reg);
		cpu->cycles.add(2, 12);

		debugPrintInstruction(cpu, "LDH", IndStart, (uint16_t) 0xff00, "+", Direct, addr, IndFinish, Comma, Direct, src);
	};
}

// Indirect Load with increment/decrement (register offset to register)
CPUHandler LoadIndirectInc(const RID dst, const PID ind, const bool increment) {
	return [dst, ind, increment](CPU* cpu) {
		uint8_t* res = getRegister(cpu, dst);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t value = cpu->Read(*addr);
		*res = value;
		if (increment) {
			*addr += 1;
		} else {
			*addr -= 1;
		}
		cpu->cycles.add(1, 8);

		std::string op = "LD";
		op += (increment ? "I" : "D");
		debugPrintInstruction(cpu, op, Direct, dst, Comma, Indirect, ind);
	};
}

// Indirect Load with increment/decrement (register to register offset)
CPUHandler LoadIndirectInc(const PID ind, const RID src, const bool increment) {
	return [ind, src, increment](CPU* cpu) {
		uint8_t* res = getRegister(cpu, src);
		uint16_t* addr = getPair(cpu, ind);
		cpu->Write(*addr, *res);
		if (increment) {
			*addr += 1;
		} else {
			*addr -= 1;
		}
		cpu->cycles.add(1, 8);

		std::string op = "LD";
		op += (increment ? "I" : "D");
		debugPrintInstruction(cpu, op, Indirect, ind, Comma, Direct, src);
	};
}

// Load to memory (register to 16bit constant)
CPUHandler LoadToMemory(const RID src) {
	return [src](CPU* cpu) {
		// Get next bytes
		uint8_t  low = cpu->Read(++cpu->PC);
		uint8_t  high = cpu->Read(++cpu->PC);
		uint16_t word = (high << 8) + low;
		uint8_t* reg = getRegister(cpu, src);

		cpu->Write(word, *reg);
		cpu->cycles.add(3, 16);

		debugPrintInstruction(cpu, "LD ", IndStart, word, IndFinish, Comma, Direct, src);
	};
}

CPUHandler LoadFromMemory(const RID dst) {
	return [dst](CPU* cpu) {
		// Get next bytes
		uint8_t  low = cpu->Read(++cpu->PC);
		uint8_t  high = cpu->Read(++cpu->PC);
		uint16_t addr = (high << 8) + low;
		uint8_t  val = cpu->Read(addr);
		uint8_t* reg = getRegister(cpu, dst);
		*reg = val;

		cpu->cycles.add(3, 16);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, IndStart, addr, IndFinish);
	};
}

// Immediate Load (8bit constant to Register)
CPUHandler LoadImmediate(const RID dst) {
	return [dst](CPU* cpu) {
		uint8_t* dstRes = getRegister(cpu, dst);
		// Get next byte
		uint8_t value = cpu->Read(++cpu->PC);

		// Assign to register
		*dstRes = value;
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, value);
	};
}

// Immediate Load (16bit constant to register pair)
CPUHandler LoadImmediate(const PID dst) {
	return [dst](CPU* cpu) {
		uint16_t* dstRes = getPair(cpu, dst);
		// Get next bytes
		uint8_t  low = cpu->Read(++cpu->PC);
		uint8_t  high = cpu->Read(++cpu->PC);
		uint16_t word = (high << 8) + low;

		*dstRes = word;
		cpu->cycles.add(3, 12);

		debugPrintInstruction(cpu, "LD ", Direct, dst, Comma, word);
	};
}

// Increment register (8bit, immediate)
CPUHandler Increment(const RID dst) {
	return [dst](CPU* cpu) {
		uint8_t* dstRes = getRegister(cpu, dst);
		*dstRes += 1;
		cpu->Flags().Zero = *dstRes == 0 ? 1 : 0;
		cpu->Flags().BCD_AddSub = 0;
		cpu->Flags().BCD_HalfCarry = (*dstRes & 0x0f) > 9 ? 1 : 0;
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "INC", Direct, dst);
	};
}

// Increment register (16bit, immediate)
CPUHandler Increment(const PID dst) {
	return [dst](CPU* cpu) {
		uint16_t* dstRes = getPair(cpu, dst);
		*dstRes += 1;
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "INC", Direct, dst);
	};
}

// Decrement register (8bit, immediate)
CPUHandler Decrement(const RID dst) {
	return [dst](CPU* cpu) {
		uint8_t* dstRes = getRegister(cpu, dst);
		*dstRes -= 1;
		cpu->Flags().Zero = *dstRes == 0 ? 1 : 0;
		cpu->Flags().BCD_AddSub = 1;
		cpu->Flags().BCD_HalfCarry = (*dstRes & 0x0f) > 9 ? 1 : 0;
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "DEC", Direct, dst);
	};
}

// Decrement register (16bit, immediate)
CPUHandler Decrement(const PID dst) {
	return [dst](CPU* cpu) {
		uint16_t* dstRes = getPair(cpu, dst);
		*dstRes -= 1;
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "DEC", Direct, dst);
	};
}

// Add function (called by AddDirect etc)
void Add(CPU* cpu, uint8_t* a, uint8_t* b, const bool useCarry) {
	uint8_t orig = *a;
	*a += *b;
	if (useCarry && cpu->Flags().Carry) {
		*a += 1;
	}
	cpu->Flags().Carry = *a < orig ? 1 : 0;
	cpu->Flags().Zero = *a == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = (*a & 0x0f) > 9 ? 1 : 0;
}

void Add(CPU* cpu, uint16_t* a, uint16_t* b) {
	uint16_t orig = *a;
	*a += *b;
	cpu->Flags().Carry = *a < orig ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = (*a & 0x000f) > 9 ? 1 : 0;
}

// Direct Add (8bit, register to register)
CPUHandler AddDirect(const RID a, const RID b, const bool useCarry) {
	return [a, b, useCarry](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		Add(cpu, aRes, bRes, useCarry);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, (useCarry ? "ADC" : "ADD"), Direct, a, Comma, Direct, b);
	};
}

// Direct Add (16bit, register to register)
CPUHandler AddDirect(const PID a, const PID b) {
	return[a, b](CPU* cpu) {
		uint16_t* aRes = getPair(cpu, a);
		uint16_t* bRes = getPair(cpu, b);
		Add(cpu, aRes, bRes);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "ADD", Direct, a, Comma, Direct, b);
	};
}

// Indirect Add (register offset to register)
CPUHandler AddIndirect(const RID a, const PID ind, const bool useCarry) {
	return[a, ind, useCarry](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		Add(cpu, aRes, &bRes, useCarry);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, (useCarry ? "ADC" : "ADD"), Direct, a, Comma, Indirect, ind);
	};
}

// Add Immediate (8bit constant value to 8bit register)
CPUHandler AddImmediate(const RID a, const bool useCarry) {
	return[a, useCarry](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		Add(cpu, aRes, &bRes, useCarry);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, (useCarry ? "ADC" : "ADD"), Direct, a, Comma, bRes);
	};
}

// Add Immediate (8bit constant signed value to 16bit register)
CPUHandler AddImmediateS(const PID a) {
	return[a](CPU* cpu) {
		uint16_t* aRes = getPair(cpu, a);
		int8_t bRes = (int8_t) cpu->Read(++cpu->PC);
		uint16_t orig = *aRes;
		*aRes += bRes;
		cpu->Flags().Zero = 0;
		cpu->Flags().Carry = *aRes < orig;
		cpu->Flags().BCD_AddSub = 0;
		cpu->Flags().BCD_HalfCarry = (*aRes & 0x000f) > 9 ? 1 : 0;
		cpu->cycles.add(2, 16);

		debugPrintInstruction(cpu, "ADDs", Direct, a, Comma, bRes);
	};
}

// Subtract function (called by SubDirect etc)
void Subtract(CPU* cpu, uint8_t* a, uint8_t* b, const bool useCarry) {
	uint8_t orig = *a;
	*a -= *b;
	if (useCarry && cpu->Flags().Carry) {
		*a -= 1;
	}
	cpu->Flags().Carry = *a > orig;
	cpu->Flags().Zero = *a == 0 ? 0 : 1;
	cpu->Flags().BCD_AddSub = 1;
	cpu->Flags().BCD_HalfCarry = (*a & 0x0f) > 9 ? 1 : 0;
}

// Direct Subtract (8bit, register to register)
CPUHandler SubDirect(const RID a, const RID b, const bool useCarry) {
	return [a, b, useCarry](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		Subtract(cpu, aRes, bRes, useCarry);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, (useCarry ? "SBC" : "SUB"), Direct, a, Comma, Direct, b);
	};
}

// Indirect Subtract (16bit register offset to register)
CPUHandler SubIndirect(const RID a, const PID ind, const bool useCarry) {
	return[a, ind, useCarry](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		Subtract(cpu, aRes, &bRes, useCarry);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, (useCarry ? "SBC" : "SUB"), Direct, a, Comma, Indirect, ind);
	};
}

// Subtract Immediate (8bit constant value to 8bit register)
CPUHandler SubImmediate(const RID a, const bool useCarry) {
	return[a](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		Subtract(cpu, aRes, &bRes, false);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "SUB", Direct, a, Comma, bRes);
	};
}

// Compare function (used by CmpImmediate, CmpDirect etc)
void Compare(CPU* cpu, uint8_t* a, uint8_t* b) {
	uint8_t c = *a - *b;
	cpu->Flags().Carry = c > *a;
	cpu->Flags().Zero = c == 0 ? 0 : 1;
	cpu->Flags().BCD_AddSub = 1;
	cpu->Flags().BCD_HalfCarry = (c & 0x0f) > 9 ? 1 : 0;
}

CPUHandler CmpDirect(const RID a, const RID b) {
	return [a, b](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		Compare(cpu, aRes, bRes);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "CP ", Direct, a, Comma, Direct, b);
	};
}

CPUHandler CmpIndirect(const RID a, const PID ind) {
	return [a, ind](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		Compare(cpu, aRes, &bRes);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "CP ", Direct, a, Comma, Indirect, ind);
	};
}

CPUHandler CmpImmediate(const RID a) {
	return [a](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		Compare(cpu, aRes, &bRes);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "CP ", Direct, a, Comma, bRes);
	};
}

// And function (called by AndDirect etc)
void And(CPU* cpu, uint8_t* a, uint8_t* b) {
	*a &= *b;
	cpu->Flags().Zero = *a == 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 1;
	cpu->Flags().Carry = 0;
}

// Or function (called by OrDirect etc)
void Or(CPU* cpu, uint8_t* a, uint8_t* b) {
	*a |= *b;
	cpu->Flags().Zero = *a == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 0;
	cpu->Flags().Carry = 0;
}

// Or function (called by OrDirect etc)
void Xor(CPU* cpu, uint8_t* a, uint8_t* b) {
	*a ^= *b;
	cpu->Flags().Zero = *a == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 0;
	cpu->Flags().Carry = 0;
}

// Direct AND (register to register)
CPUHandler AndDirect(const RID a, const RID b) {
	return [a, b](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		And(cpu, aRes, bRes);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "AND", Direct, a, Comma, Direct, b);
	};
}

// Indirect AND (register offset to register)
CPUHandler AndIndirect(const RID a, const PID ind) {
	return[a, ind](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		And(cpu, aRes, &bRes);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "AND", Direct, a, Comma, Indirect, ind);
	};
}

// Immediate AND (8bit constant to register)
CPUHandler AndImmediate(const RID a) {
	return[a](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		And(cpu, aRes, &bRes);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "AND", Direct, a, Comma, bRes);
	};
}

// Direct OR (register to register)
CPUHandler OrDirect(const RID a, const RID b) {
	return [a, b](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		Or(cpu, aRes, bRes);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "OR ", Direct, a, Comma, Direct, b);
	};
}

// Indirect OR (register offset to register)
CPUHandler OrIndirect(const RID a, const PID ind) {
	return[a, ind](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		Or(cpu, aRes, &bRes);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "OR ", Direct, a, Comma, Indirect, ind);
	};
}

// Immediate OR (8bit constant to register)
CPUHandler OrImmediate(const RID a) {
	return[a](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		Or(cpu, aRes, &bRes);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "OR ", Direct, a, Comma, bRes);
	};
}

// Direct XOR (register to register)
CPUHandler XorDirect(const RID a, const RID b) {
	return [a, b](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t* bRes = getRegister(cpu, b);
		Xor(cpu, aRes, bRes);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "XOR", Direct, a, Comma, Direct, b);
	};
}

// Indirect XOR (register offset to register)
CPUHandler XorIndirect(const RID a, const PID ind) {
	return[a, ind](CPU* cpu) {
		uint8_t*  aRes = getRegister(cpu, a);
		uint16_t* addr = getPair(cpu, ind);
		uint8_t   bRes = cpu->Read(*addr);
		Xor(cpu, aRes, &bRes);
		cpu->cycles.add(1, 8);

		debugPrintInstruction(cpu, "XOR", Direct, a, Comma, Indirect, ind);
	};
}

// Immediate XOR (8bit constant to register)
CPUHandler XorImmediate(const RID a) {
	return[a](CPU* cpu) {
		uint8_t* aRes = getRegister(cpu, a);
		uint8_t  bRes = cpu->Read(++cpu->PC);
		Xor(cpu, aRes, &bRes);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "XOR", Direct, a, Comma, bRes);
	};
}

// Relative jump (8bit constant)
CPUHandler JumpRelative(const JumpCondition condition) {
	return [condition](CPU* cpu) {
		uint8_t u8 = cpu->Read(++cpu->PC);
		int r8 = (int8_t) u8;
		if (shouldJump(cpu, condition)) {
			cpu->PC += r8;
			cpu->cycles.add(2, 12);
		} else {
			cpu->cycles.add(2, 8);
		}

		debugPrintInstruction(cpu, "JR ", condition, Comma, (int) r8);
	};
}

// Immediate Absolute jump (16bit constant)
CPUHandler JumpAbsolute(const JumpCondition condition) {
	return [condition](CPU* cpu) {
		// Get next bytes
		uint8_t  low = cpu->Read(++cpu->PC);
		uint8_t  high = cpu->Read(++cpu->PC);
		uint16_t word = (high << 8) + low;

		if (shouldJump(cpu, condition)) {
			cpu->PC = word;
			cpu->cycles.add(3, 16);
		} else {
			cpu->cycles.add(3, 12);
		}

		debugPrintInstruction(cpu, "JP ", condition, Comma, word);
	};
}

// Immediate Absolute Jump (register)
CPUHandler JumpAbsolute(const PID src) {
	return [src](CPU* cpu) {
		cpu->PC = *getPair(cpu, src);
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, "JP ", ZE, Comma, Indirect, src);
	};
}

// Rotate Left function (called by RLCA/RLA etc)
void RotateLeft(CPU* cpu, uint8_t* val, const RotationType type) {
	uint8_t car = cpu->Flags().Carry; // Save Carry for ThC (Through Carry)
	uint8_t shf = *val >> 7;
	cpu->Flags().Carry = shf;
	*val = *val << 1;

	switch (type) {
	case ThC: *val |= car; break;
	case Rot: *val |= shf; break;
	}

	cpu->Flags().Zero = *val == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 0;
}

// Rotate Right function (called by RRCA/RRA etc)
void RotateRight(CPU* cpu, uint8_t* val, const RotationType type) {
	uint8_t car = cpu->Flags().Carry; // Save Carry for ThC (Through Carry)
	uint8_t old = *val;               // Save old value for Rep (Repeat)
	uint8_t shf = *val << 7;
	cpu->Flags().Carry = shf >> 7;
	*val = *val >> 1;

	switch (type) {
	case ThC: *val |= (car << 7); break;
	case Rot: *val |= shf;        break;
	case Rep: *val |= (old << 7); break;
	}

	cpu->Flags().Zero = *val == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 0;
}

// Rotate Accumulator
CPUHandler RotateAcc(const bool left, const bool throughCarry) {
	return [left, throughCarry](CPU* cpu) {
		uint8_t* acc = getRegister(cpu, A);
		if (left) {
			RotateLeft(cpu, acc, throughCarry ? ThC : Rot);
		} else {
			RotateRight(cpu, acc, throughCarry ? ThC : Rot);
		}
		cpu->Flags().Zero = 0;
		cpu->cycles.add(1, 4);

		std::string op = "R";
		if (left) op += "L";
		else op += "R";
		if (!throughCarry) op += "C";
		op += "A";
		debugPrintInstruction(cpu, op);
	};
}

// Rotate Register
CPUHandler RotateReg(const RID reg, const bool left, const RotationType type) {
	return [reg, left, type](CPU* cpu) {
		uint8_t* val = getRegister(cpu, reg);
		if (left) {
			RotateLeft(cpu, val, type);
		} else {
			RotateRight(cpu, val, type);
		}
		cpu->cycles.add(2, 8);

		std::string op;
		switch (type) {
		case Shf: op = (left ? "SLA" : "SRL"); break;
		case Rot: op = (left ? "RLC" : "RRC"); break;
		case ThC: op = (left ? "RL" : "RR"); break;
		case Rep: op = "SRA"; break;
		}
		debugPrintInstruction(cpu, op, Direct, reg);
	};
}

// Rotate Indirect
CPUHandler RotateInd(const PID ind, const bool left, const RotationType type) {
	return [ind, left, type](CPU* cpu) {
		uint16_t* addr = getPair(cpu, ind);
		uint8_t value = cpu->Read(*addr);
		if (left) {
			RotateLeft(cpu, &value, type);
		} else {
			RotateRight(cpu, &value, type);
		}
		cpu->Write(*addr, value);
		cpu->cycles.add(2, 16);

		std::string op;
		switch (type) {
		case Shf: op = (left ? "SLA" : "SRL"); break;
		case Rot: op = (left ? "RLC" : "RRC"); break;
		case ThC: op = (left ? "RL" : "RR"); break;
		case Rep: op = "SRA"; break;
		}
		debugPrintInstruction(cpu, op, Indirect, ind);
	};
}

// Swap function (swaps nibbles)
void Swap(CPU* cpu, uint8_t* value) {
	*value = (*value >> 4) | (*value << 4);
	cpu->Flags().Zero = *value == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 0;
	cpu->Flags().Carry = 0;
}

// Direct Swap (register)
CPUHandler SwapDirect(const RID reg) {
	return [reg](CPU* cpu) {
		uint8_t* val = getRegister(cpu, reg);
		Swap(cpu, val);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "SWAP", Direct, reg);
	};
}

// Indirect Swap (register offset)
CPUHandler SwapIndirect(const PID reg) {
	return [reg](CPU* cpu) {
		uint16_t* addr = getPair(cpu, reg);
		uint8_t value = cpu->Read(*addr);
		Swap(cpu, &value);
		cpu->Write(*addr, value);
		cpu->cycles.add(2, 16);

		debugPrintInstruction(cpu, "SWAP", Indirect, reg);
	};
}

// Set bit function (called by Set/Res etc)
void Set(uint8_t* value, const uint8_t offset, const bool reset) {
	if (reset) {
		*value &= ~(1 << offset);
	} else {
		*value |= 1 << offset;
	}
}

// Direct Set/Reset (register)
CPUHandler SetDirect(const RID reg, const uint8_t bit, const bool reset) {
	return [reg, bit, reset](CPU* cpu) {
		uint8_t* val = getRegister(cpu, reg);
		Set(val, bit, reset);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, (reset ? "RST" : "SET"), (int) bit, Comma, Direct, reg);
	};
}

// Indirect Set/Reset (register offset)
CPUHandler SetIndirect(const PID ind, const uint8_t bit, const bool reset) {
	return [ind, bit, reset](CPU* cpu) {
		uint16_t* addr = getPair(cpu, ind);
		uint8_t value = cpu->Read(*addr);
		Set(&value, bit, reset);
		cpu->Write(*addr, value);
		cpu->cycles.add(2, 16);

		debugPrintInstruction(cpu, (reset ? "RST" : "SET"), (int) bit, Comma, Indirect, ind);
	};
}

// Get bit operation (called by BitDirect/Indirect)
void Bit(CPU* cpu, const uint8_t value, const uint8_t offset) {
	uint8_t bit = (value >> offset) & 0x01;
	cpu->Flags().Zero = bit == 0 ? 1 : 0;
	cpu->Flags().BCD_AddSub = 0;
	cpu->Flags().BCD_HalfCarry = 1;
}

// Direct Get bit (register)
CPUHandler BitDirect(const RID reg, const uint8_t bit) {
	return [reg, bit](CPU* cpu) {
		uint8_t* value = getRegister(cpu, reg);
		Bit(cpu, *value, bit);
		cpu->cycles.add(2, 8);

		debugPrintInstruction(cpu, "BIT", (int) bit, Comma, Direct, reg);
	};
}

// Indirect Get bit (register offset)
CPUHandler BitIndirect(const PID ind, const uint8_t bit) {
	return [ind, bit](CPU* cpu) {
		uint16_t* addr = getPair(cpu, ind);
		uint8_t value = cpu->Read(*addr);
		Bit(cpu, value, bit);
		cpu->cycles.add(2, 16);

		debugPrintInstruction(cpu, "BIT", (int) bit, Comma, Indirect, ind);
	};
}

// Enable/Disable Maskable Interrupts
CPUHandler SetInt(bool enable) {
	return [enable](CPU* cpu) {
		cpu->maskable = enable;
		cpu->cycles.add(1, 4);

		debugPrintInstruction(cpu, enable ? "EI" : "DI");
	};
}

// Unimplemented instruction
void Todo(CPU* cpu) {
	std::cout << "Unknown Opcode: " << std::setfill('0') << std::setw(2) << std::hex << (int) cpu->Read(cpu->PC) << std::endl;
}

// Inexistent instruction
void Wrong(CPU* cpu) {
	std::cout << "Called inexistent opcode: " << std::setfill('0') << std::setw(2) << std::hex << (int) cpu->Read(cpu->PC) << std::endl;
}

const static CPUHandler cbhandlers[] = {
	RotateReg(B, true, Rot),   // 00 RLC B
	RotateReg(C, true, Rot),   // 01 RLC C
	RotateReg(D, true, Rot),   // 02 RLC D
	RotateReg(E, true, Rot),   // 03 RLC E
	RotateReg(H, true, Rot),   // 04 RLC H
	RotateReg(L, true, Rot),   // 05 RLC L
	RotateInd(HL, true, Rot),  // 06 RLC (HL)
	RotateReg(A, true, Rot),   // 07 RLC A
	RotateReg(B, false, Rot),  // 08 RRC B
	RotateReg(C, false, Rot),  // 09 RRC C
	RotateReg(D, false, Rot),  // 0a RRC D
	RotateReg(E, false, Rot),  // 0b RRC E
	RotateReg(H, false, Rot),  // 0c RRC H
	RotateReg(L, false, Rot),  // 0d RRC L
	RotateInd(HL, false, Rot), // 0e RRC (HL)
	RotateReg(A, false, Rot),  // 0f RRC A
	RotateReg(B, true, ThC),   // 10 RL  B
	RotateReg(C, true, ThC),   // 11 RL  C
	RotateReg(D, true, ThC),   // 12 RL  D
	RotateReg(E, true, ThC),   // 13 RL  E
	RotateReg(H, true, ThC),   // 14 RL  H
	RotateReg(L, true, ThC),   // 15 RL  L
	RotateInd(HL, true, ThC),  // 16 RL  (HL)
	RotateReg(A, true, ThC),   // 17 RL  A
	RotateReg(B, false, ThC),  // 18 RR  B
	RotateReg(C, false, ThC),  // 19 RR  C
	RotateReg(D, false, ThC),  // 1a RR  D
	RotateReg(E, false, ThC),  // 1b RR  E
	RotateReg(H, false, ThC),  // 1c RR  H
	RotateReg(L, false, ThC),  // 1d RR  L
	RotateInd(HL, false, ThC), // 1e RR  (HL)
	RotateReg(A, false, ThC),  // 1f RR  A
	RotateReg(B, true, Shf),   // 20 SLA B
	RotateReg(C, true, Shf),   // 21 SLA C
	RotateReg(D, true, Shf),   // 22 SLA D
	RotateReg(E, true, Shf),   // 23 SLA E
	RotateReg(H, true, Shf),   // 24 SLA H
	RotateReg(L, true, Shf),   // 25 SLA L
	RotateInd(HL, true, Shf),  // 26 SLA (HL)
	RotateReg(A, true, Shf),   // 27 SLA A
	RotateReg(B, false, Rep),  // 28 SRA B
	RotateReg(C, false, Rep),  // 29 SRA C
	RotateReg(D, false, Rep),  // 2a SRA D
	RotateReg(E, false, Rep),  // 2b SRA E
	RotateReg(H, false, Rep),  // 2c SRA H
	RotateReg(L, false, Rep),  // 2d SRA L
	RotateInd(HL, false, Rep), // 2e SRA (HL)
	RotateReg(A, false, Rep),  // 2f SRA A
	SwapDirect(B),             // 30 SWAP B
	SwapDirect(C),             // 31 SWAP C
	SwapDirect(D),             // 32 SWAP D
	SwapDirect(E),             // 33 SWAP E
	SwapDirect(H),             // 34 SWAP H
	SwapDirect(L),             // 35 SWAP L
	SwapIndirect(HL),          // 36 SWAP (HL)
	SwapDirect(A),             // 37 SWAP A
	RotateReg(B, false, Shf),  // 38 SRL B
	RotateReg(C, false, Shf),  // 39 SRL C
	RotateReg(D, false, Shf),  // 3a SRL D
	RotateReg(E, false, Shf),  // 3b SRL E
	RotateReg(H, false, Shf),  // 3c SRL H
	RotateReg(L, false, Shf),  // 3d SRL L
	RotateInd(HL, false, Shf), // 3e SRL (HL)
	RotateReg(A, false, Shf),  // 3f SRL A
	BitDirect(B, 0),           // 40 BIT 0,B
	BitDirect(C, 0),           // 41 BIT 0,C
	BitDirect(D, 0),           // 42 BIT 0,D
	BitDirect(E, 0),           // 43 BIT 0,E
	BitDirect(H, 0),           // 44 BIT 0,H
	BitDirect(L, 0),           // 45 BIT 0,L
	BitIndirect(HL, 0),        // 46 BIT 0,(HL)
	BitDirect(A, 0),           // 47 BIT 0,A
	BitDirect(B, 1),           // 48 BIT 1,B
	BitDirect(C, 1),           // 49 BIT 1,C
	BitDirect(D, 1),           // 4a BIT 1,D
	BitDirect(E, 1),           // 4b BIT 1,E
	BitDirect(H, 1),           // 4c BIT 1,H
	BitDirect(L, 1),           // 4d BIT 1,L
	BitIndirect(HL, 1),        // 4e BIT 1,(HL)
	BitDirect(A, 1),           // 4f BIT 1,A
	BitDirect(B, 2),           // 50 BIT 2,B
	BitDirect(C, 2),           // 51 BIT 2,C
	BitDirect(D, 2),           // 52 BIT 2,D
	BitDirect(E, 2),           // 53 BIT 2,E
	BitDirect(H, 2),           // 54 BIT 2,H
	BitDirect(L, 2),           // 55 BIT 2,L
	BitIndirect(HL, 2),        // 56 BIT 2,(HL)
	BitDirect(A, 2),           // 57 BIT 2,A
	BitDirect(B, 3),           // 58 BIT 3,B
	BitDirect(C, 3),           // 59 BIT 3,C
	BitDirect(D, 3),           // 5a BIT 3,D
	BitDirect(E, 3),           // 5b BIT 3,E
	BitDirect(H, 3),           // 5c BIT 3,H
	BitDirect(L, 3),           // 5d BIT 3,L
	BitIndirect(HL, 3),        // 5e BIT 3,(HL)
	BitDirect(A, 3),           // 5f BIT 3,A
	BitDirect(B, 4),           // 60 BIT 4,B
	BitDirect(C, 4),           // 61 BIT 4,C
	BitDirect(D, 4),           // 62 BIT 4,D
	BitDirect(E, 4),           // 63 BIT 4,E
	BitDirect(H, 4),           // 64 BIT 4,H
	BitDirect(L, 4),           // 65 BIT 4,L
	BitIndirect(HL, 4),        // 66 BIT 4,(HL)
	BitDirect(A, 4),           // 67 BIT 4,A
	BitDirect(B, 5),           // 68 BIT 5,B
	BitDirect(C, 5),           // 69 BIT 5,C
	BitDirect(D, 5),           // 6a BIT 5,D
	BitDirect(E, 5),           // 6b BIT 5,E
	BitDirect(H, 5),           // 6c BIT 5,H
	BitDirect(L, 5),           // 6d BIT 5,L
	BitIndirect(HL, 5),        // 6e BIT 5,(HL)
	BitDirect(A, 5),           // 6f BIT 5,A
	BitDirect(B, 6),           // 70 BIT 6,B
	BitDirect(C, 6),           // 71 BIT 6,C
	BitDirect(D, 6),           // 72 BIT 6,D
	BitDirect(E, 6),           // 73 BIT 6,E
	BitDirect(H, 6),           // 74 BIT 6,H
	BitDirect(L, 6),           // 75 BIT 6,L
	BitIndirect(HL, 6),        // 76 BIT 6,(HL)
	BitDirect(A, 6),           // 77 BIT 6,A
	BitDirect(B, 7),           // 78 BIT 7,B
	BitDirect(C, 7),           // 79 BIT 7,C
	BitDirect(D, 7),           // 7a BIT 7,D
	BitDirect(E, 7),           // 7b BIT 7,E
	BitDirect(H, 7),           // 7c BIT 7,H
	BitDirect(L, 7),           // 7d BIT 7,L
	BitIndirect(HL, 7),        // 7e BIT 7,(HL)
	BitDirect(A, 7),           // 7f BIT 7,A
	SetDirect(B, 0, true),     // 80 RES 0,B
	SetDirect(C, 0, true),     // 81 RES 0,C
	SetDirect(D, 0, true),     // 82 RES 0,D
	SetDirect(E, 0, true),     // 83 RES 0,E
	SetDirect(H, 0, true),     // 84 RES 0,H
	SetDirect(L, 0, true),     // 85 RES 0,L
	SetIndirect(HL, 0, true),  // 86 RES 0,(HL)
	SetDirect(A, 0, true),     // 87 RES 0,A
	SetDirect(B, 1, true),     // 88 RES 1,B
	SetDirect(C, 1, true),     // 89 RES 1,C
	SetDirect(D, 1, true),     // 8a RES 1,D
	SetDirect(E, 1, true),     // 8b RES 1,E
	SetDirect(H, 1, true),     // 8c RES 1,H
	SetDirect(L, 1, true),     // 8d RES 1,L
	SetIndirect(HL, 1, true),  // 8e RES 1,(HL)
	SetDirect(A, 1, true),     // 8f RES 1,A
	SetDirect(B, 2, true),     // 90 RES 2,B
	SetDirect(C, 2, true),     // 91 RES 2,C
	SetDirect(D, 2, true),     // 92 RES 2,D
	SetDirect(E, 2, true),     // 93 RES 2,E
	SetDirect(H, 2, true),     // 94 RES 2,H
	SetDirect(L, 2, true),     // 95 RES 2,L
	SetIndirect(HL, 2, true),  // 96 RES 2,(HL)
	SetDirect(A, 2, true),     // 97 RES 2,A
	SetDirect(B, 3, true),     // 98 RES 3,B
	SetDirect(C, 3, true),     // 99 RES 3,C
	SetDirect(D, 3, true),     // 9a RES 3,D
	SetDirect(E, 3, true),     // 9b RES 3,E
	SetDirect(H, 3, true),     // 9c RES 3,H
	SetDirect(L, 3, true),     // 9d RES 3,L
	SetIndirect(HL, 3, true),  // 9e RES 3,(HL)
	SetDirect(A, 3, true),     // 9f RES 3,A
	SetDirect(B, 4, true),     // a0 RES 4,B
	SetDirect(C, 4, true),     // a1 RES 4,C
	SetDirect(D, 4, true),     // a2 RES 4,D
	SetDirect(E, 4, true),     // a3 RES 4,E
	SetDirect(H, 4, true),     // a4 RES 4,H
	SetDirect(L, 4, true),     // a5 RES 4,L
	SetIndirect(HL, 4, true),  // a6 RES 4,(HL)
	SetDirect(A, 4, true),     // a7 RES 4,A
	SetDirect(B, 5, true),     // a8 RES 5,B
	SetDirect(C, 5, true),     // a9 RES 5,C
	SetDirect(D, 5, true),     // aa RES 5,D
	SetDirect(E, 5, true),     // ab RES 5,E
	SetDirect(H, 5, true),     // ac RES 5,H
	SetDirect(L, 5, true),     // ad RES 5,L
	SetIndirect(HL, 5, true),  // ae RES 5,(HL)
	SetDirect(A, 5, true),     // af RES 5,A
	SetDirect(B, 6, true),     // b0 RES 6,B
	SetDirect(C, 6, true),     // b1 RES 6,C
	SetDirect(D, 6, true),     // b2 RES 6,D
	SetDirect(E, 6, true),     // b3 RES 6,E
	SetDirect(H, 6, true),     // b4 RES 6,H
	SetDirect(L, 6, true),     // b5 RES 6,L
	SetIndirect(HL, 6, true),  // b6 RES 6,(HL)
	SetDirect(A, 6, true),     // b7 RES 6,A
	SetDirect(B, 7, true),     // b8 RES 7,B
	SetDirect(C, 7, true),     // b9 RES 7,C
	SetDirect(D, 7, true),     // ba RES 7,D
	SetDirect(E, 7, true),     // bb RES 7,E
	SetDirect(H, 7, true),     // bc RES 7,H
	SetDirect(L, 7, true),     // bd RES 7,L
	SetIndirect(HL, 7, true),  // be RES 7,(HL)
	SetDirect(A, 7, true),     // bf RES 7,A
	SetDirect(B, 0, false),    // c0 SET 0,B
	SetDirect(C, 0, false),    // c1 SET 0,C
	SetDirect(D, 0, false),    // c2 SET 0,D
	SetDirect(E, 0, false),    // c3 SET 0,E
	SetDirect(H, 0, false),    // c4 SET 0,H
	SetDirect(L, 0, false),    // c5 SET 0,L
	SetIndirect(HL, 0, false), // c6 SET 0,(HL)
	SetDirect(A, 0, false),    // c7 SET 0,A
	SetDirect(B, 1, false),    // c8 SET 1,B
	SetDirect(C, 1, false),    // c9 SET 1,C
	SetDirect(D, 1, false),    // ca SET 1,D
	SetDirect(E, 1, false),    // cb SET 1,E
	SetDirect(H, 1, false),    // cc SET 1,H
	SetDirect(L, 1, false),    // cd SET 1,L
	SetIndirect(HL, 1, false), // ce SET 1,(HL)
	SetDirect(A, 1, false),    // cf SET 1,A
	SetDirect(B, 2, false),    // d0 SET 2,B
	SetDirect(C, 2, false),    // d1 SET 2,C
	SetDirect(D, 2, false),    // d2 SET 2,D
	SetDirect(E, 2, false),    // d3 SET 2,E
	SetDirect(H, 2, false),    // d4 SET 2,H
	SetDirect(L, 2, false),    // d5 SET 2,L
	SetIndirect(HL, 2, false), // d6 SET 2,(HL)
	SetDirect(A, 2, false),    // d7 SET 2,A
	SetDirect(B, 3, false),    // d8 SET 3,B
	SetDirect(C, 3, false),    // d9 SET 3,C
	SetDirect(D, 3, false),    // da SET 3,D
	SetDirect(E, 3, false),    // db SET 3,E
	SetDirect(H, 3, false),    // dc SET 3,H
	SetDirect(L, 3, false),    // dd SET 3,L
	SetIndirect(HL, 3, false), // de SET 3,(HL)
	SetDirect(A, 3, false),    // df SET 3,A
	SetDirect(B, 4, false),    // e0 SET 4,B
	SetDirect(C, 4, false),    // e1 SET 4,C
	SetDirect(D, 4, false),    // e2 SET 4,D
	SetDirect(E, 4, false),    // e3 SET 4,E
	SetDirect(H, 4, false),    // e4 SET 4,H
	SetDirect(L, 4, false),    // e5 SET 4,L
	SetIndirect(HL, 4, false), // e6 SET 4,(HL)
	SetDirect(A, 4, false),    // e7 SET 4,A
	SetDirect(B, 5, false),    // e8 SET 5,B
	SetDirect(C, 5, false),    // e9 SET 5,C
	SetDirect(D, 5, false),    // ea SET 5,D
	SetDirect(E, 5, false),    // eb SET 5,E
	SetDirect(H, 5, false),    // ec SET 5,H
	SetDirect(L, 5, false),    // ed SET 5,L
	SetIndirect(HL, 5, false), // ee SET 5,(HL)
	SetDirect(A, 5, false),    // ef SET 5,A
	SetDirect(B, 6, false),    // f0 SET 6,B
	SetDirect(C, 6, false),    // f1 SET 6,C
	SetDirect(D, 6, false),    // f2 SET 6,D
	SetDirect(E, 6, false),    // f3 SET 6,E
	SetDirect(H, 6, false),    // f4 SET 6,H
	SetDirect(L, 6, false),    // f5 SET 6,L
	SetIndirect(HL, 6, false), // f6 SET 6,(HL)
	SetDirect(A, 6, false),    // f7 SET 6,A
	SetDirect(B, 7, false),    // f8 SET 7,B
	SetDirect(C, 7, false),    // f9 SET 7,C
	SetDirect(D, 7, false),    // fa SET 7,D
	SetDirect(E, 7, false),    // fb SET 7,E
	SetDirect(H, 7, false),    // fc SET 7,H
	SetDirect(L, 7, false),    // fd SET 7,L
	SetIndirect(HL, 7, false), // fe SET 7,(HL)
	SetDirect(A, 7, false)     // ff SET 7,A
};

void HandleCB(CPU* cpu) {
	uint8_t opcode = cpu->Read(++cpu->PC);
	cbhandlers[opcode](cpu);
	cpu->cycles.add(1, 4);
}

const static CPUHandler handlers[] = {
	Nop,                 // 00 NOP
	LoadImmediate(BC),   // 01 LD  BC,d16
	LoadIndirect(BC, A), // 02 LD  (BC),A
	Increment(BC),       // 03 INC BC
	Increment(B),        // 04 INC B
	Decrement(B),        // 05 DEC B
	LoadImmediate(B),    // 06 LD  B,d8
	RotateAcc(true, false), // 07 RLCA
	Todo, // 08
	AddDirect(HL, BC),   // 09 ADD HL,BC
	LoadIndirect(A, BC), // 0a LD  A,(BC)
	Decrement(BC),       // 0b DEC BC
	Increment(C),        // 0c INC C
	Decrement(C),        // 0d DEC C
	LoadImmediate(C),    // 0e LD  C,d8
	RotateAcc(false, false), // 0f RRCA
	Halt(false),         // 10 STOP
	LoadImmediate(DE),   // 11 LD  DE,d16
	LoadIndirect(DE, A), // 12 LD  (DE),A
	Increment(DE),       // 13 INC DE
	Increment(D),        // 14 INC D
	Decrement(D),        // 15 DEC D
	LoadImmediate(D),    // 16 LD  D,d8
	RotateAcc(true, true), // 17 RLA
	JumpRelative(NO),    // 18 JR  r8
	AddDirect(HL, DE),   // 19 ADD HL,DE
	LoadIndirect(A, DE), // 1a LD  A,(DE)
	Decrement(DE),       // 1b DEC DE
	Increment(E),        // 1c DEC E
	Decrement(E),        // 1d DEC E
	LoadImmediate(E),    // 1e LD  E,d8
	RotateAcc(false, true), // 1f RRA
	JumpRelative(NZ),    // 20 JR  NZ,r8
	LoadImmediate(HL),   // 21 LD  HL,d16
	LoadIndirectInc(HL, A, true), // 22 LDI (HL),A
	Increment(HL),       // 23 INC HL
	Increment(H),        // 24 INC H
	Decrement(H),        // 25 DEC H
	LoadImmediate(H),    // 26 LD  H,d8
	Todo, // 27
	JumpRelative(ZE),    // 28 JR  Z,r8
	AddDirect(HL, HL),   // 29 ADD HL,HL
	LoadIndirectInc(A, HL, true), // 2a LDI A,(HL)
	Decrement(HL),       // 2b DEC HL
	Increment(L),        // 2c INC L
	Decrement(L),        // 2d DEC L
	LoadImmediate(L),    // 2e LD  L,d8
	Todo, // 2f
	JumpRelative(NC),    // 30 JR  NC,r8
	LoadImmediate(SP),   // 31 LD  SP,d16
	LoadIndirectInc(HL, A, false), // 32 LDD (HL),A
	Increment(SP),       // 33 INC SP
	Todo, // 34
	Todo, // 35
	Todo, // 36
	Todo, // 37
	JumpRelative(CA),    // 38 JR  C,r8
	AddDirect(HL, SP),   // 39 ADD HL,SP
	LoadIndirectInc(A, HL, false), // 3a LDD A,(HL)
	Decrement(SP),       // 3b DEC SP
	Increment(A),        // 3c INC A
	Decrement(A),        // 3d DEC A
	LoadImmediate(A),    // 3e LD  A,d8
	Todo, // 3f
	LoadDirect(B, B),    // 40 LD B,B
	LoadDirect(B, C),    // 41 LD B,C
	LoadDirect(B, D),    // 42 LD B,D
	LoadDirect(B, E),    // 43 LD B,E
	LoadDirect(B, H),    // 44 LD B,H
	LoadDirect(B, L),    // 45 LD B,L
	LoadIndirect(B, HL), // 46 LD B,(HL)
	LoadDirect(B, A),    // 47 LD B,A
	LoadDirect(C, B),    // 48 LD C,B
	LoadDirect(C, C),    // 49 LD C,C
	LoadDirect(C, D),    // 4a LD C,D
	LoadDirect(C, E),    // 4b LD C,E
	LoadDirect(C, H),    // 4c LD C,H
	LoadDirect(C, L),    // 4d LD C,L
	LoadIndirect(C, HL), // 4e LD C,(HL)
	LoadDirect(C, A),    // 4f LD C,A
	LoadDirect(D, B),    // 50 LD D,B
	LoadDirect(D, C),    // 51 LD D,C
	LoadDirect(D, D),    // 52 LD D,D
	LoadDirect(D, E),    // 53 LD D,E
	LoadDirect(D, H),    // 54 LD D,H
	LoadDirect(D, L),    // 55 LD D,L
	LoadIndirect(D, HL), // 56 LD D,(HL)
	LoadDirect(D, A),    // 57 LD D,A
	LoadDirect(E, B),    // 58 LD E,B
	LoadDirect(E, C),    // 59 LD E,C
	LoadDirect(E, D),    // 5a LD E,D
	LoadDirect(E, E),    // 5b LD E,E
	LoadDirect(E, H),    // 5c LD E,H
	LoadDirect(E, L),    // 5d LD E,L
	LoadIndirect(E, HL), // 5e LD E,(HL)
	LoadDirect(E, A),    // 5f LD E,A
	LoadDirect(H, B),    // 60 LD H,B
	LoadDirect(H, C),    // 61 LD H,C
	LoadDirect(H, D),    // 62 LD H,D
	LoadDirect(H, E),    // 63 LD H,E
	LoadDirect(H, H),    // 64 LD H,H
	LoadDirect(H, L),    // 65 LD H,L
	LoadIndirect(H, HL), // 66 LD H,(HL)
	LoadDirect(H, A),    // 67 LD H,A
	LoadDirect(L, B),    // 68 LD L,B
	LoadDirect(L, C),    // 69 LD L,C
	LoadDirect(L, D),    // 6a LD L,D
	LoadDirect(L, E),    // 6b LD L,E
	LoadDirect(L, H),    // 6c LD L,H
	LoadDirect(L, L),    // 6d LD L,L
	LoadIndirect(L, HL), // 6e LD L,(HL)
	LoadDirect(L, A),    // 6f LD L,A
	LoadIndirect(HL, B), // 70 LD (HL),B
	LoadIndirect(HL, C), // 71 LD (HL),C
	LoadIndirect(HL, D), // 72 LD (HL),D
	LoadIndirect(HL, E), // 73 LD (HL),E
	LoadIndirect(HL, H), // 74 LD (HL),H
	LoadIndirect(HL, L), // 75 LD (HL),L
	Halt(true),          // 76 HALT
	LoadIndirect(HL, A), // 77 LD (HL),A
	LoadDirect(A, B),    // 78 LD A,B
	LoadDirect(A, C),    // 79 LD A,C
	LoadDirect(A, D),    // 7a LD A,D
	LoadDirect(A, E),    // 7b LD A,E
	LoadDirect(A, H),    // 7c LD A,H
	LoadDirect(A, L),    // 7d LD A,L
	LoadIndirect(A, HL), // 7e LD A,(HL)
	LoadDirect(A, A),    // 7f LD A,A
	AddDirect(A, B, false),    // 80 ADD A,B
	AddDirect(A, C, false),    // 81 ADD A,C
	AddDirect(A, D, false),    // 82 ADD A,D
	AddDirect(A, E, false),    // 83 ADD A,E
	AddDirect(A, H, false),    // 84 ADD A,H
	AddDirect(A, L, false),    // 85 ADD A,L
	AddIndirect(A, HL, false), // 86 ADD A,(HL)
	AddDirect(A, A, false),    // 87 ADD A,A
	AddDirect(A, B, true),     // 88 ADC A,B
	AddDirect(A, C, true),     // 89 ADC A,C
	AddDirect(A, D, true),     // 8a ADC A,D
	AddDirect(A, E, true),     // 8b ADC A,E
	AddDirect(A, H, true),     // 8c ADC A,H
	AddDirect(A, L, true),     // 8d ADC A,L
	AddIndirect(A, HL, false), // 8e ADC A,(HL)
	AddDirect(A, A, true),     // 8f ADC A,A
	SubDirect(A, B, false),    // 90 SUB A,B
	SubDirect(A, C, false),    // 91 SUB A,C
	SubDirect(A, D, false),    // 92 SUB A,D
	SubDirect(A, E, false),    // 93 SUB A,E
	SubDirect(A, H, false),    // 94 SUB A,H
	SubDirect(A, L, false),    // 95 SUB A,L
	SubIndirect(A, HL, false), // 96 SUB A,(HL)
	SubDirect(A, A, false),    // 97 SUB A,A
	SubDirect(A, B, true),     // 98 SBC A,B
	SubDirect(A, C, true),     // 99 SBC A,C
	SubDirect(A, D, true),     // 9a SBC A,D
	SubDirect(A, E, true),     // 9b SBC A,E
	SubDirect(A, H, true),     // 9c SBC A,H
	SubDirect(A, L, true),     // 9d SBC A,L
	SubIndirect(A, HL, true),  // 9e SBC A,(HL)
	SubDirect(A, A, true),     // 9f SBC A,A
	AndDirect(A, B),     // a0 AND A,B
	AndDirect(A, C),     // a1 AND A,C
	AndDirect(A, D),     // a2 AND A,D
	AndDirect(A, E),     // a3 AND A,E
	AndDirect(A, H),     // a4 AND A,H
	AndDirect(A, L),     // a5 AND A,L
	AndIndirect(A, HL),  // a6 AND A,(HL)
	AndDirect(A, A),     // a7 AND A,A
	XorDirect(A, B),     // a8 XOR A,B
	XorDirect(A, C),     // a9 XOR A,C
	XorDirect(A, D),     // aa XOR A,D
	XorDirect(A, E),     // ab XOR A,E
	XorDirect(A, H),     // ac XOR A,H
	XorDirect(A, L),     // ad XOR A,L
	XorIndirect(A, HL),  // ae XOR A,(HL)
	XorDirect(A, A),     // af XOR A,A
	OrDirect(A, B),      // b0 OR  A,B
	OrDirect(A, C),      // b1 OR  A,C
	OrDirect(A, D),      // b2 OR  A,D
	OrDirect(A, E),      // b3 OR  A,E
	OrDirect(A, H),      // b4 OR  A,H
	OrDirect(A, L),      // b5 OR  A,L
	OrIndirect(A, HL),   // b6 OR  A,(HL)
	OrDirect(A, A),      // b7 OR  A,A
	CmpDirect(A, B),     // b8 CP  A,B
	CmpDirect(A, C),     // b9 CP  A,C
	CmpDirect(A, D),     // ba CP  A,D
	CmpDirect(A, E),     // bb CP  A,E
	CmpDirect(A, H),     // bc CP  A,H
	CmpDirect(A, L),     // bd CP  A,L
	CmpIndirect(A, HL),  // be CP  A,(HL)
	CmpDirect(A, A),     // bf CP  A,A
	Todo, // c0
	Todo, // c1
	JumpAbsolute(NZ),       // c2 JP  NZ,a16
	JumpAbsolute(NO),       // c3 JP  a16
	Todo, // c4
	Todo, // c5
	AddImmediate(A, false), // c6 ADD A,d8
	Todo, // c7
	Todo, // c8
	Todo, // c9
	JumpAbsolute(ZE),       // ca JP  Z,a16
	HandleCB,               // cb PREFIX: See cbhandlers
	Todo, // cc
	Todo, // cd
	AddImmediate(A, true),  // ce ADC A,d8
	Todo, // cf
	Todo, // d0
	Todo, // d1
	JumpAbsolute(NC),       // d2 JP  NC,a16
	Wrong,                  // d3
	Todo, // d4
	Todo, // d5
	SubImmediate(A, false), // d6 SUB A,d8
	Todo, // d7
	Todo, // d8
	Todo, // d9
	JumpAbsolute(CA),       // da JP  C,a16
	Wrong,                  // db
	Todo, // dc
	Wrong,                  // dd
	SubImmediate(A, true),  // de SBC A,d8
	Todo, // df
	LoadHighAbs(A),         // e0 LDH (a8),A
	Todo, // e1
	LoadHighMem(C, A),      // e2 LD  (C),A
	Wrong,                  // e3
	Wrong,                  // e4
	Todo, // e5
	AndImmediate(A),        // e6 AND A,d8
	Todo, // e7
	AddImmediateS(SP),      // e8 ADD SP,r8
	JumpAbsolute(HL),       // e9 JP  (HL)
	LoadToMemory(A),        // ea LD  (a16),A
	Wrong,                  // eb
	Wrong,                  // ec
	Wrong,                  // ed
	XorImmediate(A),        // ee XOR A,d8
	Todo, // ef
	LoadHighReg(A),         // f0 LDH A,(a8)
	Todo, // f1
	LoadHighReg(A, C),      // f2 LD  A,(C)
	SetInt(false),          // f3 DI
	Wrong,                  // f4
	Todo, // f5
	OrImmediate(A),         // f6 OR  A,d8
	Todo, // f7
	Todo, // f8
	LoadDirect(SP, HL),     // f9 LD  SP,HL
	LoadFromMemory(A),      // fa LD  A,(a16)
	SetInt(true),           // fb EI
	Wrong,                  // fc
	Wrong,                  // fd
	CmpImmediate(A),        // fe CP  A,d8
	Todo  // ff
};



void CPU::Execute(const uint8_t opcode) {
	handlers[opcode](this);
}
