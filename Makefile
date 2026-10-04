CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude -Ithird_party/imgui \
             $(shell pkg-config --cflags glfw3)
LDFLAGS  := $(shell pkg-config --libs glfw3) -lGL -lpthread -ldl

SRCS := src/main.cpp \
        src/core.cpp \
        src/worker.cpp \
        src/compute.cpp \
        src/coordinator.cpp \
        src/ui.cpp \
        src/style.cpp \
        src/loop.cpp \
        src/panel_left.cpp \
        src/panel_right.cpp \
        third_party/imgui/imgui.cpp \
        third_party/imgui/imgui_draw.cpp \
        third_party/imgui/imgui_tables.cpp \
        third_party/imgui/imgui_widgets.cpp \
        third_party/imgui/imgui_impl_glfw.cpp \
        third_party/imgui/imgui_impl_opengl3.cpp

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
