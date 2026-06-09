# Semantic 模块接口协作说明

> 负责人：C（语义分析）  
> 分支：`feature/semantic`  
> 最后更新：2026-06-09

---

## 一、模块职责

语义分析位于 Parser 与 IRGenerator 之间：

```
Lexer → Parser → Semantic → IR → Optimizer → Assembly
```

| 输入 | 输出 |
|------|------|
| AST 根节点 `ProgramNode*` | 成功：`Semantic Pass`；失败：`std::runtime_error` |

已实现功能（对应 `1.md` 任务 C）：

| 任务 | 实现 |
|------|------|
| 符号表 | `SymbolTable` + `Symbol` |
| 变量重定义检查 | 同名变量再次声明时报错 |
| 未声明检查 | 使用未声明变量时报错 |
| 作用域管理 | 块语句 `{ }` 进入/退出作用域 |
| 类型检查 | 当前仅支持 `int` |

---

## 二、本模块拥有的文件

```
include/Semantic.h
include/Symbol.h          ← 语义模块重写，与 backend 对齐
src/Semantic.cpp
tests/semantic_*.tc
src/semantic_test_main.cpp
Makefile                  ← 语义模块本地测试用
.gitignore
SEMANTIC_接口协作说明.md
BRANCH_接口差异对照.md    ← 各分支差异详情
```

以下文件**仅用于本地集成测试**，合并时以各负责人分支为准：

```
include/Lexer.h
include/Parser.h
src/Lexer.cpp             ← 临时借用 backend 版本
src/Parser.cpp            ← 临时借用 backend 版本
```

---

## 三、修改前必须通知 C 的文件

```
include/AST.h
include/Token.h
include/Semantic.h
include/Symbol.h
src/Parser.cpp
src/Semantic.cpp
Makefile
```

### 特别敏感的变化

1. **AST 节点** — 增删 `ASTNodeType`、`ProgramNode` 字段划分、赋值节点形态
2. **Semantic 接口** — `SemanticAnalyzer::analyze()` 签名、错误信息关键字
3. **作用域规则** — 当前**禁止变量遮蔽**（含块内重声明），与 backend 汇编符号策略绑定

---

## 四、与 backend 的对齐结论（重要）

本分支语义实现已与 `origin/feature/backend` **逐行对齐**（`Semantic.h`、`Symbol.h`、`Semantic.cpp` 与 backend 一致，`AST.h` 结构一致）。

合并 `feature/semantic` → `feature/backend` 时：

| 文件 | 预期冲突 |
|------|----------|
| `include/Semantic.h` | 无（相同） |
| `include/Symbol.h` | 无（相同） |
| `src/Semantic.cpp` | 极小（仅注释差异） |
| `include/AST.h` | 无（结构相同） |
| `tests/semantic_*.tc` | 无（backend 已有相同用例） |

**backend 同学（D）无需修改 IR/CodeGen**，但合并前请确认未改动上述公共头文件。

---

## 五、各分支接口差异摘要

> 完整对照见 [`BRANCH_接口差异对照.md`](BRANCH_接口差异对照.md)

### 5.1 Token / Lexer（B: `feature/lexer` ⚠️ 差异最大）

| 项目 | B: `feature/lexer` | backend / parser / semantic（集成基准） |
|------|---------------------|----------------------------------------|
| 头文件位置 | 根目录 `Lexer` 文件，`Lexer.h` **未提交** | `include/Lexer.h` |
| 输入方式 | `Lexer(string source)` 字符串 | `Lexer(string filename)` 读文件 |
| 整数 Token | `TokenType::NUM` | `TokenType::INTEGER` |
| 标识符 Token | `TokenType::ID` | `TokenType::IDENTIFIER` |
| 乘除 Token | `MUL` / `DIV` | `MULTIPLY` / `DIVIDE` |
| 比较 Token | `LT` / `GT` / `EQ` | `LESS` / `GREATER` / `EQUAL` |
| 分号 Token | `SEMI` | `SEMICOLON` |
| 结束 Token | `END` | `END_OF_FILE` |
| Token 字段 | 测试代码用 `lexeme` | `value` |
| 非法字符 | 返回 `ERROR_TOKEN` | 抛出 `runtime_error` |
| 注释 | 支持 `//` 单行注释 | 不支持 |

**合并前 B 必须**：统一 `TokenType` 命名、补齐 `include/Lexer.h`、Token 字段统一为 `value`、与 `include/Token.h` 对齐。

### 5.2 AST（A: `feature/parser` ✅ 已对齐）

| 项目 | 旧 `main` / 初始 stub | 当前约定（parser / backend / semantic） |
|------|----------------------|----------------------------------------|
| 节点类型 | 含 FUNCTION、FOR、RETURN 等 | 仅 TinyC 子集 11 种 |
| `ProgramNode` | `declarations` + `functions` | `declarations` + `statements` |
| `NumberNode` | `variant<int,double>` + `isFloat` | `int value` |

**合并前 A 无需改 AST 结构**；parser 分支 `include/AST.h` 与 backend 仅空白差异。

### 5.3 Symbol（C 已重写 ⚠️ parser 分支仍是旧 stub）

