CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall \
             $(shell pkg-config --cflags glfw3)
LDFLAGS  := $(shell pkg-config --libs glfw3) -lGL -lpthread -ldl

SRCS := main.cpp \
        imgui.cpp \
        imgui_draw.cpp \
        imgui_tables.cpp \
        imgui_widgets.cpp \
        imgui_impl_glfw.cpp \
        imgui_impl_opengl3.cpp

TARGET := parallel_matrix

all: $(TARGET)
	@echo ""
	@echo "  Build OK →  ./$(TARGET)"
	@echo ""

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean