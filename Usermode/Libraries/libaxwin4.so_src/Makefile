# Acess2 AxWin4 Library
# Makefile

-include ../Makefile.cfg

AXWIN4DIR := ../../Applications/axwin4_src/

CPPFLAGS += -I$(AXWIN4DIR)Common/include/
CFLAGS   += -Wextra
CXXFLAGS +=
ASFLAGS  +=
LDFLAGS  += -soname libaxwin4.so -Map map.txt
LIBS  += -lc -lc++

OBJ  = main.o ipc.o ipc_acessipcpipe.o
OBJ += wm.o window_drawing.o
OBJ += Common__serialisation.o
BIN = libaxwin4.so

include ../Makefile.tpl

$(_OBJPREFIX)Common__%.o: $(AXWIN4DIR)/Common/%.cpp
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -MQ $@ -MP -MD -MF $(@:%=%.dep)




