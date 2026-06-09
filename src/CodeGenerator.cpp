#include "../include/CodeGenerator.h"
#include <sstream>
#include <cctype>

static bool isTemp(const std::string& value) {
    return !value.empty() && value[0] == '%';
}

static int alignTo16(int value) {
    return (value + 15) / 16 * 16;
}

static bool isIntegerLiteral(const std::string& value) {
    if (value.empty()) return false;
    size_t start = value[0] == '-' ? 1 : 0;
    if (start == value.size()) return false;
    for (size_t i = start; i < value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }
    return true;
}

static std::string asmOperand(const std::string& value) {
    return isIntegerLiteral(value) ? "$" + value : value;
}

static void assignTempOffsets(FunctionIR& func,
                              std::unordered_map<std::string, int>& variableOffsets,
                              int& stackOffset) {
    stackOffset = 0;
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (isTemp(instr->result) && variableOffsets.find(instr->result) == variableOffsets.end()) {
                stackOffset -= 4;
                variableOffsets[instr->result] = stackOffset;
            }
        }
    }
}

static void storeTemp(std::ofstream& output,
                      const std::unordered_map<std::string, int>& variableOffsets,
                      const std::string& result) {
    auto it = variableOffsets.find(result);
    if (it != variableOffsets.end()) {
        output << "\tmovl %eax, " << it->second << "(%rbp)\n";
    }
}

CodeGenerator::CodeGenerator(const std::string& filename) : stackOffset(0) {
    output.open(filename);
    if (!output.is_open()) {
        throw std::runtime_error("Cannot open output file: " + filename);
    }
}

CodeGenerator::~CodeGenerator() {
    if (output.is_open()) {
        output.close();
    }
}

void CodeGenerator::generate(IRProgram& program) {
    output << ".data\n";
    for (const std::string& global : program.globalVariables) {
        output << "\t" << global << ":\t.long 0\n";
    }
    output << "\n.text\n";

    for (auto& func : program.functions) {
        generateFunction(*func);
    }
}

