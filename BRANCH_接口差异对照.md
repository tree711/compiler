# 各分支接口差异对照表

> 文档维护：C（语义分析）  
> 对比基准：`origin/feature/backend`（当前唯一可跑通完整流水线的分支）  
> 生成日期：2026-06-09

本文档记录仓库各 feature 分支之间**公共接口的实际差异**，供合并前评审。对比方法：`git fetch origin` 后逐文件 diff。

---

## 分支文件清单

| 分支 | 关键文件 | 流水线状态 |
|------|----------|------------|
| `main` | 初始 stub，AST 含函数/循环扩展 | 不可编译集成 |
| `feature/lexer` | `Lexer`（根目录单文件），`Token.h` | Lexer 独立可测，**Token 命名未对齐** |
| `feature/parser` | `AST.h` ✅, `Parser.cpp`, 旧 `Symbol.h` / `Semantic.cpp` stub | Parser+AST 可用，Semantic 不可用 |
| `feature/semantic` | 完整 Semantic + 对齐 AST/Symbol + 测试 | Semantic 可测（借用 backend Lexer/Parser） |
| `feature/backend` | 完整编译器 + Makefile + tests | **全流水线可跑** |

---

## 1. `include/Token.h`

### 现状

`feature/lexer`、`feature/parser`、`feature/backend`、`feature/semantic` 的 `Token.h` **文本内容相同**（backend 标准版）。

### 隐藏问题：`feature/lexer` 实现与 `Token.h` 不一致

lexer 分支根目录 `Lexer` 文件的实现**并未使用** `include/Token.h` 中的枚举名：

| Token.h（头文件约定） | Lexer 实现实际使用 | 影响 |
|----------------------|-------------------|------|
| `INTEGER` | `NUM` | Parser 无法识别整数 |
| `IDENTIFIER` | `ID` | Parser 无法识别标识符 |
| `MULTIPLY` | `MUL` | 乘法解析失败 |
| `DIVIDE` | `DIV` | 除法解析失败 |
| `LESS` | `LT` | 关系运算失败 |
| `GREATER` | `GT` | 关系运算失败 |
| `EQUAL` | `EQ` | 相等比较失败 |
| `SEMICOLON` | `SEMI` | 语句终结失败 |
| `END_OF_FILE` | `END` | 主循环无法结束 |

此外 lexer 实现还有：

- Token 测试输出使用 `t.lexeme`，而 `Token.h` 定义为 `value`
- 缺少 `Lexer.h`（`#include "Lexer.h"` 但仓库未提交该头文件）
- 构造函数接收**源代码字符串**，不是文件名

### 合并建议

**以 `include/Token.h`（backend 版）为唯一标准**，B 修改 `Lexer` 实现中的所有 `TokenType` 引用，并提交 `include/Lexer.h`。

---

## 2. `include/Lexer.h` + `src/Lexer.cpp`

| 对比项 | feature/lexer | feature/backend / semantic（测试版） |
|--------|--------------|-------------------------------------|
| 头文件 | ❌ 未提交 | ✅ `include/Lexer.h` |
| 构造参数 | `const string& source` | `const string& filename` |
| 核心 API | `getNextToken()`, `tokenize()` | 相同 |
| 注释跳过 | ✅ `//` | ❌ |
| 词法错误 | 返回 `ERROR_TOKEN` | 抛异常 |
| 浮点数 | ❌ | ❌（Token.h 有 FLOAT 但未实现） |
| 字符串 | ❌ | ❌ |

### 合并建议

1. 保留 backend 的文件输入接口（`main.cpp` 依赖）
2. 可选：合并 B 的 `//` 注释支持（不影响 Token 接口）
3. 词法错误策略需全员统一（抛异常 vs 错误 Token）

---

## 3. `include/AST.h`

### main（旧） vs parser/backend/semantic（新）

```diff
- ASTNodeType: FUNCTION_DEFINITION, FOR_STATEMENT, RETURN_STATEMENT, ...
+ ASTNodeType: 仅 PROGRAM, DECLARATION, ASSIGNMENT, IF, WHILE, BLOCK, BINARY_OP, UNARY_OP, IDENTIFIER, NUMBER

- ProgramNode: declarations + functions
+ ProgramNode: declarations + statements

- NumberNode: variant<int,double> value; bool isFloat;
+ NumberNode: int value;
```

### parser vs backend

**无结构性差异**（仅空格/换行）。

### Semantic / IR 依赖

两者均要求：

- 顶层声明走 `program->declarations`
- 顶层语句走 `program->statements`
- 块内声明作为 `BlockNode->statements` 中的 `DeclarationNode`

---

## 4. `include/Symbol.h`

