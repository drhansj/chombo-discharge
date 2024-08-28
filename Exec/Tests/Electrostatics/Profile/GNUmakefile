include $(DISCHARGE_HOME)/Lib/Definitions.make

# Things for the Chombo makefile system. 
ebase    = program
include $(CHOMBO_HOME)/mk/Make.example

# For building this application -- it needs the chombo-discharge source code. 
$(ebaseobject): dependencies
.DEFAULT_GOAL=$(ebase)

# Build dependencies if they do not exis. 
dependencies: 
	$(MAKE) --directory=$(DISCHARGE_HOME) discharge-lib
	$(MAKE) --directory=$(DISCHARGE_HOME) electrostatics

# Make advection-diffusion headers and library visible. 
XTRACPPFLAGS += $(ELECTROSTATICS_INCLUDE)
XTRALIBFLAGS += $(addprefix -l, $(ELECTROSTATICS_LIB))$(config)

pristine:
	make realclean
	cd $(CHOMBO_HOME); make realclean

main:
	make realclean
	make -j32 all
	mv $(ebase)*.ex main.exe
