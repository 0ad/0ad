SUBDIRS = src include
DIST_SUBDIRS = src include

dist-hook:
	cp -Rf "msvc"      $(distdir)
	find $(distdir) -name CVS -type d | xargs rm -rf
