#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

// Define structures for each instruction type
typedef struct {
    uint32_t rs;
    uint32_t rt;
    uint32_t imm;
} TypeI;

typedef struct {
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;
} TypeR;

typedef struct {
    uint32_t target_address;
} TypeJ;

// Function prototypes
TypeI readTypeI(uint32_t instruction);
TypeR readTypeR(uint32_t instruction);
TypeJ readTypeJ(uint32_t instruction);







int main(int argc, char * argv[]) {

	/*
	 * This variable will store the maximum
	 * number of instructions to run before
	 * forcibly terminating the program. It
	 * is set via a command line argument.
	 */
	uint32_t MaxInstructions;

	/*
	 * This variable will store the address
	 * of the next instruction to be fetched
	 * from the instruction memory.
	 */
	uint32_t ProgramCounter;

	/*
	 * This variable will store the instruction
	 * once it is fetched from instruction memory.
	 */
	uint32_t CurrentInstruction;

	//IF THE USER HAS NOT SPECIFIED ENOUGH COMMAND LINE ARUGMENTS
	if(argc < 3){

		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Input argument missing!\n");
		fprintf(stderr, "Expected: file-name, max-instructions\n");
		return -1;

	}

     	//CONVERT MAX INSTRUCTIONS FROM STRING TO INTEGER	
	MaxInstructions = atoi(argv[2]);	

	//Open file pointers & initialize Heap & Regsiters
	initHeap();
	initFDT();
	initRegFile(0);

	//LOAD ELF FILE INTO MEMORY AND STORE EXIT STATUS
	int status = LoadOSMemory(argv[1]);

	//IF LOADING FILE RETURNED NEGATIVE EXIT STATUS
	if(status < 0){ 
		
		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Unable to open file at %s!\n", argv[1]);
		return status; 
	
	}

	printf("\n ----- BOOT Sequence ----- \n");
	printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", exec.GSP, exec.GP, exec.GPC_START);

	RegFile[28] = exec.GP;
	RegFile[29] = exec.GSP;
	RegFile[31] = exec.GPC_START;

	printRegFile();

	printf("\n ----- Execute Program ----- \n");
	printf("Max Instruction to run = %d \n",MaxInstructions);
	fflush(stdout);
	ProgramCounter = exec.GPC_START;

	//VARIABLE DECLARATIONS
	uint32_t result;
	uint64_t result64;
    int32_t rs_signed;
    int32_t rt_signed;
	int32_t imm_signed;
	int32_t imm_ext;
	uint32_t imm_upper;
	int32_t offset;
	TypeR valsR;
	TypeI valsI;
	TypeJ valsJ;

	int i;
	for(i = 0; i < MaxInstructions; i++) {

	uint32_t result = 0;
	uint64_t result64 = 0;
    int32_t rs_signed=0;
    int32_t rt_signed=0;
	int32_t imm_signed=0;
	int32_t imm_ext=0;
	uint32_t imm_upper=0;
	int32_t offset=0;
	TypeR valsR = {0,0,0,0,0};
	TypeI valsI = {0,0,0};
	TypeJ valsJ = {0};

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,false);
		printf("Instruction: 0x%08x\n", CurrentInstruction);

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();

		//GET OPCODE
		uint32_t opcode = (CurrentInstruction >> 26);
		
		//Decode instruction for all types of instructions. Only use the relevant type for each instruction
		valsR = readTypeR(CurrentInstruction);
		valsI = readTypeI(CurrentInstruction);
		valsJ = readTypeJ(CurrentInstruction);
		
		// EXECUTE INSTRUCTION
		// R type instructions //
		if (opcode == 0b000000) { //R type instructions
			switch(valsR.funct)
			{
				// ALU Instructions //
				case 0b100000: //add
					printf("ADD: %d + %d\n", RegFile[valsR.rs], RegFile[valsR.rt]);     
					RegFile[valsR.rd] = RegFile[valsR.rs] + RegFile[valsR.rt];   
					break;
				case 0b100010: //subtract
					// Sign extension
					rs_signed = (int32_t)RegFile[valsR.rs];  
					rt_signed = (int32_t)RegFile[valsR.rt];  
					result = rs_signed - rt_signed;    
					//rd = rs - rt   
					RegFile[valsR.rd] = (uint32_t)result;
					break;
				case 0b100011: //subtract unsigned
					// rd = rs - rt
					RegFile[valsR.rd] = RegFile[valsR.rs] - RegFile[valsR.rt];
					break;
				case 0b100001: //add unsigned
					// rd = rs + rt
					RegFile[valsR.rd] = RegFile[valsR.rs] + RegFile[valsR.rt];
					break;
				case 0b100100://AND
					RegFile[valsR.rd] = RegFile[valsR.rs] & RegFile[valsR.rt];
					break;
				case 0b100101://OR
					RegFile[valsR.rd] = RegFile[valsR.rs] | RegFile[valsR.rt];
					break;		
				case 0b100110: //XOR (exclusive or)
					//rd is bitwise xor of rs and rt
					RegFile[valsR.rd] = RegFile[valsR.rs] ^ RegFile[valsR.rt];
					break;
				case 0b100111: // NOR
					//rd is bitwise nor of rs and rt
					RegFile[valsR.rd] = ~(RegFile[valsR.rs] | RegFile[valsR.rt]);
					break;
				case 0b101010: // Set Less Than (signed)
					//Sign extension
					rs_signed = (int32_t)RegFile[valsR.rs];
					rt_signed = (int32_t)RegFile[valsR.rt];
					if (rs_signed < rt_signed) {
						RegFile[valsR.rd] = 1;
					} else {
						RegFile[valsR.rd] = 0;
					}
					break;
				case 0b101011: // Set Less than unsigned
					if (RegFile[valsR.rs] < RegFile[valsR.rt]){
						RegFile[valsR.rd] = 1;
					}else{
						RegFile[valsR.rd] = 0;
					}
					break;
		
				// Shift Instructions //
				case 0b000000: //sll (shift left logical)
					RegFile[valsR.rd] = RegFile[valsR.rt] << valsR.shamt;
					break;
				case 0b000010: //srl (shift right logical)
					RegFile[valsR.rd] = RegFile[valsR.rt] >> valsR.shamt;
					break;
				case 0b000011: //sra (shift right arithmetic)
					RegFile[valsR.rd] = (int32_t)RegFile[valsR.rt] >> valsR.shamt;
					break;
				case 0b000100: //sllv (shift left logical variable)
					RegFile[valsR.rd] = RegFile[valsR.rt] >> RegFile[valsR.rs];
					break;
				case 0b000110: // srlv (shift right logical variable)
					RegFile[valsR.rd] = RegFile[valsR.rt] >> RegFile[valsR.rs];
					break;
				case 0b000111: // srav (shift right arithmetic variable)
					RegFile[valsR.rd] = (int32_t)RegFile[valsR.rt] >> RegFile[valsR.rs];
					break;					

				// Multiplication and Division Instructions //
				case 0b010000: // mfhi (Move from HI)
					RegFile[valsR.rd] = RegFile[32];
					break;
				case 0b010010: // mflo (Move from LO)
					RegFile[valsR.rd] = RegFile[33];
					break;
				case 0b010001 : // mthi (Move to HI)
					RegFile[33] = RegFile[valsR.rs];
					break;
				case 0b010011 : // mtlo (Move to LO)
					RegFile[32] = RegFile[valsR.rs];
					break;
				case 0b011000: // mult (Multiply)
					result64 = (int64_t)((int32_t)RegFile[valsR.rs]) * (int64_t)((int32_t)RegFile[valsR.rt]);
					RegFile[32] = (uint32_t)(result64 >> 32);
					RegFile[33] = (uint32_t)(result64 & 0xFFFFFFFF);
					break;
				case 0b011001: // multu (Multiply unsigned)
					result64 = (uint64_t)RegFile[valsR.rs] * (uint64_t)RegFile[valsR.rt];
					RegFile[32] = (uint32_t)(result64 >> 32);
					RegFile[33] = (uint32_t)(result64 & 0xFFFFFFFF);
					break;
				case 0b011010: // div (Divide)
					rs_signed = (int32_t)RegFile[valsR.rs];
					rt_signed = (int32_t)RegFile[valsR.rt];
					if (rt_signed != 0)
					{
						RegFile[33] = (uint32_t)(rs_signed / rt_signed);
						RegFile[32] = (uint32_t)(rs_signed % rt_signed);
					}
					break;
				case 0b011011: // divu (Divide unsigned)
					if (RegFile[valsR.rt] != 0)
					{
						RegFile[33] = RegFile[valsR.rs] / RegFile[valsR.rt];
						RegFile[32] = RegFile[valsR.rs] % RegFile[valsR.rt];
					}

			  	// Jump Instructions //
				case 0b001000: // jr (Jump Register)
					//jump to address specified by rs
					ProgramCounter = RegFile[valsR.rs]; 
					break;
				case 0b001001: // jalr (Jump and Link Register)
					RegFile[31] = ProgramCounter + 4; // Save address of next instruction into RA
					 // Jump to address specified by rs
					ProgramCounter = RegFile[valsR.rs];
					break;
				default:
					break;
			}
		}

	    // I type ALU instructions //
		if (opcode == 0b001001){ //add immediate unsigned
			printf("ADDIU: %d + %d\n", RegFile[valsI.rs], valsI.imm);
			//rt = rs + immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] + (int16_t)valsI.imm;
		} if (opcode == 0b001000){ //add immediate (signed)
			imm_signed = (int32_t)(int16_t)valsI.imm;
    		// Perform addition with sign-extended immediate value
    		RegFile[valsI.rs] = RegFile[valsI.rt] + imm_signed;
		} if (opcode == 0b001010){ //slti (set less than immediate)
			//Sign extension
			rs_signed = (int32_t)RegFile[valsI.rs];
			imm_signed = (int32_t)(int16_t)valsI.imm;
			if (rs_signed < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			
			}
		} if (opcode == 0b001011) { // sltiu (Set Less than unsigned immediate)
			if (RegFile[valsI.rs] < valsI.imm){
				RegFile[valsI.rt] = 1;
			}else{
				RegFile[valsI.rt] = 0;
			}
		} if (opcode == 0b001000) { // andi (And Immediate)
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise and between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] & imm_ext;
		} if (opcode == 0b001101) { // ori (or Immediate)
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise or between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] | imm_ext;
		} if (opcode == 0b001110) { // xori (exclusive or Immediate) 
			// Zero-Extend to 32 bits
			imm_ext = (int32_t)valsI.imm;
			//rt is bitwise xor between rs and immediate
			RegFile[valsI.rt] = RegFile[valsI.rs] ^ imm_ext;
		} if (opcode == 0b001111) { // lui (Load Upper Immediate)
			// Extract upper 16 bits of the immediate value and left-shift it by 16 bits
			imm_upper = (valsI.imm & 0xFFFF) << 16;
			RegFile[valsI.rt] = imm_upper;
		}

		// Load and Store Instructions //
		printf("Opcode: %d\n", opcode);
		if (opcode == 0b100000) { // lb (Load Byte)
			RegFile[valsI.rt] = readByte(RegFile[valsI.rs] + valsI.imm, false);
		} if (opcode == 0b100001) { // lh (Load Halfword)
			RegFile[valsI.rt] = readWord(RegFile[valsI.rs] + valsI.imm, false);     // might not work
		} if (opcode == 0b100010) { // lwl (Load Word Left)
			RegFile[valsI.rt] = readWord(RegFile[valsI.rs] + valsI.imm, false);
		} if (opcode == 0b100110) { // lwr (Load Word Right)
			RegFile[valsI.rt] = readWord(RegFile[valsI.rs] + valsI.imm, false);
		} if (opcode == 0b100011) { // lw (Load Word)
			printf("Load Word into %d: %d + %d\n", valsI.rt, RegFile[valsI.rs], valsI.imm);
			RegFile[valsI.rt] = readWord((uint32_t)(RegFile[valsI.rs] + valsI.imm), false);
		} if (opcode == 0b100100) { // lbu (Load Byte Unsigned)
			RegFile[valsI.rt] = readByte(RegFile[valsI.rs] + valsI.imm, false);
		} if (opcode == 0b100101) { // lhu (Load Halfword Unsigned)
			RegFile[valsI.rt] = readWord(RegFile[valsI.rs] + valsI.imm, false);     // might not work
		} if (opcode == 0b101000) { // sb (Store Byte)
			writeByte(RegFile[valsI.rs] + valsI.imm, RegFile[valsI.rt], false);
		} if (opcode == 0b101001) { // sh (Store Halfword)
			writeWord(RegFile[valsI.rs] + valsI.imm, RegFile[valsI.rt], false);     // might not work
		} if (opcode == 0b101011) { // sw (Store Word)
			writeWord(RegFile[valsI.rs] + valsI.imm, RegFile[valsI.rt], false);
		} if (opcode == 0b101010) { // swl (Store Word Left)
			writeWord(RegFile[valsI.rs] + valsI.imm, RegFile[valsI.rt], false);
		} if (opcode == 0b101110) { // swr (Store Word Right)
			writeWord(RegFile[valsI.rs] + valsI.imm, RegFile[valsI.rt], false);
		}

		// Branch Instructions //
		if (opcode == 0b00001){ // for all I type instructions with opcode 000001
			switch(valsI.rt) 
			{			
				case 0b000000: //BLTZ (Branch if Less Than Zero)
					if (RegFile[valsI.rs] < 0){
						//sign extension
						offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2);
					}
					break;
				case 0b000001: // BGEZ (Branch if Greater Than or Equal to Zero)
					if ((int32_t)RegFile[valsI.rs] >= 0) {
						// Sign extension
						offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2); // Calculate branch target address
					}
					break;
				case 0b010000: //BLTZAL (Branch on Less Than Zero and Link)
					if (RegFile[valsI.rs] < 0){
						// Save the return address in register $31 (RA)
						RegFile[31] = ProgramCounter + 4;
						//sign extension
						offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2);                        
					}
					break;
				case 0b010001: //BGEZAL (Branch on Greater Than or Equal to Zero and Link)
					if ((int32_t)RegFile[valsI.rs] >= 0) {
						// Save the return address in register $31 (RA)
						RegFile[31] = ProgramCounter + 4;
						//sign extension
						offset = (int32_t)valsI.imm;
						ProgramCounter += 4 + (offset << 2);                       
					}
					break;
			}
		 }

		if (opcode == 0b000100){//BEQ (branch on equal)
			if (RegFile[valsI.rs] == RegFile[valsI.rt]){
				//sign extension on offset
				offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		} if (opcode == 0b000101){//BNE (branch on not equal)
			if (RegFile[valsI.rs] != RegFile[valsI.rt]){
				//sign extension on offset
				offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		} if (opcode == 0b000110) { // BLEZ (branch on less than or equal to zero)
    		if (RegFile[valsI.rs] <= 0) {
				// Sign extension on offset
				offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		}if (opcode == 0b000111) { // BGTZ (branch on greater than zero)
			if (RegFile[valsI.rs] > 0) {
				// Sign extension on offset
				offset = (int32_t)valsI.imm;
				ProgramCounter += 4 + (offset << 2);
			}
		}

		// J type instructions //
		if (opcode == 0b000010){ // J (jump)
			//concatenate the first 4 bits of the current PC with the lower 26 bits of target address.
			//The last two bits of the address can be assumed to be 0 since instructions are word-aligned
			ProgramCounter = (ProgramCounter & 0xF0000000) | (valsJ.target_address << 2);
		} if (opcode == 0b000011) { // jal (jump and link)
			// Store the return address (address of the next instruction) in register $ra (31)
			RegFile[31] = ProgramCounter + 4; 
			
			// Jump to the target address
			ProgramCounter = (ProgramCounter & 0xF0000000) | (valsJ.target_address << 2);
		}

		// Exception Instructions //
		if (opcode == 0b000000) { // syscall and exception
			uint32_t code = CurrentInstruction & 0x3FFFFC0;
			uint32_t brk = CurrentInstruction & 0x03F;
			if (brk == 0b001100) { // syscall
				syscall();
			} if (brk == 0b001101) { // break
				break;
			}
		}

		RegFile[0] = 0;	  
		ProgramCounter += 4; // Increment the program counter to the next instruction 
	}

	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;


}//end of main

//Function Definitions
TypeI readTypeI(uint32_t instruction) {
    TypeI result;
    result.rs = (instruction << 6) >> 27;
    result.rt = (instruction << 11) >> 27;
    result.imm = (instruction << 16) >> 16;
    return result;
}

TypeR readTypeR(uint32_t instruction) {
    TypeR result;
    result.rs = (instruction >> 21) & 0x1F;
    result.rt = (instruction >> 16) & 0x1F;
    result.rd = (instruction >> 11) & 0x1F;
    result.shamt = (instruction >> 6) & 0x1F;
    result.funct = instruction & 0x3F;
    return result;
}

TypeJ readTypeJ(uint32_t instruction) {
    TypeJ result;
    result.target_address = instruction & 0x03FFFFFF; // 26 bits
    return result;
}