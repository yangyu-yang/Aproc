#
# Andes BSP Version
#

uname_s=$(shell (uname -s) 2>/dev/null || echo unknown)

ifndef ANDESIGHT_NAME
	ANDESIGHT_NAME:="Andes BSP"
endif

ifndef TOP_DIR
	TOP_DIR:=`echo ./../`/
endif
ifndef TOOLCHAINS_BIN
	TOOLCHAINS_BIN:=$(shell ls $(TOP_DIR)toolchains/nds*/bin -d --color=never)
endif

get-version: all
all: ice-version burner-version sid-version toolchain-version print-line

ice-version:
	@if [ -f "$(TOP_DIR)ice/ICEman" ]; then \
		printf "==================================================== %27s\n" "ice"; \
		if [ -z "${ICE_CONSOLE_INFO}" ]; then \
			$(TOP_DIR)ice/ICEman --version | head -n1; \
		else \
			echo -e "${ICE_CONSOLE_INFO}"; \
		fi; \
		if [ "$(uname_s)" != "Linux" ]; then \
			if [ -f /cygdrive/c/WINDOWS/system32/cscript ]; then \
				echo "AICE-driver DLL version: `/cygdrive/c/WINDOWS/system32/cscript //nologo versionInfo.vbs %SystemDirectory%/libusb0.dll`"; \
			else \
				which cscript &>/dev/null; \
				if [ $$? -eq 0 ]; then \
					echo "AICE-driver DLL version: `cscript //nologo versionInfo.vbs %SystemDirectory%/libusb0.dll`"; \
				fi; \
			fi; \
		fi; \
		echo; \
	fi;

burner-version:
	@if [ -f "$(TOP_DIR)flash/bin/IntelJ3" ]; then \
		printf "==================================================== %27s\n" "flash/bin"; \
		$(TOP_DIR)flash/bin/IntelJ3 --version | head -n1; \
		echo; \
	fi;
	@if [ -f "$(TOP_DIR)flash/bin/PAR_burn" ]; then \
		printf "==================================================== %27s\n" "flash/bin/PAR_burn"; \
		$(TOP_DIR)flash/bin/PAR_burn --version | head -n1; \
		echo; \
	fi;
	@if [ -f "$(TOP_DIR)flash/bin/SPI_burn" ]; then \
		printf "==================================================== %27s\n" "flash/bin/SPI_burn"; \
		$(TOP_DIR)flash/bin/SPI_burn --version | head -n1; \
		echo; \
	fi;

sid-version:
	@if [ -f "$(TOP_DIR)sid/sid" ]; then \
		printf "==================================================== %27s\n" "sid"; \
		$(TOP_DIR)sid/sid --version | head -n1; \
		echo; \
	fi;

toolchain-version:
	@export NDS_ENV_PATH=$${PATH}; \
	for TOOLCHAIN_PATH in $(TOOLCHAINS_BIN); do \
		export PATH=$${TOOLCHAIN_PATH}:$${NDS_ENV_PATH}; \
		printf "==================================================== %27s\n" "$${TOOLCHAIN_PATH##*toolchains/}"  ; \
		if [ -f "$${TOOLCHAIN_PATH}/../config/nds-target-config.h" ]; then \
			CROSS_COMPILE=`cat "$${TOOLCHAIN_PATH}/../config/nds-target-config.h" |grep '#define NDS_TARGET*'| cut -d" " -f 3 `; \
		fi; \
		if [ -f "$${TOOLCHAIN_PATH}/../config/nds32-target-config.h" ]; then \
			CROSS_COMPILE=`cat "$${TOOLCHAIN_PATH}/../config/nds32-target-config.h" |grep '#define NDS32_TARGET*'| cut -d" " -f 3 `; \
		fi; \
		CROSS_COMPILE="$$CROSS_COMPILE"- ;\
		# printf "%s %-18s %s\n" "------------------------------" "$${CROSS_COMPILE}gcc" "------------------------------"; \
		$${CROSS_COMPILE}gcc --version | head -n1; \
		# printf "%s %-18s %s\n" "------------------------------" "$${CROSS_COMPILE}gdb" "------------------------------"; \
		$${CROSS_COMPILE}gdb --version 2> /dev/null | grep "^GNU gdb" | head -n1; \
		echo; \
	done
	@export PATH=$${NDS_ENV_PATH}

make-version:
	@printf "==================================================== %27s\n" "make"
	@make --version | head -n1
	@echo

print-line:
	@echo "================================================================================"

.PHONY: all bsp-version ice-version burner-version sid-version toolchain-version make-version print-line