### parser/main（旧 stub）

```cpp
enum class SymbolType { INTEGER, FLOAT, VOID, FUNCTION };
class SymbolTable {
    bool insert(Symbol* symbol);      // 裸指针
    Symbol* lookup(name);           // 非 const
    // 无 column 字段
};
struct FunctionSymbol : Symbol { ... };
```

### semantic/backend（新，TinyC 专用）

```cpp
enum class SymbolType { INTEGER };
class SymbolTable {
    bool declare(name, type, line, column);
    const Symbol* lookup(name) const;
    // Symbol 含 column 字段
};
// 无 FunctionSymbol
```

### 合并建议

**合并 parser 分支时直接采用 semantic/backend 的 Symbol.h**，旧 stub 应丢弃。

---

## 5. `include/Semantic.h` + `src/Semantic.cpp`

| 分支 | 状态 |
|------|------|
| parser | 旧实现：基于废弃 AST（`ProgramNode->functions`）、旧 Symbol API，**不可用于集成** |
| semantic | ✅ 与 backend 一致 |
| backend | ✅ 参考实现 |

### 作用域规则（semantic = backend）

```cpp
// declare() 内：若 lookup(name) 在任何作用域已存在 → 拒绝
// 这意味着块内 int value; 当外层已有 value 时会报错
```

原因（backend `Semantic.cpp` 注释）：汇编生成直接使用变量名，shadow 会导致符号冲突。

---

## 6. `include/Parser.h` + `src/Parser.cpp`

| 对比项 | feature/parser | feature/backend |
|--------|---------------|-----------------|
| Parser.h | ❌ | ✅ |
| 错误恢复 | `synchronize()` + try/catch | 无 |
| parseProgram | 相同逻辑 | 相同逻辑 |
| 赋值解析 | `parseAssignment` 从 `parseEquality` 开始 | 相同 |
| 支持语法 | int 声明, if/else, while, 块, 表达式语句 | 相同 |

### 已知 Parser 行为（影响 Semantic）

1. 赋值语句产出 `AssignmentNode`（`name` + `value`），Semantic 在 `visitStatement(ASSIGNMENT)` 中检查左侧已声明
2. 表达式语句（如 `a + b;`）产出 `BinaryOpNode`，Semantic 会检查其中标识符
3. `if` 无 else 时 `elseBranch` 为 `nullptr`，Semantic 正确处理

---

## 7. Backend 模块对上游的依赖

### IRGenerator（`feature/backend`）

```cpp
IRProgram IRGenerator::generate(ASTNode* root);
```

假设：

| 假设 | 说明 |
|------|------|
| root 是 ProgramNode | 否则返回空 IRProgram |
| 遍历 declarations 再 statements | 与 Semantic 相同顺序 |
| STORE 使用 `assign->name` | 要求变量名全局唯一 |
| 关系运算返回 0/1 整数 | Semantic 不验证条件类型 |
| 支持 BINARY_OP: `+ - * / == < >` | 与 Semantic 白名单一致 |

### CodeGenerator

- 读取 `IRProgram.globalVariables` 和 `STORE` 指令中的变量名
- **未做符号重命名**，与 Semantic no-shadow 规则耦合

### Optimizer

- 仅依赖 `IR.h`，与 Semantic 无直接接口

---

## 8. 合并冲突预测

| 合并操作 | 高风险文件 | 风险原因 |
|----------|-----------|----------|
| lexer → main | `src/Lexer.cpp`, `include/Token.h` | Token 枚举名不一致 |
| parser → main | `include/AST.h`, `include/Symbol.h`, `src/Parser.cpp` | AST/Symbol 完全换版 |
| semantic → backend | 低 | Semantic/Symbol 已对齐 |
| backend → main | 几乎全部 | main 是旧 stub |

---

## 9. 集成验证清单（合并 main 前）

- [ ] B：`TokenType` 命名与 `Token.h` 一致，`Lexer.h` 已提交
- [ ] A：`ProgramNode` 含 `declarations` + `statements`
- [ ] C：6 个 `tests/semantic_*.tc` 全部通过
- [ ] D：`tests/pipeline.tc` 全流水线通过
- [ ] 全员：`make` 一键编译，`./compiler tests/pipeline.tc output.s` 成功

---

## 10. 参考命令

```bash
# 查看某文件在各分支的差异
git diff origin/feature/lexer:include/Token.h origin/feature/backend:include/Token.h
git diff origin/feature/parser:include/Symbol.h origin/feature/backend:include/Symbol.h
git diff origin/feature/parser:include/AST.h origin/feature/backend:include/AST.h

# 本地跑语义测试
make test
```
