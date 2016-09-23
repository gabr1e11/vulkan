#
# Roberto Sosa Cano
# Copyright 2016
#

UNAME=$(shell uname)
CXX=g++
OBJDIR=obj

#
#Files to be compiled
#
VPATH=.

FILES=main.cpp

OBJECTS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(FILES))

CXXFLAGS= -Werror -MMD -O0 -g -I /opt/VulkanSDK/1.0.21.1/x86_64/include -I . -std=c++14
LDFLAGS+= -L /opt/VulkanSDK/1.0.21.1/x86_64/lib -lvulkan -lglfw3 -lX11 -lXrandr -lXinerama -lXi -lXxf86vm -lXcursor -ldl -pthread -fPIC

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
