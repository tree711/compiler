CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
INCLUDES = -Iinclude
SRCDIR = src
BUILDDIR = build

SEMANTIC_SRCS = $(SRCDIR)/Semantic.cpp \
                $(SRCDIR)/Lexer.cpp \
                $(SRCDIR)/Parser.cpp \
                $(SRCDIR)/semantic_test_main.cpp

SEMANTIC_OBJS = $(SEMANTIC_SRCS:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
SEMANTIC_TARGET = semantic_test

.PHONY: all test clean

all: $(SEMANTIC_TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(SEMANTIC_TARGET): $(SEMANTIC_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

test: $(SEMANTIC_TARGET)
	./$(SEMANTIC_TARGET)

clean:
	rm -rf $(BUILDDIR) $(SEMANTIC_TARGET)
