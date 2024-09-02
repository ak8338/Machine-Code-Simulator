
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

//Simulates the instructions for add,sub,or,and,slt and jr
void execute_instruction(uint16_t instr, uint16_t regs[], uint16_t& pc) {
    unsigned functionCode = instr & 0xF; 
    
    unsigned regSrcA = (instr >> 10) & 0x7; 
    unsigned regSrcB = (instr >> 7) & 0x7;  
    unsigned regDst = (instr >> 4) & 0x7;  
    const unsigned functionCodeAdd = 0b0000;  
    const unsigned functionCodeSub = 0b0001;  
    const unsigned functionCodeOr = 0b0010;  
    const unsigned functionCodeAnd = 0b0011;  
    const unsigned functionCodeSlt = 0b0100;  
    const unsigned functionCodeJr = 0b1000;  
 

    switch (functionCode) {
        case functionCodeAdd: 
            regs[regDst] = regs[regSrcA] + regs[regSrcB];
            break;
        case functionCodeSub: 
            regs[regDst] = regs[regSrcA] - regs[regSrcB];
            break;
        case functionCodeOr: 
            regs[regDst] = regs[regSrcA] | regs[regSrcB];
            break;
        case functionCodeAnd: 
            regs[regDst] = regs[regSrcA] & regs[regSrcB];
            break;
        case functionCodeSlt: 
            regs[regDst] = (regs[regSrcA] < regs[regSrcB]) ? 1 : 0;
            break;
        case functionCodeJr: 
            pc = regs[(instr >> 10) & 0x7] - 1; 
            break;
        default:
            
            break;
    }
    regs[0] = 0;

    pc += 1; 
}

//simulates instruction for  slti lw sw jeq and addi
void execute_imm_instruction(uint16_t instr, uint16_t regs[], uint16_t& pc, uint16_t memory[]) {
    uint16_t opcode = (instr >> 13) & 0x7; 
    uint16_t regSrc = (instr >> 10) & 0x7; 
    uint16_t regDst = (instr >> 7) & 0x7; 
    uint16_t imm = instr & 0x7F; 
    const uint16_t opcodeSlti = 0b111; 
    const uint16_t opcodeLw = 0b100;   
    const uint16_t opcodeSw = 0b101;  
    const uint16_t opcodeJeq = 0b110; 
    const uint16_t opcodeAddi = 0b001; 

    
    if (imm & 0x40) { 
        imm |= 0xFF80; 
    }

    switch (opcode) {
        case opcodeSlti: { 
            regs[regDst] = (static_cast<unsigned>(regs[regSrc]) < static_cast<unsigned>(imm)) ? 1 : 0;
            pc += 1;
            break;
        }
        case opcodeLw: { 
            uint16_t regAddr = (instr >> 9) & 0x7; 
            uint16_t regDst = (instr >> 6) & 0x7; 
            uint16_t imm = instr & 0x7F; 

            
            if (imm & 0x40) imm |= 0xFF80;

            uint16_t loadAddr = (regs[regAddr] + imm) & 8191; 
            if (loadAddr < (1 << 13)) { 
            regs[regDst] = memory[loadAddr]; 
            }
            pc += 1; 
            break;
        }

        case opcodeSw: { 
            uint16_t regAddr = (instr >> 9) & 0x7; 
            uint16_t regSrc = (instr >> 6) & 0x7; 
            uint16_t imm = instr & 0x7F; 
           
            if (imm & 0x40) imm |= 0xFF80;

            uint16_t storeAddr = (regs[regAddr] + imm) & 8191; 
            if (storeAddr < (1 << 13)) { 
                memory[storeAddr] = regs[regSrc]; 
                }
            pc += 1; 
            break;
        }
        case opcodeJeq: { 
            if (regs[regSrc] == regs[regDst]) { 
                pc = pc + 1 + imm; 
            } else {
                pc += 1; 
            }
            break;
        }
        case opcodeAddi: { 
            regs[regDst] = regs[regSrc] + imm; 
            pc += 1;
            break;
        }
    
        default: {
        
            pc += 1; 
            break;
        }
    }
    regs[0] = 0;
}


//simulates instructions for j and jal
void execute_control_instruction(uint16_t instr, uint16_t regs[], uint16_t& pc, bool& isHalt) {
    uint16_t opcode = (instr >> 13) & 0x7; 
    uint16_t imm = instr & 0x1FFF; 
    const uint16_t opcodeJ = 0b010;
    const uint16_t opcodeJal = 0b011; 
    
    switch (opcode) {
        case opcodeJ: 
            if (imm == pc) { 
                isHalt = true; 
            } else {
                pc = imm; 
            }
            break;
        case opcodeJal:
            regs[7] = pc + 1; 
            pc = imm; 
            break;
       
        default:
            
            break;
    }
    regs[0] = 0;
}


/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }
    uint16_t memory[MEM_SIZE] = {0}; 
    uint16_t regs[NUM_REGS] = {0}; 
    uint16_t pc = 0;
    bool isHalt = false;

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    // TODO: your code here. Load f and parse using load_machine_code
    load_machine_code(f, memory); // Load the machine code into memory
    f.close();

    // TODO: your code here. Do simulation.
    while (!isHalt) { // Simulation loop
        uint16_t instr = memory[pc & 8191]; 
        uint16_t imm = instr & 0x1FFF; 
        uint16_t opcode = (instr >> 13) & 0x7; 
        const uint16_t opcodeFirst = 0b000;
        const uint16_t opcodeSlti = 0b111; 
        const uint16_t opcodeLw = 0b100;   
        const uint16_t opcodeSw = 0b101;  
        const uint16_t opcodeJeq = 0b110; 
        const uint16_t opcodeAddi = 0b001; 
        const uint16_t opcodeJ = 0b010; 
        const uint16_t opcodeJal = 0b011;


        
        if (opcode == opcodeJ && imm == pc) {
            isHalt = true; 
            continue; 
        }
        if (opcode == opcodeFirst) {
            execute_instruction(instr, regs, pc); 
        } else if (opcode == opcodeSlti || opcode == opcodeAddi || opcode == opcodeLw || opcode == opcodeSw || opcode == opcodeJeq){
            execute_imm_instruction(instr, regs, pc, memory); 
        } else if (opcode == opcodeJ || opcode == opcodeJal) {
            execute_control_instruction(instr, regs, pc, isHalt); 
        }

        
    }

   
    print_state(pc, regs, memory, 128); 

    return 0;
}