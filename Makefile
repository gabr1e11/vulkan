#
# Roberto Sosa Cano
# Copyright 2016
#

UNAME=$(shell uname)
CXX=g++
GLSL=glslangValidator
OBJDIR=obj
GLSLDIR=glsl

VULKAN_SDK_INCLUDE=$(VULKAN_SDK_PATH)/include/
VULKAN_SDK_LIB=$(VULKAN_SDK_PATH)/lib

#
#Files to be compiled
#
VPATH=. $(GLSLDIR)

FILES=VulkanEngine.cpp
OBJECTS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(FILES))

SHADERS=triangle.vert triangle.frag
SHADER_OBJECTS_TMP=$(patsubst %.vert,$(GLSLDIR)/%.vert.spv,$(SHADERS))
SHADER_OBJECTS=$(patsubst %.frag,$(GLSLDIR)/%.frag.spv,$(SHADER_OBJECTS_TMP))

TUTORIAL=tutorial.cpp
OBJECTS_TUTORIAL=$(patsubst %.cpp,$(OBJDIR)/%.o,$(TUTORIAL))

CXXFLAGS= -Werror -MMD -O0 -g -I $(VULKAN_SDK_INCLUDE) -I . -std=c++14
LDFLAGS+= -L $(VULKAN_SDK_LIB) `pkg-config --static --libs glfw3` -lvulkan

#
# Main rules
#
.PHONY: release

all: vulkan

tutorial: dirs $(OBJECTS_TUTORIAL)
	@echo "- Generating $@...\c"
	@$(CXX) -o $@ $(OBJECTS_TUTORIAL) $(LDFLAGS)
	@echo "done"

release:
	$(MAKE) clean
	$(MAKE) all

vulkan: dirs $(OBJECTS) $(SHADER_OBJECTS)
	@echo "- Generating $@...\c"
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "done"

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: %.cpp
	@echo "- Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(GLSLDIR)/%.vert.spv: %.vert
	@echo "- Compiling $<..."
	@$(GLSL) -V -o $@ $< > /dev/null

$(GLSLDIR)/%.frag.spv: %.frag
	@echo "- Compiling $<..."
	@$(GLSL) -V -o $@ $< > /dev/null

dirs:
	@mkdir -p $(OBJDIR)

clean:
	@echo "- Cleaning project directories...\c"
	@rm -fr $(OBJDIR) vulkan tutorial
	@rm -f $(GLSLDIR)/*.spv
	@echo "done"
