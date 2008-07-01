.PHONY: all upload wml-upload pdf-upload html ps emph clean emph-clean html-clean ps-clean

XML2HTML		:=	../xml2html

TARGET			:=	start

CFILES			:= $(wildcard Progs/*.c Progs/*.txt Progs/*.cfg)
EMPHCFILES_PS		:= $(CFILES:%=%_emph_ps) Progs/Makefile1_emph_ps
EMPHCFILES_HTML		:= $(CFILES:%=%_emph_html) Progs/Makefile1_emph_html

XML_FILES		:=	$(wildcard ./Sources/*.xml)
DEPS			:=	$(XML_FILES)
DEPS			+=	$(wildcard ./Sources/code/*)
DEPS			+=	$(XML2HTML)/lib/xml2html.jar
DEPS			+=	$(XML2HTML)/data/xml/DOCUMENT.dtd

BUILD			:=	./build

FIGURES_PS		:=	$(wildcard ./Figures/*.eps)
FIGURES_HTML		:=	$(FIGURES_PS:%.eps=%.png)

NB_XML_FILES		+=	$(shell echo $(XML_FILES) | wc -w)
XML_HTML_FILES		+=	$(shell seq 1 $(NB_XML_FILES) | sed 's/\(.*\)/Section\1.html/')

RSYNC		:= rsync
RSYNCOPTIONS	:= -avz
PUBLISHDIR	:= webruntime@runtime:/home/webruntime/html/pm2-doc

THISDIR			:=	$(CURDIR)

upload: pdf-upload html-upload

html-upload: html
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/html/$(TARGET)/ $(PUBLISHDIR)

pdf-upload: $(BUILD)/ps/$(TARGET).pdf
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/ps/$(TARGET).pdf $(PUBLISHDIR)/resources

$(BUILD)/ps/$(TARGET).pdf: $(BUILD)/ps/$(TARGET).ps
	ps2pdf $< $@

html: $(BUILD)/html/$(TARGET)/index.html

$(BUILD)/html/$(TARGET)/%.html: $(TARGET).xml $(DEPS) $(FIGURES_HTML) $(EMPHCFILES_HTML)
	$(XML2HTML)/bin/shell/xml2runtime.sh $(TARGET).xml $(BUILD)/html/$(TARGET) $(SECTIONS)

ps: $(THISDIR)/$(TARGET).ps

$(BUILD)/ps/$(TARGET).ps: $(BUILD)/ps/$(TARGET).tex
	cd $(BUILD)/ps/ && ln -sf ../../Figures
	cd $(BUILD)/ps/ && ln -sf ../../Progs
	cd $(BUILD)/ps/ && ln -sf ../../$(XML2HTML)/data/latex/* .
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && dvips $(TARGET) -o

$(BUILD)/ps/$(TARGET).tex: $(TARGET).xml $(DEPS) $(FIGURES_PS) $(EMPHCFILES_PS)
	mkdir -p $(BUILD)/ps
	$(XML2HTML)/bin/shell/xml2latex.sh $(TARGET).xml $(BUILD)/ps $(SECTIONS)

emph: $(EMPHCFILES_PS) $(EMPHCFILES_HTML)
$(EMPHCFILES_PS): %_emph_ps: %
	@perl -w Progs/emph.pl $< $@ ps
$(EMPHCFILES_HTML): %_emph_html: %
	@perl -w Progs/emph.pl $< $@ html

distclean: clean

clean: html-clean html-clean ps-clean emph-clean
	rm -rf $(BUILD)

emph-clean:; rm -f $(EMPHCFILES_PS) $(EMPHCFILES_HTML)
html-clean:; rm -rf $(BUILD)/html
ps-clean:; rm -rf $(BUILD)/ps $(THISDIR)/$(TARGET).ps

