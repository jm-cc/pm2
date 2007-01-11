.PHONY: all upload wml-upload pdf-upload wml pdf clean wml-clean ps-clean

XML2HTML		:=	./xml2html

TARGET			:=	madMpiDoc

XML_FILES		:=	$(wildcard *.xml)
DEPS			:=	$(XML_FILES)
DEPS			+=	$(XML2HTML)/lib/xml2html.jar
DEPS			+=	$(XML2HTML)/data/xml/DOCUMENT.dtd

WWW			:=	/net/cvs/runtime/site/www/MadMPI/doc
BUILD			:=	./build

FIGURES_PS		:=	$(wildcard ./Figures/*.eps)
FIGURES_HTML		:=	$(FIGURES_PS:%.eps=%.png)

WML_FILES		+=	$(wildcard $(BUILD)/wml/$(TARGET)/*.wml)
WML_DEST		+=	$(patsubst $(BUILD)/wml/$(TARGET)/%.wml,$(WWW)/%.wml,$(WML_FILES))
WML_DEST		+=	$(WWW)/my_macros.txt

WML_FIGURES_DEST	:=	$(patsubst ./Figures/%.png,$(WWW)/Figures/%.png,$(FIGURES_HTML))

THISDIR			:=	$(CURDIR)

all: pdf

upload: pdf-upload wml-upload
	make -C $(WWW)/..

wml-upload: $(WML_DEST) $(WML_FIGURES_DEST)

$(WWW)/%.wml: $(BUILD)/wml/$(TARGET)/%.wml
	cp $< $@

$(WWW)/Figures/%.png: ./Figures/%.png
	cp $< $@

$(WWW)/my_macros.txt: ./my_macros.txt
	cp $< $@

pdf-upload: $(WWW)/resources/$(TARGET).pdf

$(WWW)/resources/$(TARGET).pdf: $(THISDIR)/$(TARGET).pdf
	cp $< $@

wml: $(BUILD)/wml/$(TARGET)/index.wml

$(BUILD)/wml/$(TARGET)/%.wml: $(TARGET).xml $(DEPS) $(FIGURES_HTML)
	$(XML2HTML)/bin/shell/xml2wml.sh $(TARGET).xml $(BUILD)/wml/$(TARGET) $(SECTIONS)

pdf: $(THISDIR)/$(TARGET).pdf

$(THISDIR)/$(TARGET).pdf: $(BUILD)/ps/$(TARGET).ps
	ps2pdf $< $@

$(BUILD)/ps/$(TARGET).ps: $(BUILD)/ps/$(TARGET).tex
	cd $(BUILD)/ps/ && ln -sf ../../Figures
	cd $(BUILD)/ps/ && ln -sf ../../Progs
	cd $(BUILD)/ps/ && ln -sf ../../$(XML2HTML)/data/latex/* .
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && latex $(TARGET)
	cd $(BUILD)/ps/ && dvips $(TARGET) -o 

$(BUILD)/ps/$(TARGET).tex: $(TARGET).xml $(DEPS) $(FIGURES_PS)
	mkdir -p $(BUILD)/ps
	$(XML2HTML)/bin/shell/xml2latex.sh $(TARGET).xml $(BUILD)/ps $(SECTIONS)

distclean: clean

clean: wml-clean ps-clean
	rm -rf $(BUILD)
wml-clean:; rm -rf $(BUILD)/wml
ps-clean:; rm -rf $(BUILD)/ps $(THISDIR)/$(TARGET).pdf
