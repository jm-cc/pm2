.PHONY: all upload wml-upload pdf-upload wml ps inria labri emph clean emph-clean wml-clean ps-clean labri-clean inria-clean

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

WWW			:=	/net/cvs/runtime/site/www/pm2-doc
BUILD			:=	./build

FIGURES_PS		:=	$(wildcard ./Figures/*.eps)
FIGURES_HTML		:=	$(FIGURES_PS:%.eps=%.png)

NB_XML_FILES		+=	$(shell echo $(XML_FILES) | wc -w)
XML_WML_FILES		+=	$(shell seq 1 $(NB_XML_FILES) | sed 's/\(.*\)/Section\1.wml/')

WML_DEST		=	$(WWW)/index.wml
WML_DEST		+=	$(WWW)/footnote.wml
WML_DEST		+=	$(WWW)/undefined.wml
WML_DEST		+=	$(patsubst %.wml,$(WWW)/%.wml,$(XML_WML_FILES))
WML_DEST		+=	$(WWW)/my_macros.txt

WML_FIGURES_DEST	:=	$(patsubst ./Figures/%.png,$(WWW)/Figures/%.png,$(FIGURES_HTML))

THISDIR			:=	$(CURDIR)

all: $(THISDIR)/$(TARGET).pdf

$(THISDIR)/$(TARGET).pdf: $(BUILD)/ps/$(TARGET).ps
	ps2pdf $< $@

upload: pdf-upload wml-upload
	make -C $(WWW)

wml-upload: $(WML_DEST) $(WML_FIGURES_DEST)

$(WWW)/%.wml: $(BUILD)/wml/$(TARGET)/%.wml
	cp $< $@

$(WWW)/Figures/%.png: ./Figures/%.png
	cp $< $@

$(WWW)/my_macros.txt: ./Sources/wml/my_macros.txt
	cp $< $@

pdf-upload: $(WWW)/resources/$(TARGET).pdf

$(WWW)/resources/$(TARGET).pdf: $(BUILD)/ps/$(TARGET).ps
	ps2pdf $< $@

wml: $(BUILD)/wml/$(TARGET)/index.wml

$(BUILD)/wml/$(TARGET)/%.wml: $(TARGET).xml $(DEPS) $(FIGURES_HTML) $(EMPHCFILES_HTML)
	$(XML2HTML)/bin/shell/xml2wml.sh $(TARGET).xml $(BUILD)/wml/$(TARGET) $(SECTIONS)

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

html: $(BUILD)/html/$(TARGET)/index.html

$(BUILD)/html/$(TARGET)/%.html: $(TARGET).xml $(DEPS) $(FIGURES_HTML) $(EMPHCFILES_HTML)
	$(XML2HTML)/bin/shell/xml2html.sh $(TARGET).xml $(BUILD)/html/$(TARGET) $(SECTIONS)

inria: $(BUILD)/inria/$(TARGET).ps

$(BUILD)/inria/$(TARGET).ps: $(BUILD)/inria/$(TARGET).tex
	cd $(BUILD)/inria/ && ln -sf ../../Figures
	cd $(BUILD)/inria/ && ln -sf ../../Progs
	cd $(BUILD)/inria/ && ln -sf ../../$(XML2HTML)/data/latex/* .
	cd $(BUILD)/inria/ && latex $(TARGET)
	cd $(BUILD)/inria/ && latex $(TARGET)
	cd $(BUILD)/inria/ && latex $(TARGET)
	cd $(BUILD)/inria/ && dvips $(TARGET) -o

$(BUILD)/inria/$(TARGET).tex: $(TARGET).xml $(DEPS) $(FIGURES_PS) $(EMPHCFILES_PS)
	mkdir -p $(BUILD)/inria
	$(XML2HTML)/bin/shell/xml2inriaReport.sh $(TARGET).xml $(BUILD)/inria $(SECTIONS)

labri: $(BUILD)/labri/$(TARGET).ps

$(BUILD)/labri/$(TARGET).ps: $(BUILD)/labri/$(TARGET).tex
	cd $(BUILD)/labri/ && ln -sf ../../Figures
	cd $(BUILD)/labri/ && ln -sf ../../Progs
	cd $(BUILD)/labri/ && ln -sf ../../$(XML2HTML)/data/latex/* .
	cd $(BUILD)/labri/ && latex $(TARGET)
	cd $(BUILD)/labri/ && latex $(TARGET)
	cd $(BUILD)/labri/ && latex $(TARGET)
	cd $(BUILD)/labri/ && dvips $(TARGET) -o

$(BUILD)/labri/$(TARGET).tex: $(TARGET).xml $(DEPS) $(FIGURES_PS) $(EMPHCFILES_PS)
	mkdir -p $(BUILD)/labri
	$(XML2HTML)/bin/shell/xml2labriReport.sh $(TARGET).xml $(BUILD)/labri $(SECTIONS)

emph: $(EMPHCFILES_PS) $(EMPHCFILES_HTML)
$(EMPHCFILES_PS): %_emph_ps: %
	@perl -w emph.pl $< $@ ps 
$(EMPHCFILES_HTML): %_emph_html: %
	@perl -w emph.pl $< $@ html

distclean: clean

clean: wml-clean html-clean ps-clean inria-clean labri-clean emph-clean
	rm -rf $(BUILD)

emph-clean:; rm -f $(EMPHCFILES_PS) $(EMPHCFILES_HTML)
wml-clean:; rm -rf $(BUILD)/wml

html-clean:; rm -rf $(BUILD)/html

ps-clean:; rm -rf $(BUILD)/ps $(THISDIR)/$(TARGET).ps

inria-clean:; rm -rf $(BUILD)/inria

labri-clean:; rm -rf $(BUILD)/labri
