#include "../include/IR.h"
#include <unordered_map>
#include <algorithm>

class Optimizer {
private:
    void constantFolding(IRProgram& program);
    void deadCodeElimination(IRProgram& program);
    void copyPropagation(IRProgram& program);
    void optimizeFunction(FunctionIR& func);

public:
    void optimize(IRProgram& program) {
        for (auto& func : program.functions) {
            optimizeFunction(*func);
        }
    }
};

void Optimizer::optimizeFunction(FunctionIR& func) {
    constantFolding(func);
    copyPropagation(func);
    deadCodeElimination(func);
}

void Optimizer::constantFolding(IRProgram& program) {
    for (auto& func : program.functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::ADD || instr->opcode == IROpcode::SUB ||
                    instr->opcode == IROpcode::MUL || instr->opcode == IROpcode::DIV) {
                    bool allConst = true;
                    std::vector<int> values;
                    
                    for (const std::string& op : instr->operands) {
                        if (op.empty() || op[0] == '%') {
                            allConst = false;
                            break;
                        }
                        try {
                            values.push_back(std::stoi(op));
                        } catch (...) {
                            allConst = false;
                            break;
                        }
                    }
                    
                    if (allConst && values.size() == 2) {
                        int result;
                        switch (instr->opcode) {
                            case IROpcode::ADD: result = values[0] + values[1]; break;
                            case IROpcode::SUB: result = values[0] - values[1]; break;
                            case IROpcode::MUL: result = values[0] * values[1]; break;
                            case IROpcode::DIV: result = values[0] / values[1]; break;
                            default: continue;
                        }
                        instr->opcode = IROpcode::CONST;
                        instr->operands.clear();
                        instr->operands.push_back(std::to_string(result));
                    }
                }
            }
        }
    }
}

void Optimizer::copyPropagation(IRProgram& program) {
    for (auto& func : program.functions) {
        std::unordered_map<std::string, std::string> copyMap;
        
        for (auto& block : func->blocks) {
            std::vector<std::unique_ptr<IRInstruction>> newInstructions;
            
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::LOAD && !instr->result.empty()) {
                    copyMap[instr->result] = instr->operands[0];
                    continue;
                }
                
                for (std::string& op : instr->operands) {
                    auto it = copyMap.find(op);
                    if (it != copyMap.end()) {
                        op = it->second;
                    }
                }
                
                if (instr->opcode == IROpcode::CONST || instr->opcode == IROpcode::ADD ||
                    instr->opcode == IROpcode::SUB || instr->opcode == IROpcode::MUL ||
                    instr->opcode == IROpcode::DIV) {
                    copyMap.erase(instr->result);
                }
                
                newInstructions.push_back(std::move(instr));
            }
            
            block->instructions = std::move(newInstructions);
        }
    }
}

void Optimizer::deadCodeElimination(IRProgram& program) {
    for (auto& func : program.functions) {
        for (auto& block : func->blocks) {
            std::unordered_map<std::string, bool> used;
            
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::RET) {
                    for (const std::string& op : instr->operands) {
                        used[op] = true;
                    }
                }
            }
            
            bool changed = true;
            while (changed) {
                changed = false;
                for (auto& instr : block->instructions) {
                    if (!instr->result.empty() && used.find(instr->result) != used.end()) {
                        for (const std::string& op : instr->operands) {
                            if (op[0] == '%' && used.find(op) == used.end()) {
                                used[op] = true;
                                changed = true;
                            }
                        }
                    }
                }
            }
            
            std::vector<std::unique_ptr<IRInstruction>> newInstructions;
            for (auto& instr : block->instructions) {
                if (instr->result.empty() || used.find(instr->result) != used.end()) {
                    newInstructions.push_back(std::move(instr));
                }
            }
            
            block->instructions = std::move(newInstructions);
        }
    }
}
