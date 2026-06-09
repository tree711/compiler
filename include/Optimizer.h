#pragma once

#include "IR.h"

class Optimizer {
private:
    void constantFolding(FunctionIR& func);
    void deadCodeElimination(FunctionIR& func);
    void copyPropagation(FunctionIR& func);
    void optimizeFunction(FunctionIR& func);

public:
    void optimize(IRProgram& program);
};