| 项目 | parser / main 旧版 | semantic / backend 新版 |
|------|-------------------|------------------------|
| `SymbolType` | INTEGER, FLOAT, VOID, FUNCTION | 仅 `INTEGER` |
| 符号存储 | `map<string, Symbol*>` 裸指针 | `unordered_map<string, Symbol>` 值语义 |
| 插入 API | `insert(Symbol*)` | `declare(name, type, line, column)` |
| 查找返回 | `Symbol*` 非 const | `const Symbol*` |
| `Symbol` 字段 | 无 `column` | 有 `column` |
| 函数符号 | `FunctionSymbol` 结构体 | 已删除（TinyC 无函数） |

**合并 parser 时**：以 `feature/semantic` 或 `feature/backend` 的 `Symbol.h` 为准，丢弃 parser 旧 stub。

### 5.4 Parser（A: `feature/parser` ⚠️ 风格差异）

| 项目 | `feature/parser` | `feature/backend`（集成基准） |
|------|-----------------|------------------------------|
| 头文件 | 无，类写在 `.cpp` 内 | `include/Parser.h` |
| 错误恢复 | 有 `synchronize()` + try/catch | 无，直接抛异常 |
| AST 产出 | 相同 | 相同 |

**合并建议**：以 backend 的 `Parser.h` + `Parser.cpp` 为准；A 的错误恢复逻辑可作为后续增强单独 PR。

### 5.5 Semantic（C 已完成 ✅）

| 分支 | 状态 |
|------|------|
| `feature/parser` | 旧 stub，基于废弃 AST/Symbol，不可用 |
| `feature/backend` | 完整实现 |
| `feature/semantic` | 完整实现，与 backend 一致 |

### 5.6 Backend 对 Semantic 的隐含假设（D 必读）

`IRGenerator` 在 Semantic Pass 之后假设：

1. `ProgramNode` 含 `declarations`（顶层 `int` 声明）和 `statements`（顶层语句）
2. 所有变量名全局唯一（**不允许 shadow**），因为 `STORE` 直接用源码变量名
3. 表达式运算符仅：`+ - * / == < >` 及一元 `-`
4. 赋值必须是 `AssignmentNode`，不是裸 `IdentifierNode`

若 Semantic 改为允许 shadow，D 须同步修改 `CodeGenerator` 的符号命名策略。

---

## 六、约定接口速查

### AST（Parser 产出 → Semantic 消费 → IR 消费）

```cpp
ProgramNode { declarations, statements }
DeclarationNode { type="int", name, initializer? }
AssignmentNode  { name, value }
IfStatementNode { condition, thenBranch, elseBranch? }
WhileStatementNode { condition, body }
BlockNode       { statements }
BinaryOpNode    { op, left, right }   // + - * / == < >
UnaryOpNode     { op, operand }       // -
NumberNode      { value }
IdentifierNode  { name }
```

### Semantic 对外接口

```cpp
class SemanticAnalyzer {
public:
    void analyze(ASTNode* root);  // root 必须是 ProgramNode*
};
```

### Symbol 接口

```cpp
enum class SymbolType { INTEGER };

struct Symbol {
    std::string name;
    SymbolType type;
    int scope;    // 0 = 全局
    int line;
    int column;
};

class SymbolTable {
    void enterScope();
    void exitScope();
    bool declare(name, type, line, column);  // 全局唯一名
    const Symbol* lookup(name) const;
};
```

### 错误信息格式

```
Variable 'xxx' is not declared at line L, column C
Variable 'xxx' is already declared at line L, column C
Unsupported type 'xxx' at line L, column C
Unsupported binary operator 'xxx' at line L, column C
```

---

## 七、推荐合并顺序

```
feature/lexer  ──→  统一 Token.h / Lexer.h
       ↓
feature/parser ──→  确认 AST.h 不变，采用 backend Parser 风格
       ↓
feature/semantic ──→  Semantic.h / Symbol.h / Semantic.cpp（本分支）
       ↓
feature/backend ──→  完整流水线集成测试
       ↓
main
```

---

## 八、本地测试

```bash
make test                              # 6 个语义回归用例
./semantic_test tests/semantic_valid.tc   # 输出 Semantic Pass
```

| 测试文件 | 预期 |
|----------|------|
| `semantic_valid.tc` | 通过 |
| `semantic_undefined.tc` | 未声明 |
| `semantic_duplicate.tc` | 重复声明 |
| `semantic_scope.tc` | 块外使用局部变量 |
| `semantic_self_init.tc` | 自引用初始化 |
| `semantic_shadow.tc` | 块内重名（禁止 shadow） |

---

## 九、给各同学的提醒

### 给后端同学（D）

- `feature/backend` 请勿被直接覆盖；需要 Semantic 代码请 merge `feature/semantic`
- 修改 `AST.h` / `Symbol.h` / `Semantic.h` 前请先通知 C
- 变量名直接用于汇编，与 Semantic 的 no-shadow 规则绑定

### 给 Parser 同学（A）

- AST 结构已与 backend 对齐，请勿引入 FUNCTION/FOR 等扩展节点（除非全员协商）
- 合并时采用 `include/Parser.h`，Semantic 依赖稳定的 AST 产出

### 给 Lexer 同学（B）

- **`feature/lexer` 与集成基准 Token 命名不一致，是当前最大合并风险**
- 请按 `BRANCH_接口差异对照.md` 第五节对齐后再合并
- `Token.h` 变更务必提前通知 A 和 C
