

CPPFLAGS += $(addprefix -I,$(wildcard $(ACESSUSERDIR)Libraries/*/include_exp/))
CPPFLAGS += -I$(ACESSUSERDIR)/include/ -DARCHDIR_is_$(ARCHDIR)
CPPFLAGS += -I $(ACESSDIR)/Externals/Output/$(ARCHDIR)/include
CFLAGS += -std=gnu99 -g
LDFLAGS += -L $(ACESSDIR)/Externals/Output/$(ARCHDIR)/lib

CRTI := $(OUTPUTDIR)Libs/crti.o
CRTBEGIN := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
CRT0 := $(OUTPUTDIR)Libs/crt0.o
CRT0S := $(OUTPUTDIR)Libs/crt0S.o
CRTEND := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN := $(OUTPUTDIR)Libs/crtn.o
LIBGCC_PATH = $(shell $(CC) -print-libgcc-file-name)
