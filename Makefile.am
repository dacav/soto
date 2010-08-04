ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = bootstrap
SUBDIRS = src
.PHONY : doc

doc:
	cat doc/Doxyfile | doxygen -

report:
	(cat doc/Doxyfile ;\
	 echo "GENERATE_HTML=NO" ;\
	 echo "GENERATE_LATEX=YES" ;\
	 echo "INPUT=src/headers/doc.h" ;\
	) | doxygen -

