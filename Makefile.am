## Process this file with automake to generate Makefile.in

ACLOCAL_AMFLAGS = -I m4 

SUBDIRS = src doc include scripts

export: dist
	scp $(PACKAGE)-$(VERSION).tar.gz $(WEB_SERVE):$(WEB_PATH)/software/.
	ssh $(WEB_SERVE) "cd $(WEB_PATH)/software ; \rm -f $(PACKAGE).tar.gz ; ln -s $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE).tar.gz; \rm -fr $(PACKAGE); mkdir $(PACKAGE)"
	scp -r doc/html $(WEB_SERVE):$(WEB_PATH)/software/$(PACKAGE)/.
	ssh $(WEB_SERVE) "cd $(WEB_PATH)/software; sed -e 's%<\!-- ultracam version --><td>.*</td>%<\!-- ultracam version --><td>$(VERSION)</td>%' index.html > junk.html; mv junk.html index.html"
	ssh $(WEB_SERVE) "cd $(WEB_PATH)/software; sed -e 's%<\!-- ultracam date --><td>.*</td>%<\!-- ultracam date --><td>$(DATE)</td>%' index.html > junk.html; mv junk.html index.html"
	rm $(PACKAGE)-$(VERSION).tar.gz

ALIASES = Ultracam

.PHONY : $(ALIASES)

DATE    = $(shell date)

progdir = @bindir@/@PACKAGE@

prog_DATA = $(ALIASES)
 
