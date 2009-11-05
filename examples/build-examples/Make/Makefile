
INSTALL_DIR=../../install

PARTICLEDB_XML  = -DPARTICLEDB_XML=\"$(INSTALL_DIR)/share/chimp/data/particledb.xml\"

XML2_LIBS       = $(shell xml2-config --libs)
XML2_CFLAGS     = $(shell xml2-config --cflags)

LDLIBS          = -lparticledb -lrandom -lmisc -lboost_regex $(XML2_LIBS)
LDFLAGS         = -L$(INSTALL_DIR)/lib
INCLUDES        = -I$(INSTALL_DIR)/include
CPPFLAGS        = $(INCLUDES) $(PARTICLEDB_XML) $(XML2_CFLAGS)

PROGS = testRuntimeDB

ALL : $(PROGS)

clean :
	rm -f $(PROGS)

