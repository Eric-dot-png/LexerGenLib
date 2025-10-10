# file : makefile

LIBNAME = lexerlib# replace
EXE := a.out
STATIC_LIB := $(LIBNAME).a

# directories
SRC_DIR := src
INC_DIR := includes

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/$(LIBNAME)
OBJ_DIR := $(BUILD_DIR)/obj
OUT_DIR := output

# Compiler / Compiler flags 
CXX := g++
OPTIMIZE = O0
CXXFLAGS := -Wall -Wextra -g -I$(INC_DIR) -MMD -MP -std=c++20 -$(OPTIMIZE)
ASAN := -fsanitize=address,leak -g -fno-omit-frame-pointer

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# build executable
exe: $(EXE)

# build static library
lib: $(STATIC_LIB)

# run the executable
run: $(OUT_DIR)
	$(BIN_DIR)/a.out
r: $(OUT_DIR)
	$(BIN_DIR)/a.out

log: $(OUT_DIR)
	$(BIN_DIR)/a.out > $(OUT_DIR)/out.log

l: $(OUT_DIR)
	$(BIN_DIR)/a.out > $(OUT_DIR)/out.log

graph: $(OUT_DIR)
	dot -Tsvg -o $(OUT_DIR)/nfa.svg $(OUT_DIR)/nfa.dot
	dot -Tsvg -o $(OUT_DIR)/dfa.svg $(OUT_DIR)/dfa.dot
#dot -Tsvg -o $(OUT_DIR)/dfaMin.svg $(OUT_DIR)/dfaMin.dot

# clean the directory 
clean: 
	rm -rf $(BUILD_DIR)

c:
	rm -rf $(BUILD_DIR)

$(EXE): CXXFLAGS += -DMDEBUG
$(EXE): $(BIN_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) $(ASAN) $(OBJS) -o $(BIN_DIR)/$(EXE)

$(STATIC_LIB): $(LIB_DIR) $(OBJS)
	ar rcs $(LIB_DIR)/lib/$(STATIC_LIB) $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
	cp -r $(INC_DIR)/* $(LIB_DIR)/include/

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# make the object dir if it does not exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# make bin dir if it does not exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# make the lib dir if it does not exist
$(LIB_DIR):
	mkdir -p $(LIB_DIR)
	mkdir -p $(LIB_DIR)/include
	mkdir -p $(LIB_DIR)/lib

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

-include $(OBJS:.o=.d)