$(ALIASES):
	@echo '#' > $(ALIASES)
	@echo '# This file defines aliases for the $(PACKAGE) v$(VERSION) software.' >> $(ALIASES)
	@echo '# Source it to define them.' >> $(ALIASES)
	@echo '#' >> $(ALIASES)
	@echo '# this to allow both bash and csh to work' >> $(ALIASES)
	@echo 'test "$$?BASH_VERSION" = "0" || eval '\''alias() { command alias "$$1=$$2"; }'\' >> $(ALIASES)
	@echo '#' >> $(ALIASES)
	@echo 'alias add       $(progdir)/add'         >> $(ALIASES)
	@echo 'alias addbad    $(progdir)/addbad'      >> $(ALIASES)
	@echo 'alias addfield  $(progdir)/addfield'    >> $(ALIASES)
	@echo 'alias addsky    $(progdir)/addsky'      >> $(ALIASES)
	@echo 'alias addspec   $(progdir)/addspec'     >> $(ALIASES)
	@echo 'alias backsub   $(progdir)/backsub'     >> $(ALIASES)
	@echo 'alias badgen    $(progdir)/badgen'      >> $(ALIASES)
	@echo 'alias bcrop     $(progdir)/bcrop'       >> $(ALIASES)
	@echo 'alias boxavg    $(progdir)/boxavg'      >> $(ALIASES)
	@echo 'alias boxmed    $(progdir)/boxmed'      >> $(ALIASES)
	@echo 'alias bsub      $(progdir)/bsub'      >> $(ALIASES)
	@echo 'alias cadd      $(progdir)/cadd'        >> $(ALIASES)
	@echo 'alias cdiv      $(progdir)/cdiv'        >> $(ALIASES)
	@echo 'alias cmul      $(progdir)/cmul'        >> $(ALIASES)
	@echo 'alias collapse  $(progdir)/collapse'    >> $(ALIASES)
	@echo 'alias combine   $(progdir)/combine'     >> $(ALIASES)
	@echo 'alias cplot     $(progdir)/cplot'       >> $(ALIASES)
	@echo 'alias crop      $(progdir)/crop'        >> $(ALIASES)
	@echo 'alias cset      $(progdir)/cset'        >> $(ALIASES)
	@echo 'alias cwin      $(progdir)/cwin'        >> $(ALIASES)
	@echo 'alias window    $(progdir)/window'      >> $(ALIASES)
	@echo 'alias diags     $(progdir)/diags'       >> $(ALIASES)
	@echo 'alias div       $(progdir)/div'         >> $(ALIASES)
	@echo 'alias dsub      $(progdir)/dsub'        >> $(ALIASES)
	@echo 'alias expand    $(progdir)/expand'      >> $(ALIASES)
	@echo 'alias fits2ucm  $(progdir)/fits2ucm'    >> $(ALIASES)
	@echo 'alias folder    $(progdir)/folder'      >> $(ALIASES)
	@echo 'alias genseries $(progdir)/genseries'   >> $(ALIASES)
	@echo 'alias gentemp   $(progdir)/gentemp'     >> $(ALIASES)
	@echo 'alias gettime   $(progdir)/gettime'     >> $(ALIASES)
	@echo 'alias grab      $(progdir)/grab'        >> $(ALIASES)
	@echo 'alias grab2fits $(progdir)/grab2fits'   >> $(ALIASES)
	@echo 'alias hedit     $(progdir)/hedit'       >> $(ALIASES)
	@echo 'alias hedlis    $(progdir)/hedlis'      >> $(ALIASES)
	@echo 'alias hist      $(progdir)/hist'        >> $(ALIASES)
	@echo 'alias list      $(progdir)/list'        >> $(ALIASES)
	@echo 'alias lplot     $(progdir)/lplot'       >> $(ALIASES)
	@echo 'alias makeflat  $(progdir)/makeflat'    >> $(ALIASES)
	@echo 'alias mdiv      $(progdir)/mdiv'        >> $(ALIASES)
	@echo 'alias movie     $(progdir)/movie'       >> $(ALIASES)
	@echo 'alias msub      $(progdir)/msub'        >> $(ALIASES)
	@echo 'alias mul       $(progdir)/mul'         >> $(ALIASES)
	@echo 'alias ncal      $(progdir)/ncal'        >> $(ALIASES)
	@echo 'alias noise     $(progdir)/noise'       >> $(ALIASES)
	@echo 'alias oneline   $(progdir)/oneline'     >> $(ALIASES)
	@echo 'alias pixel     $(progdir)/pixel'       >> $(ALIASES)
	@echo 'alias plot      $(progdir)/plot'        >> $(ALIASES)
	@echo 'alias ppos      $(progdir)/ppos'        >> $(ALIASES)
	@echo 'alias reduce    $(progdir)/reduce'      >> $(ALIASES)
	@echo 'alias rtplot    $(progdir)/rtplot'      >> $(ALIASES)
	@echo 'alias setaper   $(progdir)/setaper'     >> $(ALIASES)
	@echo 'alias setdefect $(progdir)/setdefect'   >> $(ALIASES)
	@echo 'alias setfield  $(progdir)/setfield'    >> $(ALIASES)
	@echo 'alias setreg    $(progdir)/setreg'      >> $(ALIASES)
	@echo 'alias setwin    $(progdir)/setwin'      >> $(ALIASES)
	@echo 'alias shifter   $(progdir)/shifter'     >> $(ALIASES)
	@echo 'alias sreduce   $(progdir)/sreduce'     >> $(ALIASES)
	@echo 'alias stats     $(progdir)/stats'       >> $(ALIASES)
	@echo 'alias sub       $(progdir)/sub'         >> $(ALIASES)
	@echo 'alias times     $(progdir)/times'       >> $(ALIASES)
	@echo 'alias ucm2fits  $(progdir)/ucm2fits'    >> $(ALIASES)
	@echo 'alias uinfo     $(progdir)/uinfo'       >> $(ALIASES)
	@echo 'alias uinit     $(progdir)/uinit'       >> $(ALIASES)
	@echo 'alias uset      $(progdir)/uset'        >> $(ALIASES)
	@echo 'alias vshow     $(progdir)/vshow'       >> $(ALIASES)
	@echo 'alias wjoin     $(progdir)/wjoin'       >> $(ALIASES)
	@echo 'alias addaframe $(progdir)/addaframe.csh'           >> $(ALIASES)
	@echo 'alias averun    $(progdir)/averun.csh'              >> $(ALIASES)
	@echo 'alias breakup   $(progdir)/breakup.csh'             >> $(ALIASES)
	@echo "alias busca2ucm 'python $(progdir)/busca2ucm.py'"   >> $(ALIASES)
	@echo 'alias check_for_bad_data $(progdir)/check_for_bad_data.csh' >> $(ALIASES)
	@echo "alias copymask  'perl $(progdir)/copymask.pl'"      >> $(ALIASES)
	@echo 'alias debias    $(progdir)/debias.csh'              >> $(ALIASES)
	@echo "alias dummy     'perl $(progdir)/dummy.pl'"         >> $(ALIASES)
	@echo 'alias extract_times $(progdir)/extract_times.csh'   >> $(ALIASES)
	@echo "alias fmatch    'python $(progdir)/fmatch.py'"      >> $(ALIASES)
	@echo "alias grms      'python $(progdir)/grms.py'"        >> $(ALIASES)
	@echo "alias hotpix    'python $(progdir)/hotpix.py'"      >> $(ALIASES)
	@echo "alias l3gain    'perl $(progdir)/l3gain.pl'"        >> $(ALIASES)
	@echo 'alias makebias  $(progdir)/makebias.csh'            >> $(ALIASES)
	@echo "alias missbias  'python $(progdir)/missbias.py'"    >> $(ALIASES)
	@echo 'alias mulaframe $(progdir)/mulaframe.csh'           >> $(ALIASES)
	@echo "alias newreduce 'perl $(progdir)/newreduce.pl'"     >> $(ALIASES)
	@echo "alias pbias     'python $(progdir)/pbias.py'"       >> $(ALIASES)
	@echo "alias procbias  'python $(progdir)/procbias.py'"    >> $(ALIASES)
	@echo "alias setup     'perl $(progdir)/setup.pl'"         >> $(ALIASES)
	@echo "alias shiftaper 'perl $(progdir)/shiftaper.pl'"     >> $(ALIASES)
	@echo "alias splitr    'perl $(progdir)/splitr.pl'"        >> $(ALIASES)
	@echo 'alias sumframes $(progdir)/sumframes.csh'           >> $(ALIASES)
	@echo "alias transform 'perl $(progdir)/transform.pl'"     >> $(ALIASES)
	@echo 'alias ucat      $(progdir)/ucat.csh'                >> $(ALIASES)
	@echo "alias unique    'python $(progdir)/unique.py'"      >> $(ALIASES)
	@echo 'alias uls       $(progdir)/uls.csh'                 >> $(ALIASES)
	@echo "alias ulog2fits 'python $(progdir)/ulog2fits.py'"   >> $(ALIASES)
	@echo 'alias ult2asc   $(progdir)/ult2asc.pl'              >> $(ALIASES)
	@echo "alias updateaper 'perl $(progdir)/updateaper.pl'"   >> $(ALIASES)
#	@echo "setenv ULTRACAM $(progdir)"                         >> $(ALIASES)
	@echo ' ' >> $(ALIASES)
	@echo 'echo " "' >> $(ALIASES)
	@echo 'echo "Welcome to the $(PACKAGE) pipeline software, $(VERSION), built $(DATE)"' >> $(ALIASES)
	@echo 'echo " "' >> $(ALIASES)
	@echo 'echo "Point a browser at ${prefix}/html/$(PACKAGE)/index.html for help."' >> $(ALIASES)
	@echo 'echo " "' >> $(ALIASES)
