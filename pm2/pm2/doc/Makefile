
LATEX = latex
BIBTEX = bibtex

#SRC_DOC = $(wildcard *.tex)
SRC_DOC = getting_started.tex
BIBFILES = $(wildcard *.bib)

INCLUDE_MAKE = $(wildcard Makefile.include)
ifeq ($(INCLUDE_MAKE),Makefile.include)
include Makefile.include
endif

PS?= $(SRC_DOC:%.tex=%.ps)

DVI = $(PS:%.ps=%.dvi)

DOCS = $(PS)

all: doc clean
	@rep="$(dir $(wildcard */Makefile))" ;\
	if [ "$$rep" ]; then \
		for i in $$rep ; do \
			( cd $$i ; $(MAKE) ) ;\
		done ;\
	fi

doc: $(DOCS)

ps: $(PS)

%.ps: %.dvi
	@if [ $* = transparents ]; then \
		OPTIONS="-t landscape" ;\
		echo "Using paper landscape..." ;\
	else \
		OPTIONS="" ;\
	fi ;\
	dvips $$OPTIONS -o $*.ps $*.dvi

%.dvi: %.tex $(BIBFILES)
	@$(LATEX) $*.tex | tee tmp.log ; \
	restart=`grep 'Rerun to get cross-references right' tmp.log` ;\
	if [ -z "$$restart" ]; then \
		restart=`grep 'No file .*.los' tmp.log` ;\
	fi ;\
	bib=`grep 'Citation .* undefined' tmp.log` ;\
	if [ "$$bib" != "toujours" -a ! -z "$$bib" ]; then \
		$(BIBTEX) $* ;\
		restart=y ;\
	fi ;\
	while [ "$$restart" != "" ] ; do \
		$(LATEX) $*.tex | tee tmp.log ; \
		restart=`grep 'Rerun to get cross-references right' tmp.log` ;\
		if [ -z "$$restart" ]; then \
			restart=`grep 'No file .*\.los' tmp.log` ;\
		fi ;\
	done ;\
	$(RM) tmp.log

clean:
	$(RM) *.aux *.dvi *.tmp *.toc *.log *.los *.bbl *.blg *~ \#*\#

realclean: clean
	$(RM) $(DOCS)

.PHONY: clean realclean all doc ps

distclean: realclean
	@rep="$(dir $(wildcard */Makefile))" ;\
	if [ "$$rep" ]; then \
		for i in $$rep ; do \
			( cd $$i ; $(MAKE) distclean ) ;\
		done ;\
	fi


