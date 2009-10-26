
# Acess2 Module/Driver Templater Makefile
# Makefile.tpl

-include ../../Makefile.cfg

CPPFLAGS = -I../../Kernel/include -I../../Kernel/arch/$(ARCHDIR)/include -DARCH=$(ARCH)
CFLAGS = -Wall -Werror $(CPPFLAGS)

OBJ := $(addsuffix .$(ARCH),$(OBJ))
BIN = ../$(NAME).kmd.$(ARCH)
KOBJ = ../$(NAME).xo.$(ARCH)

DEPFILES  = $(filter %.o.$(ARCH),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(ARCH)=%.d.$(ARCH))

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(KOBJ) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) -T ../link.ld -shared -o $@ $(OBJ)
	@echo --- $(LD) -o ../$(NAME).o.$(ARCH)
	@$(CC) -Wl,-r -nostdlib -o $(KOBJ) $(OBJ)

%.o.$(ARCH): %.c Makefile ../Makefile.tpl ../../Makefile.cfg 
	@echo --- $(CC) -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $*.d.$(ARCH) $<

-include $(DEPFILES)