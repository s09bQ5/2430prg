SUBDIRS = src po

g2430prgdocdir = ${prefix}/doc/g2430prg
g2430prgdoc_DATA = \
		    README\
		    COPYING\
		    AUTHORS\
		    ChangeLog\
		    INSTALL\
		    NEWS
		    
EXTRA_DIST = $(g2430prgdoc_DATA)

dist-hook:
	    for specfile in *.spec; do \
		if test -f $$specfile; then \
		    cp -p $$specfile $(distdir); \
		fi \
	    done
