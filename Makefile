# Определяем операционную систему
UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Директории
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Пути для macOS (Homebrew)
ifeq ($(UNAME_S),Darwin)
    PG_PATH := $(shell brew --prefix postgresql@15 2>/dev/null || brew --prefix postgresql 2>/dev/null)
    CXXFLAGS += -I$(PG_PATH)/include -I$(INCLUDE_DIR)
    LDFLAGS = -L$(PG_PATH)/lib -lpq
else
    # Пути для Linux
    CXXFLAGS += -I/usr/include/postgresql -I$(INCLUDE_DIR)
    LDFLAGS = -lpq
endif

TARGET = $(BUILD_DIR)/server
SOURCES = $(SRC_DIR)/server.cpp $(SRC_DIR)/database.cpp
OBJECTS = $(BUILD_DIR)/server.o $(BUILD_DIR)/database.o
HEADERS = $(INCLUDE_DIR)/database.h

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/server.o: $(SRC_DIR)/server.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/database.o: $(SRC_DIR)/database.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run