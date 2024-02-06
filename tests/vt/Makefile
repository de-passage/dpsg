INCLUDE_FLAGS = -I../../cpp/

CXX ?= g++

BUILD_DIR = build

SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.cpp)

SRC_DEPS = $(SRC:%.cpp=$(BUILD_DIR)/%.d)

# Rewrite the following line using the correct syntax to read the file
ALL_CXX_FLAGS = $(shell cat compile_flags.txt) $(CXXFLAGS)

OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)

TARGET = main
EXE = $(BUILD_DIR)/$(TARGET)

.PHONY: all clean run
all: $(EXE)

run: $(EXE)
	@$(EXE)

$(EXE): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) -g3 -gdwarf-4 $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -g3 -gdwarf-4 $(ALL_CXX_FLAGS) $(INCLUDE_FLAGS) -c -o $@ $<

$(BUILD_DIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(ALL_CXX_FLAGS) -MM -MT $(@:%.d=%.o) $(INCLUDE_FLAGS) $< > $@

clean:
	rm -rf $(BUILD_DIR)

-include $(SRC_DEPS)
