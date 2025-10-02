# file : makefile

LIBNAME = lexerlib# replace
TARGET := a.out
STATIC_LIB := $(LIBNAME).a

# directories
SRC_DIR := src
INC_DIR := includes

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/$(LIBNAME)
OBJ_DIR := $(BUILD_DIR)/obj

# Compiler / Compiler flags 
CXX := g++
OPTIMIZE = O0
CXXFLAGS := -Wall -Wextra -g -I$(INC_DIR) -MMD -MP -std=c++20 -$(OPTIMIZE)
ASAN := -fsanitize=address,leak -g -fno-omit-frame-pointer

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

# build executable
exe: $(TARGET)

# build static library
lib: $(STATIC_LIB)

# run the executable
run:
	$(BIN_DIR)/a.out
r:
	$(BIN_DIR)/a.out

# clean the directory 
clean:
	rm -rf $(BUILD_DIR)

c:
	rm -rf $(BUILD_DIR)

$(TARGET): $(BIN_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) $(ASAN) $(OBJS) -o $(BIN_DIR)/$(TARGET)

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

-include $(OBJS:.o=.d)
