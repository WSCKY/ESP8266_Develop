#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle
#FLAVOR = release
FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef SDK_PATH
SDK_PATH = $(shell pwd)/../ESP8266_RTOS_SDK
export SDK_PATH
endif

ifndef BIN_PATH
BIN_PATH = $(shell pwd)/../Binary
export BIN_PATH
endif

#default loadbin is non-OTA
loadbin ?= 0

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	user   \
        apps_httpd  \
        tftpd  \
        upgrade

endif # } PDIR

LDDIR = $(SDK_PATH)/ld

CCFLAGS += -Os

TARGET_LDFLAGS =		\
	-nostdlib		\
	-Wl,-EL \
	--longcalls \
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -O0
endif

COMPONENTS_eagle.app.v6 = \
	user/libuser.a    \
	apps_httpd/libhttpd.a  \
        tftpd/libtftpd.a  \
        upgrade/libupgrade.a

LINKFLAGS_eagle.app.v6 = \
	-L$(SDK_PATH)/lib        \
	-Wl,--gc-sections   \
	-nostdlib	\
    -T$(LD_FILE)   \
	-Wl,--no-check-sections	\
    -u call_user_start	\
	-Wl,-static						\
	-Wl,--start-group					\
	-lcirom \
	-lcrypto	\
	-lespconn	\
	-lespnow	\
	-lfreertos	\
	-lgcc					\
	-lhal					\
	-ljson	\
	-llwip	\
	-lmain	\
	-lmesh	\
	-lmirom	\
	-lnet80211	\
	-lnopoll	\
	-lphy	\
	-lpp	\
	-lpwm	\
	-lsmartconfig	\
	-lspiffs	\
	-lssl	\
	-lwpa	\
	-lwps		\
	$(DEP_LIBS_eagle.app.v6)					\
	-Wl,--end-group

DEPENDS_eagle.app.v6 = \
                $(LD_FILE) \
                $(LDDIR)/eagle.rom.addr.v6.ld

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-DICACHE_FLASH

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)inc_httpd
sinclude $(SDK_PATH)/Makefile

.PHONY: FORCE upload loadboot loadparam loadres
FORCE:

loadboot: | $(BIN_PATH)
	@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 4MB-c1 0x00000 $(BIN_PATH)/boot_v1.7.bin

loadparam: | $(BIN_PATH)
	@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 4MB-c1 0x3FC000 $(BIN_PATH)/esp_init_data_default.bin 0x3FE000 $(BIN_PATH)/blank.bin

upload: | $(BIN_PATH)
ifeq ($(loadbin), 0)
	@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 4MB 0x00000 $(BIN_PATH)/eagle.flash.bin 0x20000 $(BIN_PATH)/eagle.irom0text.bin
else
    ifeq ($(loadbin), 1)
	@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 4MB-c1 0x01000 $(BIN_PATH)/upgrade/user1.4096.new.6.bin
    else
        ifeq ($(loadbin), 2)
		@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 4MB-c1 0x101000 $(BIN_PATH)/upgrade/user2.4096.new.6.bin
        else
		@echo "load error!!!"
        endif
    endif
endif

loadres:
	@esptool.py --port /dev/ttyUSB0 write_flash -ff 80m -fm dio --flash_size 2MB-c1 0x81000 ./html/fs/index.html 0x82000 ./html/fs/404.html 0x83000 ./html/fs/img/logo.ico 0x84000 ./html/fs/upload.html 0x85000 ./html/fs/img/header.png

