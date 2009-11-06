CHIMP_INSTALL_DIR     = $(realpath ../../../install)

include $(CHIMP_INSTALL_DIR)/share/chimp/Makefile.chimp-project

PROGS = testRuntimeDB

LDLIBS   = $(CHIMP_LDLIBS)
LDFLAGS  = $(CHIMP_LDFLAGS)
CPPFLAGS = $(CHIMP_CPPFLAGS)

ALL : $(PROGS)


clean :
	rm -f $(PROGS)

