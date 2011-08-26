.PHONY: all upload wml-upload pdf-upload html pdf emph clean emph-clean html-clean pdf-clean

XML2HTML		:=	../xml2html

TARGET			:=	start

CFILES			:= $(wildcard Progs/*.c Progs/*.txt Progs/*.cfg)
EMPHCFILES_PDF		:= $(CFILES:%=%_emph_pdf) Progs/Makefile1_emph_pdf
EMPHCFILES_HTML		:= $(CFILES:%=%_emph_html) Progs/Makefile1_emph_html

XML_FILES		:=	$(wildcard ./Sources/*.xml)
DEPS			:=	$(XML_FILES)
DEPS			+=	$(wildcard ./Sources/code/*)
DEPS			+=	$(XML2HTML)/lib/xml2html.jar
DEPS			+=	$(XML2HTML)/data/xml/DOCUMENT.dtd

BUILD			:=	./build

FIGURES_PDF		:=	$(wildcard ./Figures/*.eps)
FIGURES_HTML		:=	$(FIGURES_PS:%.eps=%.png)

NB_XML_FILES		+=	$(shell echo $(XML_FILES) | wc -w)
XML_HTML_FILES		+=	$(shell seq 1 $(NB_XML_FILES) | sed 's/\(.*\)/Section\1.html/')

RSYNC		:= rsync
RSYNCOPTIONS	:= -avz
PUBLISHHOST	:= sync
PUBLISHDIR	:= $(PUBLISHHOST):/web/runtime/pm2-doc

THISDIR			:=	$(CURDIR)

upload: pdf-upload html-upload

html-upload: html
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/html/$(TARGET)/ $(PUBLISHDIR)

pdf-upload: $(BUILD)/pdf/$(TARGET).pdf
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/pdf/$(TARGET).pdf $(PUBLISHDIR)/resources

html: $(BUILD)/html/$(TARGET)/index.html

$(BUILD)/html/$(TARGET)/%.html: $(TARGET).xml $(DEPS) $(FIGURES_HTML) $(EMPHCFILES_HTML)
	$(XML2HTML)/bin/shell/xml2runtime.sh $(TARGET).xml $(BUILD)/html/$(TARGET) $(SECTIONS)

pdf: $(BUILD)/pdf/$(TARGET).pdf

$(BUILD)/pdf/$(TARGET).pdf: $(BUILD)/pdf/$(TARGET).tex
	cd $(BUILD)/pdf/ && ln -sf ../../Figures
	cd $(BUILD)/pdf/ && ln -sf ../../Progs
	cd $(BUILD)/pdf/ && ln -sf ../../$(XML2HTML)/data/latex/* .
	cd $(BUILD)/pdf/ && rubber -d $(TARGET).tex

$(BUILD)/pdf/$(TARGET).tex: $(TARGET).xml $(DEPS) $(FIGURES_PDF) $(EMPHCFILES_PDF)
	mkdir -p $(BUILD)/pdf
	$(XML2HTML)/bin/shell/xml2latex.sh $(TARGET).xml $(BUILD)/pdf $(SECTIONS)

emph: $(EMPHCFILES_PDF) $(EMPHCFILES_HTML)
$(EMPHCFILES_PDF): %_emph_pdf: %
	@perl -w Progs/emph.pl $< $@ pdf
$(EMPHCFILES_HTML): %_emph_html: %
	@perl -w Progs/emph.pl $< $@ html

distclean: clean

clean: html-clean html-clean pdf-clean emph-clean
	rm -rf $(BUILD)

emph-clean:; rm -f $(EMPHCFILES_PDF) $(EMPHCFILES_HTML)
html-clean:; rm -rf $(BUILD)/html
pdf-clean:; rm -rf $(BUILD)/pdf

