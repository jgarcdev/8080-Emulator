#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "instr-stages.h"
#include "machine.h"
#include "hardware.h"

extern machine_t guest;

static void extract(opcode_t* opcode, bool* reqDatabyte0, bool* reqDatabyte1) {
	uint8_t insn = guest.proc->IR;

	// Single register instructions
	if ((insn & 0b11000110) == 0b00000100) {
		// It is INR if lsb is 0, DCR if 1
		if ((insn & 0x1) == 0x0) *opcode = OP_INR;
		else *opcode = OP_DCR;
		return;
	} else if ((insn & 0b11110111) == 0b00100111) {
		if (((insn >> 3) & 0x1) == 0x1) *opcode = OP_CMA;
		return;
	}

	// Data transfer instructions
	if ((insn & 0b11000000) == 0b01000000) {
		*opcode = OP_MOV;
		return;
	} else if ((insn & 0b11100111) == 0b00000010) {
		if (((insn >> 3) & 0x1) == 0x1) *opcode = OP_LDAX;
		else *opcode = OP_STAX;
		return;
	}

	// Register or memory to accumulator instructions
	if (((insn >> 6) & 0b11) == 0b10) {
		uint8_t op = (insn >> 3) & 0b111;

		if (op == 0b000) *opcode = OP_ADD;
		else if (op == 0b001) *opcode = OP_ADC;
		else if (op == 0b010) *opcode = OP_SUB;
		// else if (op == 0b011) *opcode = OP_SBB;
		else if (op == 0b100) *opcode = OP_ANA;
		else if (op == 0b101) *opcode = OP_XRA;
		else if (op == 0b110) *opcode = OP_ORA;
		else *opcode = OP_CMP;

		return;
	}

	// Register pair instructions
	if ((insn & 0b11001011) == 0b11000001) {
		if (((insn >> 2) & 0x1) == 0x1) *opcode = OP_PUSH;
		else *opcode = OP_POP;
		return;
	} else if ((insn & 0b11000101) == 0b00000001) {
		uint8_t op = insn & 0b1010;

		if (op == 0b1000) { *opcode = OP_DAD; return; }
		else if (op == 0b0010) { *opcode = OP_INX; return; }
		else if (op == 0b1010) { *opcode = OP_DCX; return; }

		// return; // ?????
	} else if ((insn & 0b11100001) == 0b11100001) {
		uint8_t op = (insn >> 1) & 0b1111;

		if (op == 0b0101) *opcode = OP_XCHG;
		else if (op == 0b0001) *opcode = OP_XTHL;
		else *opcode = OP_SPHL;

		return;
	}

	// Immediate instructions
	if ((insn & 0b11001111) == 0b00000001) {
		// *opcode = OP_LXI;
	} else if ((insn & 0b11000111) == 0b00000110) {
		*reqDatabyte0 = true;
		*opcode = OP_MVI;
		return;
	} else if ((insn & 0b11000111) == 0b11000110) {
		*reqDatabyte0 = true;

		uint8_t op = (insn >> 3) & 0b111;

		if (op == 0b000) *opcode = OP_ADI;
		else if (op == 0b001) *opcode = OP_ACI;
		else if (op == 0b010) *opcode = OP_SUI;
		else if (op == 0b011) *opcode = OP_SBI;
		else if (op == 0b100) *opcode = OP_ANI;
		else if (op == 0b101) *opcode = OP_XRI;
		else if (op == 0b110) *opcode = OP_ORI;
		else *opcode = OP_CPI;

		return;
	}

	// Direct addressing instructions
	if ((insn & 0b11100111) == 0b00100010) {
		*reqDatabyte0 = true;
		*reqDatabyte1 = true;

		uint8_t op = ((insn >> 3) & 0b11);

		if (op == 0b10) *opcode = OP_STA;
		else if (op == 0b11) *opcode = OP_LDA;
		else if (op == 0b00) *opcode = OP_SHLD;
		else *opcode = OP_LHLD;

		return;
	}


	if (insn == 0b01110110) {
		*opcode = OP_HLT;
		return;
	}

	// All left is 0b00000000, meaning NOP
	*opcode = OP_NOP;
}

static void decideALUOP(alu_op_t* aluop, opcode_t opcode) {
	switch (opcode)	{
		case OP_INR: case OP_ADD: case OP_ADC: case OP_DAD: case OP_INX: case OP_ADI: case OP_ACI:
		case OP_STA: case OP_LDA: case OP_SHLD: case OP_LHLD:
			*aluop = PLUS_OP;
			break;
		case OP_DCR: case OP_SUB: case OP_CMP: case OP_DCX: case OP_SUI: case OP_SBI: case OP_CPI:
			*aluop = MINUS_OP;
			break;
		case OP_ANA: case OP_ANI:
			*aluop = AND_OP;
			break;
		case OP_XRA: case OP_XRI:
			*aluop = XOR_OP;
			break;
		case OP_ORA: case OP_ORI:
			*aluop = OR_OP;
			break;
		case OP_CMA:
			*aluop = NOT_OP;
			break;
		default:
			*aluop = PASS_OP;
			break;
	}
}

void decode(opcode_t* opcode, alu_op_t* aluop) {
	// When the processor is in waiting (aka halt), skip
	if (State.ctrSigs.WAIT) return;

	// T4

	bool reqDatabyte0 = false;
	bool reqDatabyte1 = false;

	extract(opcode, &reqDatabyte0, &reqDatabyte1);
	printf("Opcode is %d\n", *opcode);

	// If reqDatabyte*, a new machine cycle is to begin, one for each *
	if (reqDatabyte0) {
		printf("Databyte 0 needed! Starting new machine cycle!\n");

		// initMachineCycle();
	}
	if (reqDatabyte1) printf("Databyte 1 needed!\n");



	decideALUOP(aluop, *opcode);
	// Generate control signals used for Exec/Mem/Writeback
	printf("ALU OP is %d\n", *aluop);
	
}
