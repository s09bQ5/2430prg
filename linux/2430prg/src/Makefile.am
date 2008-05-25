
INCLUDES = \
	    -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	    -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	    -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	    $(2430PRG_CFLAGS)
	    
AM_CFLAGS =\
	    -Wall \
	    -g
	    
bin_PROGRAMS = 2430prg

2430prg_SOURCES = \
		    main.c \
		    parport.c \
		    cc2430.c \
		    driver/ppdev/ppdev.c \
		    driver/io/io.c
		    
2430prg_LDFLAGS = 

2430prg_LDADD = $(2430PRG_LIBS)

