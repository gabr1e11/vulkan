#
# Roberto Sosa Cano
# Copyright 2016
#

UNAME=$(shell uname)
CXX=g++
OBJDIR=obj

VULKAN_SDK_PATH=/opt/VulkanSDK/1.0.21.1/
VULKAN_SDK_INCLUDE=$(VULKAN_SDK_PATH)/x86_64/include/
VULKAN_SDK_LIB=$(VULKAN_SDK_PATH)/x86_64/lib

#
#Files to be compiled
#
VPATH=.

FILES=VulkanEngine.cpp

OBJECTS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(FILES))

CXXFLAGS= -Werror -MMD -O0 -g -I $(VULKAN_SDK_INCLUDE) -I . -std=c++14
LDFLAGS+= -L $(VULKAN_SDK_LIB) `pkg-config --static --libs glfw3` -lvulkan

#
# Main rules
#
.PHONY: release

all: vulkan

release:
	$(MAKE) clean
	$(MAKE) all

vulkan: dirs $(OBJECTS)
	@echo "- Generating $@...\c"
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "done"

-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.o: %.cpp
	@echo "- Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

dirs:
	@mkdir -p $(OBJDIR)

clean:
	@echo "- Cleaning project directories...\c"
	@rm -fr $(OBJDIR) vulkan
	@echo "done"
