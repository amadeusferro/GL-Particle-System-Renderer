CXX = g++

CXXFLAGS = -std=c++11 -Wall -Wextra -O3

SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = -Iinclude -I/usr/include/GLFW -I/usr/include/GL -I/usr/include/glm

LIBS = -lglfw -lGLEW -lGL -lGLU -lX11 -lpthread -lXrandr -lXi -ldl

TARGET = $(BIN_DIR)/myProgram

SRCS = $(wildcard $(SRC_DIR)/*.cpp)

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run