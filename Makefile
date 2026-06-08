CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
INCLUDES = -Iinclude
SRCDIR = src
BUILDDIR = build

# 所有源文件
SRCS = $(SRCDIR)/main.cpp \
       $(SRCDIR)/IRGenerator.cpp \
       $(SRCDIR)/Optimizer.cpp \
       $(SRCDIR)/CodeGenerator.cpp \
       $(SRCDIR)/Lexer.cpp \
       $(SRCDIR)/Parser.cpp \
       $(SRCDIR)/Semantic.cpp

OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
TARGET = compiler

.PHONY: all clean run

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

# 仅编译后端模块（Lexer/Parser/Semantic 可能尚未完成）
backend: $(BUILDDIR)/main.o $(BUILDDIR)/IRGenerator.o $(BUILDDIR)/Optimizer.o $(BUILDDIR)/CodeGenerator.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET) output_test.s output_pipeline.s
