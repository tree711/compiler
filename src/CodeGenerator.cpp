#include "../include/IR.h"
#include <fstream>
#include <sstream>

class CodeGenerator {
private:
    std::ofstream output;
    std::unordered_map<std::string, int> variableOffsets;
    int stackOffset;

    void generateFunction(FunctionIR& func);
    void generateBasicBlock(BasicBlock& block);
    std::string getOperand(const std::string& op);

public:
    CodeGenerator(const std::string& filename) : stackOffset(0) {
        output.open(filename);
        if (!output.is_open()) {
            throw std::runtime_error("Cannot open output file: " + filename);
        }
    }

    ~CodeGenerator() {
        if (output.is_open()) {
            output.close();
        }
    }

    void generate(IRProgram& program) {
        output << ".data\n";
        for (const std::string& global : program.globalVariables) {
            output << "\t" << global << ":\t.long 0\n";
        }
        output << "\n.text\n";

        for (auto& func : program.functions) {
            generateFunction(*func);
        }
    }
};

void CodeGenerator::generateFunction(FunctionIR& func) {
    output << "\t.globl " << func.name << "\n";
    output << func.name << ":\n";
    
    output << "\tpushq %rbp\n";
    output << "\tmovq %rsp, %rbp\n";
    
    stackOffset = 0;
    variableOffsets.clear();
    
    for (auto& block : func.blocks) {
        generateBasicBlock(*block);
    }
    
    output << "\tpopq %rbp\n";
    output << "\tret\n";
}

void CodeGenerator::generateBasicBlock(BasicBlock& block) {
    output << block.name << ":\n";
    
    for (auto& instr : block.instructions) {
        switch (instr->opcode) {
            case IROpcode::CONST: {
                std::string result = instr->result;
                std::string value = instr->operands[0];
                output << "\tmovl $" << value << ", %eax\n";
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::LOAD: {
                std::string result = instr->result;
                std::string var = instr->operands[0];
                if (variableOffsets.find(var) != variableOffsets.end()) {
                    int offset = variableOffsets[var];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << var << ", %eax\n";
                }
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::STORE: {
                std::string var = instr->result;
                std::string value = instr->operands[0];
                if (variableOffsets.find(value) != variableOffsets.end()) {
                    int offset = variableOffsets[value];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << value << ", %eax\n";
                }
                output << "\tmovl %eax, " << var << "\n";
                break;
            }
            case IROpcode::ADD: {
                std::string result = instr->result;
                std::string left = instr->operands[0];
                std::string right = instr->operands[1];
                
                if (variableOffsets.find(left) != variableOffsets.end()) {
                    int offset = variableOffsets[left];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << left << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\taddl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\taddl " << right << ", %eax\n";
                }
                
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::SUB: {
                std::string result = instr->result;
                std::string left = instr->operands[0];
                std::string right = instr->operands[1];
                
                if (variableOffsets.find(left) != variableOffsets.end()) {
                    int offset = variableOffsets[left];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << left << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\tsubl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tsubl " << right << ", %eax\n";
                }
                
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::MUL: {
                std::string result = instr->result;
                std::string left = instr->operands[0];
                std::string right = instr->operands[1];
                
                if (variableOffsets.find(left) != variableOffsets.end()) {
                    int offset = variableOffsets[left];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << left << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\timull " << offset << "(%rbp)\n";
                } else {
                    output << "\timull " << right << "\n";
                }
                
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::DIV: {
                std::string result = instr->result;
                std::string left = instr->operands[0];
                std::string right = instr->operands[1];
                
                output << "\tmovl $0, %edx\n";
                if (variableOffsets.find(left) != variableOffsets.end()) {
                    int offset = variableOffsets[left];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << left << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\tidivl " << offset << "(%rbp)\n";
                } else {
                    output << "\tidivl " << right << "\n";
                }
                
                variableOffsets[result] = stackOffset;
                stackOffset -= 4;
                output << "\tpushl %eax\n";
                break;
            }
            case IROpcode::BRANCH: {
                std::string target = instr->operands[0];
                output << "\tjmp " << target << "\n";
                break;
            }
            case IROpcode::BRANCH_EQ: {
                std::string cond = instr->operands[0];
                std::string value = instr->operands[1];
                std::string target = instr->operands[2];
                
                if (variableOffsets.find(cond) != variableOffsets.end()) {
                    int offset = variableOffsets[cond];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << cond << ", %eax\n";
                }
                output << "\tcmp $" << value << ", %eax\n";
                output << "\tje " << target << "\n";
                break;
            }
            case IROpcode::RET: {
                if (!instr->operands.empty()) {
                    std::string value = instr->operands[0];
                    if (variableOffsets.find(value) != variableOffsets.end()) {
                        int offset = variableOffsets[value];
                        output << "\tmovl " << offset << "(%rbp), %eax\n";
                    } else {
                        output << "\tmovl " << value << ", %eax\n";
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

std::string CodeGenerator::getOperand(const std::string& op) {
    if (op.empty()) return "";
    if (op[0] == '%') {
        auto it = variableOffsets.find(op);
        if (it != variableOffsets.end()) {
            return std::to_string(it->second) + "(%rbp)";
        }
    }
    return op;
}
