# TinyC 编译器项目 - 团队协作指南

## 📋 团队分工与分支对应

| 成员 | 分支名称 | 负责模块 | 工作量 |
|------|----------|----------|--------|
| **A（组长）** | `feature/parser` | Parser（语法分析） | 30% |
| **B** | `feature/lexer` | Lexer（词法分析） | 20% |
| **C** | `feature/semantic` | Semantic（语义分析） | 25% |
| **D** | `feature/backend` | IRGenerator、Optimizer、CodeGenerator | 25% |

---

## 🚀 第一次使用（必读）

### 1. 克隆仓库
```bash
git clone https://github.com/tree711/compiler.git
cd compiler
```

### 2. 切换到自己的分支
```bash
# A（组长）
git checkout feature/parser

# B
git checkout feature/lexer

# C
git checkout feature/semantic

# D
git checkout feature/backend
```

### 3. 确认当前分支
```bash
git branch
# 应该显示 * feature/xxx （你负责的分支）
```

---

## 📅 每日开发流程

### 开始工作前（重要！）
```bash
# 1. 切换到主分支
git checkout main

# 2. 拉取最新代码
git pull origin main

# 3. 切换回自己的分支
git checkout feature/xxx

# 4. 合并主分支的最新代码
git merge main

# 5. 开始开发...
```

### 开发过程中
```bash
# 修改代码后，查看状态
git status

# 添加修改的文件
git add .

# 提交修改
git commit -m "描述你的修改"
```

### 提交信息规范
```
feat: 添加词法分析器关键字识别
fix: 修复语法错误处理
docs: 更新注释
refactor: 重构代码结构
test: 添加测试用例
```

### 推送到远程
```bash
git push origin feature/xxx
```

---

## ⚠️ 重要注意事项

### ❌ 禁止的操作
- **不要直接在 `main` 分支上修改代码！**
- **不要强制推送（git push -f）！**
- **不要删除别人的分支！**

### ✅ 推荐的操作
- **每天开始前先 `git pull origin main`**
- **在自己的分支上开发**
- **定期提交代码（每天至少一次）**
- **遇到冲突时及时沟通**

---

## 🔧 常见问题处理

### 问题1：推送时提示冲突
```bash
# 解决方法
git pull origin main
git merge main
# 手动解决冲突后
git add .
git commit -m "解决冲突"
git push origin feature/xxx
```

### 问题2：误在 main 分支上修改了
```bash
# 撤销修改
git checkout -- <文件名>
# 或者
git reset --hard HEAD
```

### 问题3：想查看别人的代码
```bash
# 切换到别人的分支查看
git checkout feature/lexer

# 查看完后切回自己的分支
git checkout feature/xxx
```

---

## 📝 完成开发后的合并流程

### 方式一：通过 Pull Request（推荐）
1. 访问 GitHub 仓库页面
2. 点击 "Compare & pull request"
3. 填写 PR 描述
4. 等待代码审查通过
5. 合并到主分支

### 方式二：直接合并
```bash
# 1. 切换到主分支
git checkout main

# 2. 拉取最新代码
git pull origin main

# 3. 合并你的分支
git merge feature/xxx

# 4. 推送到远程
git push origin main
```

---

## 📊 项目结构

```
compiler/
├── include/           # 头文件
│   ├── Token.h       # Token 定义（B 负责）
│   ├── AST.h         # AST 节点定义（A 负责）
│   ├── Symbol.h      # 符号表定义（C 负责）
│   └── IR.h          # 中间代码定义（D 负责）
├── src/              # 源文件
│   ├── Lexer.cpp     # 词法分析器（B 负责）
│   ├── Parser.cpp    # 语法分析器（A 负责）
│   ├── Semantic.cpp  # 语义分析器（C 负责）
│   ├── IRGenerator.cpp  # IR 生成器（D 负责）
│   ├── Optimizer.cpp    # 优化器（D 负责）
│   └── CodeGenerator.cpp # 代码生成器（D 负责）
└── 1.md             # 项目文档
```

---

## 🎯 各模块开发重点

### A - Parser（语法分析）
- 设计文法规则
- 构建抽象语法树（AST）
- 实现递归下降分析器
- 语法错误恢复机制

### B - Lexer（词法分析）
- 设计 Token 类型
- 实现扫描器
- 维护行号和列号
- 词法错误处理

### C - Semantic（语义分析）
- 实现符号表
- 变量重定义检查
- 未声明变量检查
- 作用域管理
- 类型检查

### D - Backend（后端）
- **IRGenerator**：生成三地址码
- **Optimizer**：常量折叠、死代码删除
- **CodeGenerator**：生成伪汇编代码

---

## 📞 联系方式

如有问题，请在团队群内沟通：
- 代码冲突
- 接口设计问题
- 技术难题

---

## 📅 时间节点

- **Week 1**：完成各自模块的核心功能
- **Week 2**：模块测试与接口对接
- **Week 3**：集成测试与优化
- **Week 4**：文档完善与演示准备

---

**祝开发顺利！** 🎉

有任何问题及时沟通，不要自己闷头解决！