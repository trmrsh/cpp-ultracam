## Process this file with automake to generate Makefile.in
##

EXTRA_DIST = aper.gif docgen.in document.pl Changes.html FAQ.html GettingStarted.html reduce.red aper.gif \
Errors.html Timing.html UserInput.html rxj2130.gif style.css ultracam_guide.pdf sreduce.sre

#	sed -e 's%ZzZzZ%html/api%' Doxyfile.in > Doxyfile
#	doxygen Doxyfile || :

install-data-local: 
	$(INSTALL) -d html
	sed -e 's%ZzZzZ%$(VERSION)%' docgen.in > docgen.pl
	perl docgen.pl  html
	cp style.css     html/.
	cp Changes.html html/.
	cp GettingStarted.html html/.
	cp Errors.html    html/.
	cp Timing.html    html/.
	cp UserInput.html html/.
	cp FAQ.html       html/.
	cp reduce.red     html/.
	cp sreduce.sre    html/.
	cp ultracam_guide.pdf html/.
	cp rxj2130.gif    html/.
	cp aper.gif       html/.
	chmod 0755 html
	@for file in `find html | sed -e 's% %\\\%'`; do \
	nfile=`echo $$file | sed -e 's%\\\%\\ %'`; \
	if test -d "$$nfile"; then \
	  chmod 0755 "$$nfile"; \
	else \
	  chmod 0644 "$$nfile"; \
	fi; \
	done
	-rm -r     $(prefix)/html/$(PACKAGE)
	-mkdir -p  $(prefix)/html
	cp -r html $(prefix)/html/$(PACKAGE)

uninstall-html:
	rm -r $(prefix)/html/$(PACKAGE)

clean-html:
	-rm -r html
