CC = g++
CCFLAGS = -g
FILES = ASM.cpp ASMParser.cpp Instruction.cpp Opcode.cpp RegisterTable.cpp

asm: $(FILES)
	$(CC) $(CCFLAGS) $^ -o $@

