# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -O2

# Source files
COMMON = common.h
SOURCES = default_behavior.cpp pq_behavior.cpp pq_algorithm.cpp distribution_generator.cpp

# Output binaries
DEFAULT_BINARY = default_behavior
PQ_BINARY = pq_behavior
GENERATOR_BINARY = distribution_generator

# Data files
DATA_DIR = data
UNIFORM_CSV = $(DATA_DIR)/uniform.csv
ZIPFIAN_CSV = $(DATA_DIR)/zipfian.csv
LATEST_CSV = $(DATA_DIR)/latest.csv
HOTSPOT_CSV = $(DATA_DIR)/hotspot.csv

# Default target
.PHONY: all
all: default pq distributions

default: $(DEFAULT_BINARY)
$(DEFAULT_BINARY): default_behavior.cpp $(COMMON)
	$(CXX) $(CXXFLAGS) -o $(DEFAULT_BINARY) default_behavior.cpp

pq: $(PQ_BINARY)
$(PQ_BINARY): pq_behavior.cpp pq_algorithm.cpp $(COMMON)
	$(CXX) $(CXXFLAGS) -o $(PQ_BINARY) pq_behavior.cpp pq_algorithm.cpp

generator: $(GENERATOR_BINARY)
$(GENERATOR_BINARY): distribution_generator.cpp
	$(CXX) $(CXXFLAGS) -o $(GENERATOR_BINARY) distribution_generator.cpp

distributions: $(UNIFORM_CSV) $(ZIPFIAN_CSV) $(LATEST_CSV) $(HOTSPOT_CSV)

$(UNIFORM_CSV): $(GENERATOR_BINARY)
	@mkdir -p $(DATA_DIR)
	@if [ ! -f $(UNIFORM_CSV) ]; then \
	    ./$(GENERATOR_BINARY) uniform $(UNIFORM_CSV); \
	else \
	    echo "Skipping generation: $(UNIFORM_CSV) already exists."; \
	fi

$(ZIPFIAN_CSV): $(GENERATOR_BINARY)
	@mkdir -p $(DATA_DIR)
	@if [ ! -f $(ZIPFIAN_CSV) ]; then \
	    ./$(GENERATOR_BINARY) zipfian $(ZIPFIAN_CSV); \
	else \
	    echo "Skipping generation: $(ZIPFIAN_CSV) already exists."; \
	fi

$(LATEST_CSV): $(GENERATOR_BINARY)
	@mkdir -p $(DATA_DIR)
	@if [ ! -f $(LATEST_CSV) ]; then \
	    ./$(GENERATOR_BINARY) latest $(LATEST_CSV); \
	else \
	    echo "Skipping generation: $(LATEST_CSV) already exists."; \
	fi

$(HOTSPOT_CSV): $(GENERATOR_BINARY)
	@mkdir -p $(DATA_DIR)
	@if [ ! -f $(HOTSPOT_CSV) ]; then \
	    ./$(GENERATOR_BINARY) hotspot $(HOTSPOT_CSV); \
	else \
	    echo "Skipping generation: $(HOTSPOT_CSV) already exists."; \
	fi

# Run the specified mode and distribution
run: default pq
	@echo "Run with: make run MODE=<default|pq> DISTRIBUTION=<uniform|zipfian|latest|hotspot>"
	@if [ "$(MODE)" = "" ]; then \
	    echo "Error: MODE not specified. Use MODE=default or MODE=pq."; \
	    exit 1; \
	fi
	@if [ "$(DISTRIBUTION)" = "" ]; then \
	    echo "Error: DISTRIBUTION not specified. Use DISTRIBUTION=uniform, zipfian, latest, or hotspot."; \
	    exit 1; \
	fi
	@if [ "$(DISTRIBUTION)" = "uniform" ]; then DATA_FILE=$(UNIFORM_CSV); \
	elif [ "$(DISTRIBUTION)" = "zipfian" ]; then DATA_FILE=$(ZIPFIAN_CSV); \
	elif [ "$(DISTRIBUTION)" = "latest" ]; then DATA_FILE=$(LATEST_CSV); \
	elif [ "$(DISTRIBUTION)" = "hotspot" ]; then DATA_FILE=$(HOTSPOT_CSV); \
	else \
	    echo "Error: Invalid DISTRIBUTION."; \
	    exit 1; \
	fi; \
	if [ "$(MODE)" = "default" ]; then \
	    ./$(DEFAULT_BINARY) $$DATA_FILE; \
	elif [ "$(MODE)" = "pq" ]; then \
	    ./$(PQ_BINARY) $$DATA_FILE; \
	else \
	    echo "Error: Invalid MODE."; \
	    exit 1; \
	fi

clean:
	rm -f $(DEFAULT_BINARY) $(PQ_BINARY) $(GENERATOR_BINARY)

clean_all: clean
	rm -rf $(DATA_DIR)
