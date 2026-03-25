# Prerequisite variables
SOURCEDIR := src
OUTPUTDIR := build

SOURCES := $(shell find $(SOURCEDIR) -type f -name '*.java')
CLASSES := $(SOURCES:$(SOURCEDIR)/%.java=$(OUTPUTDIR)/%.class)

TARGETS := $(shell find $(SOURCEDIR) -mindepth 1 -maxdepth 1 -type d)
TARGETS := $(TARGETS:$(SOURCEDIR)/%=$(OUTPUTDIR)/%.jar)

# Add all jars to classpath and allow compiling with private API
JCFLAGS := -cp "$(subst $() $(),:,$(wildcard $(SOURCEDIR)/*/):$(wildcard libs/*/*.jar))" -XDignore.symbol.file
JAVAC_SUPPORTS_PATCH_MODULE := $(shell { $(BOOTJDK)/javac --help-extra 2>&1 || true; } | grep -q -- '--patch-module' && echo 1 || echo 0)

all: $(TARGETS)
	
# Sometimes the CI produces empty class file resulting in broken jar, so we check and abort here
check_empty_classes: $(CLASSES)
	@set -e; \
	echo "Checking for empty class files"; \
	EMPTY_CLASSES="$$(find $(OUTPUTDIR) -name '*.class' -size 0)"; \
	if [ -n "$$EMPTY_CLASSES" ]; then \
		echo "Error: found empty class files in $@: $$EMPTY_CLASSES"; \
		exit 1; \
	fi

# Rules for building java class
$(CLASSES): $(OUTPUTDIR)/%.class: $(SOURCEDIR)/%.java
	@set -e; \
	TARGET="$(word 2, $(subst /, ,$@))"; \
	CLASS_BASENAME="$(basename $(notdir $<))"; \
	PACKAGE_DIR="$(dir $(patsubst $(OUTPUTDIR)/$(word 2, $(subst /, ,$@))/%,%,$@))"; \
	STAGEDIR="$(OUTPUTDIR)/staging/$$TARGET/$$CLASS_BASENAME"; \
	mkdir -p $(OUTPUTDIR)/classes; \
	rm -rf "$$STAGEDIR"; \
	mkdir -p "$$STAGEDIR"; \
	mkdir -p "$(OUTPUTDIR)/classes/$$PACKAGE_DIR"; \
	mkdir -p "$(dir $@)"; \
	PATCH_MODULE_FLAGS=""; \
	case "$<" in \
		$(SOURCEDIR)/runtime_patches/*) \
			if [ "$(JAVAC_SUPPORTS_PATCH_MODULE)" = "1" ]; then \
				PATCH_MODULE_FLAGS="--patch-module=java.base=$(SOURCEDIR)/runtime_patches"; \
			fi ;; \
	esac; \
	echo "Compiling $<"; \
	$(BOOTJDK)/javac $(JCFLAGS) $$PATCH_MODULE_FLAGS -d "$$STAGEDIR" $<; \
	cp "$$STAGEDIR/$$PACKAGE_DIR$$CLASS_BASENAME"*.class "$(OUTPUTDIR)/classes/$$PACKAGE_DIR"; \
	cp "$$STAGEDIR/$$PACKAGE_DIR$$CLASS_BASENAME"*.class "$(dir $@)/"; \
	rm -rf "$$STAGEDIR"

$(OUTPUTDIR)/launcher.jar: $(CLASSES) check_empty_classes
	@set -e
	@echo "Creating $@"
	@$(BOOTJDK)/jar -cf $@ -C $(basename $@) .

$(OUTPUTDIR)/lwjgl.jar: $(CLASSES) check_empty_classes
	@set -e
	@echo "Creating $@"
	@mkdir -p $(OUTPUTDIR)/lwjgl_lib
	@cd $(OUTPUTDIR)/lwjgl_lib && find ../../libs/lwjgl -name 'lwjgl*.jar' -exec $(BOOTJDK)/jar -xf {} \; && rm -rf META-INF
	@cp -R $(OUTPUTDIR)/lwjgl/* $(OUTPUTDIR)/lwjgl_lib/
	@$(BOOTJDK)/jar -cf $@ -C $(OUTPUTDIR)/lwjgl_lib .

$(OUTPUTDIR)/patchjna_agent.jar: $(CLASSES)
	@set -e
	@mv $(basename $@)/com/sun/jna/Platform.class $(basename $@)/com/sun/jna/Platform.class.patch
	@echo "Creating $@"
	@$(BOOTJDK)/jar -cfm $@ $(SOURCEDIR)/patchjna_agent.txt -C $(basename $@) .

$(OUTPUTDIR)/runtime_patches.jar: $(CLASSES) check_empty_classes
	@set -e
	@echo "Creating $@"
	@$(BOOTJDK)/jar -cf $@ -C $(basename $@) .

clean:
	rm -rf $(OUTPUTDIR)

.SUFFIXES: .java
.PHONY: all clean
