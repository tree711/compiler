#include "../include/Optimizer.h"
#include <unordered_map>
#include <algorithm>

void Optimizer::optimize(IRProgram& program) {
    for (auto& func : program.functions) {
        optimizeFunction(*func);
    }
}

void Optimizer::optimizeFunction(FunctionIR& func) {
    constantFolding(func);
    copyPropagation(func);
    deadCodeElimination(func);
}

void Optimizer::constantFolding(FunctionIR& func) {
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            bool isArith = instr->opcode == IROpcode::ADD || instr->opcode == IROpcode::SUB ||
                           instr->opcode == IROpcode::MUL || instr->opcode == IROpcode::DIV;
            bool isCmp   = instr->opcode == IROpcode::CMP_EQ || instr->opcode == IROpcode::CMP_NE ||
                           instr->opcode == IROpcode::CMP_LT || instr->opcode == IROpcode::CMP_GT ||
                           instr->opcode == IROpcode::CMP_LE || instr->opcode == IROpcode::CMP_GE;

            if (isArith || isCmp) {
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
                    if (isArith) {
                        switch (instr->opcode) {
                            case IROpcode::ADD: result = values[0] + values[1]; break;
                            case IROpcode::SUB: result = values[0] - values[1]; break;
                            case IROpcode::MUL: result = values[0] * values[1]; break;
                            case IROpcode::DIV: result = values[0] / values[1]; break;
                            default: continue;
                        }
                    } else {
                        switch (instr->opcode) {
                            case IROpcode::CMP_EQ: result = values[0] == values[1] ? 1 : 0; break;
                            case IROpcode::CMP_NE: result = values[0] != values[1] ? 1 : 0; break;
                            case IROpcode::CMP_LT: result = values[0] <  values[1] ? 1 : 0; break;
                            case IROpcode::CMP_GT: result = values[0] >  values[1] ? 1 : 0; break;
                            case IROpcode::CMP_LE: result = values[0] <= values[1] ? 1 : 0; break;
                            case IROpcode::CMP_GE: result = values[0] >= values[1] ? 1 : 0; break;
                            default: continue;
                        }
                    }
                    instr->opcode = IROpcode::CONST;
                    instr->operands.clear();
                    instr->operands.push_back(std::to_string(result));
                }
            }
        }
    }
}

void Optimizer::copyPropagation(FunctionIR& func) {
    for (auto& block : func.blocks) {
        std::unordered_map<std::string, std::string> copyMap;
        std::vector<std::unique_ptr<IRInstruction>> newInstructions;
        
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::LOAD && !instr->result.empty()) {
                copyMap[instr->result] = instr->operands[0];
                newInstructions.push_back(std::move(instr));
                continue;
            }
            
            // Replace operands with their propagated values
            for (std::string& op : instr->operands) {
                auto it = copyMap.find(op);
                if (it != copyMap.end()) {
                    op = it->second;
                }
            }
            
            // If this instruction defines a new value, invalidate old mapping for its result
            bool definesNewValue = instr->opcode == IROpcode::CONST ||
                                   instr->opcode == IROpcode::ADD ||
                                   instr->opcode == IROpcode::SUB ||
                                   instr->opcode == IROpcode::MUL ||
                                   instr->opcode == IROpcode::DIV ||
                                   instr->opcode == IROpcode::CMP_EQ ||
                                   instr->opcode == IROpcode::CMP_NE ||
                                   instr->opcode == IROpcode::CMP_LT ||
                                   instr->opcode == IROpcode::CMP_GT ||
                                   instr->opcode == IROpcode::CMP_LE ||
                                   instr->opcode == IROpcode::CMP_GE;
            if (definesNewValue) {
                copyMap.erase(instr->result);
            }

            if (instr->opcode == IROpcode::STORE) {
                const std::string storedVariable = instr->result;
                for (auto it = copyMap.begin(); it != copyMap.end(); ) {
                    if (it->second == storedVariable) {
                        it = copyMap.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            
            newInstructions.push_back(std::move(instr));
        }
        
        block->instructions = std::move(newInstructions);
    }
}

void Optimizer::deadCodeElimination(FunctionIR& func) {
    // Collect all temporaries that are used across all blocks
    std::unordered_map<std::string, bool> used;
    
    // First pass: mark all temporaries that appear as operands (i.e., are used)
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            // STORE has side effects (writing to variables) — always keep
            // BRANCH and RET are control flow — always keep
            if (instr->opcode == IROpcode::STORE ||
                instr->opcode == IROpcode::BRANCH ||
                instr->opcode == IROpcode::BRANCH_EQ ||
                instr->opcode == IROpcode::BRANCH_NE ||
                instr->opcode == IROpcode::BRANCH_LT ||
                instr->opcode == IROpcode::BRANCH_GT ||
                instr->opcode == IROpcode::BRANCH_LE ||
                instr->opcode == IROpcode::BRANCH_GE ||
                instr->opcode == IROpcode::RET) {
                // These always survive, and their operands are "used"
                for (const std::string& op : instr->operands) {
                    if (!op.empty() && op[0] == '%') {
                        used[op] = true;
                    }
                }
            }
        }
    }
    
    // Iteratively mark temps needed by other used temps
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& block : func.blocks) {
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
    }
    
    // Remove dead instructions in each block
    for (auto& block : func.blocks) {
        std::vector<std::unique_ptr<IRInstruction>> newInstructions;
        for (auto& instr : block->instructions) {
            bool hasSideEffect = instr->opcode == IROpcode::STORE ||
                                 instr->opcode == IROpcode::BRANCH ||
                                 instr->opcode == IROpcode::BRANCH_EQ ||
                                 instr->opcode == IROpcode::BRANCH_NE ||
                                 instr->opcode == IROpcode::BRANCH_LT ||
                                 instr->opcode == IROpcode::BRANCH_GT ||
                                 instr->opcode == IROpcode::BRANCH_LE ||
                                 instr->opcode == IROpcode::BRANCH_GE ||
                                 instr->opcode == IROpcode::RET;
            
            bool isUsed = hasSideEffect || 
                          instr->result.empty() || 
                          used.find(instr->result) != used.end();
            
            if (isUsed) {
                newInstructions.push_back(std::move(instr));
            }
        }
        block->instructions = std::move(newInstructions);
    }
}
