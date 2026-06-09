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
            
            // 使用传播后的值替换操作数
            for (std::string& op : instr->operands) {
                auto it = copyMap.find(op);
                if (it != copyMap.end()) {
                    op = it->second;
                }
            }
            
            // 指令产生新值时，使该结果原有的传播映射失效
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
    // 收集所有基本块中被使用的临时变量
    std::unordered_map<std::string, bool> used;
    
    // 第一遍扫描：标记所有作为操作数出现的临时变量
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            // STORE 会写入变量，具有副作用，必须保留
            // BRANCH 和 RET 属于控制流指令，必须保留
            if (instr->opcode == IROpcode::STORE ||
                instr->opcode == IROpcode::BRANCH ||
                instr->opcode == IROpcode::BRANCH_EQ ||
                instr->opcode == IROpcode::BRANCH_NE ||
                instr->opcode == IROpcode::BRANCH_LT ||
                instr->opcode == IROpcode::BRANCH_GT ||
                instr->opcode == IROpcode::BRANCH_LE ||
                instr->opcode == IROpcode::BRANCH_GE ||
                instr->opcode == IROpcode::RET) {
                // 这些指令必须保留，其操作数也应视为已使用
                for (const std::string& op : instr->operands) {
                    if (!op.empty() && op[0] == '%') {
                        used[op] = true;
                    }
                }
            }
        }
    }
    
    // 迭代标记其他已使用临时变量所依赖的值
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
    
    // 删除各基本块中的无效指令
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
