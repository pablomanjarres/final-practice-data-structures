# Build the C++ DialSort vs. Alternative benchmark
#
# Usage:
#   make            # builds ./bench at -O3
#   make debug      # builds at -O0 -g for debugging
#   make run        # build, then run --quick preset
#   make full       # build, then run --full preset
#   make memwall    # build, then run --memwall preset
#   make clean      # remove build artifacts
#   make verify     # build and run a small correctness sanity sweep

CXX      ?= g++
CXXSTD   ?= -std=c++17
WARN     ?= -Wall -Wextra -Wpedantic
OPT      ?= -O3 -DNDEBUG
INC      ?= -Iinclude

CXXFLAGS ?= $(CXXSTD) $(WARN) $(OPT) $(INC)

SRC := \
  src/dial_sort.cpp \
  src/radix_sort.cpp \
  src/merge_sort.cpp \
  src/distributions.cpp \
  src/stats.cpp \
  src/runner.cpp \
  src/main.cpp

OBJ := $(SRC:.cpp=.o)

TARGET := bench

.PHONY: all debug clean run quick full memwall verify

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: OPT := -O0 -g
debug: clean all

run: $(TARGET)
	./$(TARGET) --quick

quick: $(TARGET)
	./$(TARGET) --quick -o results-quick.csv
	@echo "wrote results-quick.csv"

full: $(TARGET)
	./$(TARGET) --full -o results-full.csv
	@echo "wrote results-full.csv"

memwall: $(TARGET)
	./$(TARGET) --memwall -o results-memwall.csv
	@echo "wrote results-memwall.csv"

verify: $(TARGET)
	./$(TARGET) --algo dial,radix,merge --sizes 10000,50000 \
	            --dists uniform,sorted,reverse,gaussian,sparseDup \
	            --reps 2 --warmup 1 --no-progress

clean:
	rm -f $(OBJ) $(TARGET) results-*.csv
