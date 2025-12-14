# Определяем операционную систему
UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Пути для macOS (Homebrew)
ifeq ($(UNAME_S),Darwin)
    PG_PATH := $(shell brew --prefix postgresql@15 2>/dev/null || brew --prefix postgresql 2>/dev/null)
    CXXFLAGS += -I$(PG_PATH)/include
    LDFLAGS = -L$(PG_PATH)/lib -lpq
else
    # Пути для Linux
    CXXFLAGS += -I/usr/include/postgresql
    LDFLAGS = -lpq
endif

TARGET = server
SOURCES = server.cpp database.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = database.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run