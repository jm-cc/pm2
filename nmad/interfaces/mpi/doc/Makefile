.PHONY: all upload html-upload pdf-upload html pdf emph clean emph-clean html-clean ps-clean

XML2HTML		:=	./xml2html

TARGET			:=	madMpiDoc

CFILES			:=	$(wildcard code/*.c code/*.txt code/*.cfg code/*.out)
EMPHCFILES_PS		:=	$(CFILES:%=%_emph_ps)
EMPHCFILES_HTML		:=	$(CFILES:%=%_emph_html)

XML_FILES		:=	$(wildcard *.xml)
DEPS			:=	$(XML_FILES)
DEPS			+=	$(XML2HTML)/lib/xml2html.jar
DEPS			+=	$(XML2HTML)/data/xml/DOCUMENT.dtd

BUILD			:=	./build

FIGURES_PS		:=	$(wildcard ./Figures/*.eps)
FIGURES_HTML		:=	$(FIGURES_PS:%.eps=%.png)

THISDIR			:=	$(CURDIR)

RSYNC			:= rsync
RSYNCOPTIONS		:= -avz
PUBLISHDIR		:= webruntime@runtime:/home/webruntime/html/MadMPI/doc

all: pdf-upload html-upload

html-upload: $(BUILD)/html/$(TARGET)/index.html
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/html/$(TARGET)/ $(PUBLISHDIR)

$(BUILD)/html/$(TARGET)/%.html: $(TARGET).xml $(DEPS) $(FIGURES_HTML) $(EMPHCFILES_HTML)
	$(XML2HTML)/bin/shell/xml2runtime.sh $(TARGET).xml $(BUILD)/html/$(TARGET) $(SECTIONS)

pdf-upload: $(BUILD)/ps/$(TARGET).pdf
	$(RSYNC) $(RSYNCOPTIONS) $(BUILD)/ps/$(TARGET).pdf $(PUBLISHDIR)/resources

$(BUILD)/ps/$(TARGET).pdf: $(BUILD)/ps/$(TARGET).ps
	ps2pdf $< $@

$(BUILD)/ps/$(TARGET).ps: $(BUILD)/ps/$(TARGET).tex
	cd $(BUILD)/ps/ && ln -sf ../../figures
	cd $(BUILD)/ps/ && ln -sf ../../code
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
	@perl -w emph.pl $< $@ ps
$(EMPHCFILES_HTML): %_emph_html: %
	@perl -w emph.pl $< $@ html

distclean: clean

clean: html-clean ps-clean emph-clean
	rm -rf $(BUILD)
emph-clean:; rm -f $(EMPHCFILES_PS) $(EMPHCFILES_HTML)
html-clean:; rm -rf $(BUILD)/html
ps-clean:; rm -rf $(BUILD)/ps $(THISDIR)/$(TARGET).pdf

