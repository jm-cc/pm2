
## $Id: Makefile,v 1.2 2001/04/02 12:23:42 bouge Exp $

GNUPLOT= gnuplot
FIG2DEV= fig2dev

.SUFFIXES:

.SUFFIXES: .eps .dvi .ps .tex .bbl .gp .eps .c

.PHONY: all eps bbl dvi ps touch tar  clean open split

TEXFILES	:= start.tex
STYFILES	:= start.sty

TO_SPLIT	:= rpc-threads1 migration3 migrate-group dsm dsm1 dsm2

CFILES_TO_SPLIT	:= $(TO_SPLIT:%=Progs/%.c)
CFILES_SPLIT_0	:= $(CFILES_TO_SPLIT:%=%-0) 
CFILES_SPLIT_1	:= $(CFILES_TO_SPLIT:%=%-1)

PLOTFILES	:= $(wildcard *.plot)
BIBFILES	:= $(wildcard *.bib)
FIGFILES	:= $(wildcard *.fig)
GPFILES 	:= $(wildcard *.gp)
EPSFILES	:= $(wildcard *.eps)
PLOTFILES	:= $(wildcard *.plot)
CFILES		:= $(wildcard Progs/*.c)

BBLFILES	:= $(TEXFILES:%.tex=%.bbl)
AUXFILES	:= $(TEXFILES:%.tex=%.aux)
DVIFILES	:= $(TEXFILES:%.tex=%.dvi)
PSFILES 	:= $(TEXFILES:%.tex=%.ps)
FIGEPSFILES	:= $(FIGFILES:%.fig=%.eps)
GPEPSFILES	:= $(GPFILES:%.gp=%.eps)
ALLEPSFILES	:= $(FIGEPSFILES) $(GPEPSFILES) $(EPSFILES)

OFILES		:= $(CFILES:%.c=%.o)
EXEFILES	:= $(CFILES:%.c=%)

BBLFILES	:= 
INCLUDEFILES	:= $(CFILES) $(CFILES_SPLIT_0) $(CFILES_SPLIT_1) \
			$(ALLEPSFILES) $(BBLFILES) $(STYFILES)

all: eps dvi split

$(CFILES_SPLIT_0): %-0: %
	perl -w Progs/split.pl $<

$(CFILES_SPLIT_1): %-1: %
	perl -w Progs/split.pl $<

split: $(CFILES_SPLIT_0) $(CFILES_SPLIT_1)

$(GPEPSFILES): %.eps: $(PLOTFILES) %.gp
	$(GNUPLOT) $*.gp > $*.eps		

$(FIGEPSFILES): %.eps: %.fig
	$(FIG2DEV) -L ps $*.fig > $*.eps

eps: $(ALLEPSFILES)

$(AUXFILES): %.aux: %.tex 
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

$(BBLFILES): %.bbl: %.tex $(BIBFILES) $(INCLUDEFILES)
	$(MAKE) $*.aux
	-rm *.dvi	# Here!
	bibtex $*

bbl: $(BBLFILES)

$(DVIFILES): %.dvi: %.tex $(INCLUDEFILES)
	latex $*
	@while grep --silent 'Rerun to ' $*.log; \
		do latex $*; done

dvi: $(DVIFILES) 

$(PSFILES): %.ps: %.dvi
	dvips $* -o
	chmod -R a+rX $*.*

ps: $(PSFILES) 

clean: open
	-rm *.bbl *.aux *.log *.cb *.ps *.blg *~ *.dvi *.toc 
	-rm $(OFILES) $(EXEFILES)
	-rm $(CFILES_SPLIT_0) $(CFILES_SPLIT_1)

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