void CodeGenerator::generateFunction(FunctionIR& func) {
    output << "\t.globl " << func.name << "\n";
    output << func.name << ":\n";
    
    stackOffset = 0;
    variableOffsets.clear();
    assignTempOffsets(func, variableOffsets, stackOffset);
    int frameSize = alignTo16(-stackOffset);

    output << "\tpushq %rbp\n";
    output << "\tmovq %rsp, %rbp\n";
    if (frameSize > 0) {
        output << "\tsubq $" << frameSize << ", %rsp\n";
    }
    
    for (auto& block : func.blocks) {
        generateBasicBlock(*block);
    }
    
    output << "\tmovq %rbp, %rsp\n";
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
                storeTemp(output, variableOffsets, result);
                break;
            }
            case IROpcode::LOAD: {
                std::string result = instr->result;
                std::string var = instr->operands[0];
                if (variableOffsets.find(var) != variableOffsets.end()) {
                    int offset = variableOffsets[var];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << asmOperand(var) << ", %eax\n";
                }
                storeTemp(output, variableOffsets, result);
                break;
            }
            case IROpcode::STORE: {
                std::string var = instr->result;
                std::string value = instr->operands[0];
                if (variableOffsets.find(value) != variableOffsets.end()) {
                    int offset = variableOffsets[value];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << asmOperand(value) << ", %eax\n";
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
                    output << "\tmovl " << asmOperand(left) << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\taddl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\taddl " << asmOperand(right) << ", %eax\n";
                }
                
                storeTemp(output, variableOffsets, result);
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
                    output << "\tmovl " << asmOperand(left) << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\tsubl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tsubl " << asmOperand(right) << ", %eax\n";
                }
                
                storeTemp(output, variableOffsets, result);
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
                    output << "\tmovl " << asmOperand(left) << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\timull " << offset << "(%rbp)\n";
                } else {
                    output << "\timull " << right << "\n";
                }
                
                storeTemp(output, variableOffsets, result);
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
                    output << "\tmovl " << asmOperand(left) << ", %eax\n";
                }
                
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    int offset = variableOffsets[right];
                    output << "\tidivl " << offset << "(%rbp)\n";
                } else {
                    output << "\tidivl " << right << "\n";
                }
                
                storeTemp(output, variableOffsets, result);
                break;
            }
            // ---- 比较指令，结果为 0 或 1 ----
            case IROpcode::CMP_EQ:
            case IROpcode::CMP_NE:
            case IROpcode::CMP_LT:
            case IROpcode::CMP_GT:
            case IROpcode::CMP_LE:
            case IROpcode::CMP_GE: {
                std::string result = instr->result;
                std::string left = instr->operands[0];
                std::string right = instr->operands[1];

                // 加载左操作数
                if (variableOffsets.find(left) != variableOffsets.end()) {
                    output << "\tmovl " << variableOffsets[left] << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << asmOperand(left) << ", %eax\n";
                }
                // 与右操作数进行比较
                if (variableOffsets.find(right) != variableOffsets.end()) {
                    output << "\tcmpl " << variableOffsets[right] << "(%rbp), %eax\n";
                } else {
                    output << "\tcmpl " << asmOperand(right) << ", %eax\n";
                }
                // 根据比较结果设置条件字节
                const char* setcc = "";
                switch (instr->opcode) {
                    case IROpcode::CMP_EQ: setcc = "sete";  break;
                    case IROpcode::CMP_NE: setcc = "setne"; break;
                    case IROpcode::CMP_LT: setcc = "setl";  break;
                    case IROpcode::CMP_GT: setcc = "setg";  break;
                    case IROpcode::CMP_LE: setcc = "setle"; break;
                    case IROpcode::CMP_GE: setcc = "setge"; break;
                    default: break;
                }
                output << "\t" << setcc << " %al\n";
                output << "\tmovzbl %al, %eax\n";

                storeTemp(output, variableOffsets, result);
                break;
            }

            // ---- 分支跳转指令 ----
            case IROpcode::BRANCH: {
                std::string target = instr->operands[0];
                output << "\tjmp " << target << "\n";
                break;
            }
            case IROpcode::BRANCH_EQ:
            case IROpcode::BRANCH_NE:
            case IROpcode::BRANCH_LT:
            case IROpcode::BRANCH_GT:
            case IROpcode::BRANCH_LE:
            case IROpcode::BRANCH_GE: {
                std::string cond = instr->operands[0];
                std::string value = instr->operands[1];
                std::string target = instr->operands[2];

                // 加载条件值
                if (variableOffsets.find(cond) != variableOffsets.end()) {
                    int offset = variableOffsets[cond];
                    output << "\tmovl " << offset << "(%rbp), %eax\n";
                } else {
                    output << "\tmovl " << asmOperand(cond) << ", %eax\n";
                }
                output << "\tcmpl $" << value << ", %eax\n";

                const char* jcc = "";
                switch (instr->opcode) {
                    case IROpcode::BRANCH_EQ: jcc = "je";  break;
                    case IROpcode::BRANCH_NE: jcc = "jne"; break;
                    case IROpcode::BRANCH_LT: jcc = "jl";  break;
                    case IROpcode::BRANCH_GT: jcc = "jg";  break;
                    case IROpcode::BRANCH_LE: jcc = "jle"; break;
                    case IROpcode::BRANCH_GE: jcc = "jge"; break;
                    default: break;
                }
                output << "\t" << jcc << " " << target << "\n";
                break;
            }
            case IROpcode::RET: {
                if (!instr->operands.empty()) {
                    std::string value = instr->operands[0];
                    if (variableOffsets.find(value) != variableOffsets.end()) {
                        int offset = variableOffsets[value];
                        output << "\tmovl " << offset << "(%rbp), %eax\n";
                    } else {
                        output << "\tmovl " << asmOperand(value) << ", %eax\n";
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
