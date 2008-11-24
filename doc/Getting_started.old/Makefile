## $Id: Makefile,v 1.10 2003/04/14 15:21:08 vdanjean Exp $

GNUPLOT= gnuplot
FIG2DEV= fig2dev

.SUFFIXES:

.SUFFIXES: .eps .dvi .ps .tex .bbl .gp .eps .c

XWDGOAL		:= ezflavor options

.PHONY: all eps bbl dvi ps touch tar  clean distclean open split emph $(XWDGOAL)

TEXFILES	:= appendicing.tex discovering.tex \
		installing.tex mastering.tex title.tex \
		customizing.tex firstpage.tex introducing.tex \
		start.tex wizarding.tex

ROOTTEXFILES	:= start.tex
STYFILES	:= start.sty

TO_SPLIT	:= rpc-threads1 migration3 migrate-group \
		freeze mutex semaphore condition

CFILES		:= $(wildcard Progs/*.c)
EMPHCFILES_TO_SPLIT	:= $(TO_SPLIT:%=Progs/%.c-emph)


EMPHCFILES	:= $(CFILES:%=%-emph) Progs/Makefile1-emph
EMPHCFILES_SPLIT_0	:= $(EMPHCFILES_TO_SPLIT:%=%-0) 
EMPHCFILES_SPLIT_1	:= $(EMPHCFILES_TO_SPLIT:%=%-1)

BIBFILES	:= $(wildcard *.bib)
FIGFILES	:= $(wildcard Figures/*.fig)
GPFILES 	:= $(wildcard Measures/*.gp)
PLOTFILES	:= $(wildcard Measures/*.plot)
XWDFILES	:= $(XWDGOAL:%=Figures/%.xwd)

BBLFILES	:= $(ROOTTEXFILES:%.tex=%.bbl)
AUXFILES	:= $(ROOTTEXFILES:%.tex=%.aux)
DVIFILES	:= $(ROOTTEXFILES:%.tex=%.dvi)
PSFILES 	:= $(ROOTTEXFILES:%.tex=%.ps)
FIGEPSFILES	:= $(FIGFILES:%.fig=%.eps)
GPEPSFILES	:= $(GPFILES:%.gp=%.eps)
XWDEPSFILES	:= $(XWDFILES:%.xwd=%.eps)
PUREEPSFILES	:= $(wildcard *.eps)

AUXEPSFILES	:= $(FIGEPSFILES) $(GPEPSFILES) $(XWDEPSFILES)
EPSFILES	:= $(AUXEPSFILES) $(PUREEPSFILES)

OFILES		:= $(CFILES:%.c=%.o)
EXEFILES	:= $(CFILES:%.c=%)

BBLFILES	:= 
INCLUDEFILES	:= $(EMPHCFILES) $(EMPHCFILES_SPLIT_0) $(EMPHCFILES_SPLIT_1) \
			$(EPSFILES) $(BBLFILES) $(STYFILES)

all: eps dvi split

$(EMPHCFILES): %-emph: %
	@perl -w emph.pl $< 

emph: $(EMPHCFILES)

$(EMPHCFILES_SPLIT_0): %-0: %
	@perl -w split.pl $<

$(EMPHCFILES_SPLIT_1): %-1: %
	@perl -w split.pl $<

split: $(EMPHCFILES_SPLIT_0) $(EMPHCFILES_SPLIT_1)

$(GPEPSFILES): %.eps: $(PLOTFILES) %.gp
	$(GNUPLOT) $*.gp > $*.eps		

$(FIGEPSFILES): %.eps: %.fig
	$(FIG2DEV) -L ps $*.fig > $*.eps

eps: $(AUXEPSFILES)

$(AUXFILES): %.aux: %.tex $(TEXFILES)
	latex $*

# The rule below creates a problem: $(MAKE) $*.aux create a dvi file,
# and the next action creates a bbl file. If this rule is called from
# the $(DVIFILES): rule, then the dvi file has a modification time too
# close with respect to the bbl file, so that Makefile believes that
# the dependence is satisfied. Actually, make uses the time(2)
# function to get time, with a one-second precision. Also,
# modification times are compared with a *strict* comparaison!

# The turnaround trick is either to wait for one second in the bbl
# rule, or to delete the dvi file created as a byproduct within this
# rule. I chose this latter solution, as it is safer.

$(BBLFILES): %.bbl: %.tex $(BIBFILES) $(INCLUDEFILES) $(TEXFILES)
	$(MAKE) $*.aux
	-rm *.dvi	# Here!
	bibtex $*

bbl: $(BBLFILES)

$(DVIFILES): %.dvi: %.tex $(INCLUDEFILES) $(TEXFILES)
	latex $*
	@while grep --silent 'Rerun to ' $*.log; \
		do latex $*; done

dvi: $(DVIFILES) 

$(PSFILES): %.ps: %.dvi
	dvips $* -o
	chmod -R a+rX $*.*

ps: $(PSFILES) 

distclean clean: open
	-$(RM) *.bbl *.aux *.log *.cb *.ps *.blg *~ *.dvi *.toc 
	-$(RM) $(OFILES) $(EXEFILES)
	-$(RM) $(EMPHCFILES) $(EMPHCFILES_SPLIT_0) $(EMPHCFILES_SPLIT_1)
##	-$(RM) $(AUXEPSFILES)

open:
	-chmod -R a+rX . ./*

touch:
	touch $(TEXFILES) $(CFILES) $(BIBFILES) $(PLOTFILES) (EPSFILES)

DATE	:= $(shell date '+%Y-%m-%d')
PWD	:= $(shell pwd)
DIRNAME	:= $(notdir $(PWD))

tar:
	cd ..; tar zcfv $(DIRNAME)_$(DATE).tgz $(DIRNAME); \
	chmod a+rX *; \
	ls -la $(DIRNAME)_$(DATE).tgz

$(XWDEPSFILES): %.eps: %.xwd
## xpr -device ps -portrait -gray 4 -psfig -compact $< > $@ See
## Using xpr seems deprecated.
## Using xv is quite painful, as there is no script mode.
## Using the Solaris convert utility looks best. See
## http://www.lbl.gov/ICSD/CIS/faq/xwd-color-PS.html for comments
## about this method
	-convert $< $@
	touch $@

Figures/ezflavor.xwd: WINDOWNAME=	"ezflavor"
Figures/options.xwd:  WINDOWNAME=	"Common options"

$(XWDFILES):
	@echo 'Capturing window named "$(WINDOWNAME)"...'
	@echo 'Wait for the rings!'
	@echo 'Warning: You should first raise this window on top!'
	@sleep 3
	xwd -debug -nobdrs -name $(WINDOWNAME) -out $@

$(XWDGOAL):
	-mv Figures/$@.xwd Figures/$@.xwd.sv
	$(MAKE) Figures/$@.xwd
