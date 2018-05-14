TEMPLATE = app
TARGET = Beancash-qt
VERSION = 1.1.2.2
INCLUDEPATH += src src/json src/qt
QT += network
DEFINES += ENABLE_WALLET
DEFINES += QT_GUI BOOST_THREAD_USE_LIB BOOST_SPIRIT_THREADSAFE __NO_SYSTEM_INCLUDES
CONFIG += no_include_pwd
CONFIG += thread

#USE RELEASE=1
#USE_UPNP=- # default 1 ('-' means don't use)
#USE_O3=1  # default 0
#USE_DBUS=1 # default 1

contains(RELEASE, 1) {
    message(building in RELEASE mode)
}

win32:os2 {
    CONFIG += release
} else {
    CONFIG += debug_and_release
}

freebsd-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
*-g++-32: QMAKE_TARGET.arch = i686
*-g++-64: QMAKE_TARGET.arch = x86_64

CONFIG += static

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
}

# for boost 1.37, add -mt to the boost libraries
# use: qmake BOOST_LIB_SUFFIX=-mt
# for boost thread win32 with _win32 sufix
# use: BOOST_THREAD_LIB_SUFFIX=_win32-...
# or when linking against a specific BerkelyDB version:

BDB_LIB_SUFFIX=-5.3

# Dependency library locations can be customized with:
#    BOOST_INCLUDE_PATH, BOOST_LIB_PATH, BDB_INCLUDE_PATH,
#    BDB_LIB_PATH, OPENSSL_INCLUDE_PATH and OPENSSL_LIB_PATH respectively


linux {
	DEPS_PATH = $(HOME)/deps
	SECP256K1_LIB_PATH = src/secp256k1/.libs
	SECP256K1_INCLUDE_PATH = src/secp256k1/include
## comment below dependencies if u don't need to compile a static binary on linux
	MINIUPNPC_LIB_PATH = $$DEPS_PATH/miniupnpc
	MINIUPNPC_INCLUDE_PATH = $$DEPS_PATH
	BOOST_LIB_PATH = $$DEPS_PATH/boost_1_58_0/stage/lib
	BOOST_INCLUDE_PATH = $$DEPS_PATH/boost_1_58_0
	BDB_LIB_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
	BDB_INCLUDE_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
	OPENSSL_LIB_PATH = $$DEPS_PATH/openssl-1.0.2g
	OPENSSL_INCLUDE_PATH = $$DEPS_PATH/openssl-1.0.2g/include
}

win32 {
    BOOST_LIB_SUFFIX=-mgw49-mt-s-1_59
    BOOST_INCLUDE_PATH=C:/deps/boost_1_59_0
    BOOST_LIB_PATH=C:/deps/boost_1_59_0/stage/lib
    BDB_INCLUDE_PATH=C:/deps/db-5.3.28/build_unix
    BDB_LIB_PATH=C:/deps/db-5.3.28/build_unix
    OPENSSL_INCLUDE_PATH=C:/deps/openssl-1.0.1j/include
    OPENSSL_LIB_PATH=C:/deps/openssl-1.0.1j
    MINIUPNPC_INCLUDE_PATH=C:/deps/
    MINIUPNPC_LIB_PATH=C:/deps/miniupnpc
    QRENCODE_INCLUDE_PATH=C:/deps/qrencode-3.4.4
    QRENCODE_LIB_PATH=C:/deps/qrencode-3.4.4/.libs
}

# ----------------- #

OBJECTS_DIR = build
MOC_DIR = build
UI_DIR = build

# use: qmake "RELEASE=1"
contains(RELEASE, 1) {
    # Mac: compile for maximum compatibility (10.5, 32-bit)
    macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.9 -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
    macx:QMAKE_CFLAGS += -mmacosx-version-min=10.9 -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
    macx:QMAKE_OBJECTIVE_CFLAGS += -mmacosx-version-min=10.9 -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk

    !windows:!macx {
        # Linux: static link
        LIBS += -Wl,-Bstatic
    }
}

!win32 {
    # for extra security against potential buffer overflows: enable GCCs Stack Smashing Protection
    message(GCCs Stack Smashing protection enabled)
    QMAKE_CXXFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
    QMAKE_LFLAGS *= -fstack-protector-all --param ssp-buffer-size=1
    # We need to exclude this for Windows cross compile with MinGW 4.2.x, as it will result in a non-working executable!
    # This can be enabled for Windows, when we switch to MinGW >= 4.4.x.
}
# for extra security on Windows: enable ASLR and DEP via GCC linker flags
win32:QMAKE_LFLAGS *= -Wl,--dynamicbase -Wl,--nxcompat -static

contains(RELEASE, 1) {
    QMAKE_LFLAGS += -static-libstdc++
}

win32:contains(QMAKE_HOST.arch, x86):{QMAKE_LFLAGS *= -Wl,--large-address-aware }

# use: qmake "USE_QRCODE=1"
# libqrencode (http://fukuchi.org/works/qrencode/index.en.html) must be installed for support
contains(USE_QRCODE, 1) {
    message(Building with QRCode support)
    DEFINES += USE_QRCODE
    LIBS += -lqrencode
}

# use: qmake "USE_UPNP=1" ( enabled by default; default)
#  or: qmake "USE_UPNP=0" (disabled by default)
#  or: qmake "USE_UPNP=-" (not supported)
# miniupnpc (http://miniupnp.free.fr/files/) must be installed for support
contains(USE_UPNP, -) {
    message(Building without UPNP support)
} else {
    message(Building with UPNP support)
    count(USE_UPNP, 0) {
        USE_UPNP=1
    }
    DEFINES += USE_UPNP=$$USE_UPNP MINIUPNP_STATICLIB STATICLIB
    INCLUDEPATH += $$MINIUPNPC_INCLUDE_PATH
    LIBS += $$join(MINIUPNPC_LIB_PATH,,-L,) -lminiupnpc
    win32:LIBS += -liphlpapi
}

# use: qmake "USE_DBUS=1" or qmake "USE_DBUS=0"
isEmpty(USE_DBUS) {
    message("USE_DBUS not set, fallback to $USE_DBUS=1")
    USE_DBUS=1
}

contains(USE_DBUS, 1) {
    message(Building with DBUS (Freedesktop notifications) support)
    DEFINES += USE_DBUS
    QT += dbus
}else {
    message(Building without DBUS (Freedesktop notifications) support)
}

contains(BEANCASH_NEED_QT_PLUGINS, 1) {
    DEFINES += BEANCASH_NEED_QT_PLUGINS
    QTPLUGIN += qcncodecs qjpcodecs qtwcodecs qkrcodecs qtaccessiblewidgets
}

INCLUDEPATH += src/leveldb/include src/leveldb/helpers
LIBS += $$PWD/src/leveldb/libleveldb.a $$PWD/src/leveldb/libmemenv.a
SOURCES += src/txdb-leveldb.cpp

!win32 {
    # we use QMAKE_CXXFLAGS_RELEASE even without RELEASE=1 because we use RELEASE to indicate linking preferences not -O preferences
    genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a
} else {
    # make an educated guess about what the ranlib command is called
    isEmpty(QMAKE_RANLIB) {
        QMAKE_RANLIB = $$replace(QMAKE_STRIP, strip, ranlib)
    }
    LIBS += -lshlwapi
    !win32 {genleveldb.commands = cd $$PWD/src/leveldb && CC=$$QMAKE_CC CXX=$$QMAKE_CXX TARGET_OS=OS_WINDOWS_CROSSCOMPILE $(MAKE) OPT=\"$$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE\" libleveldb.a libmemenv.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libleveldb.a && $$QMAKE_RANLIB $$PWD/src/leveldb/libmemenv.a}
}
genleveldb.target = $$PWD/src/leveldb/libleveldb.a
genleveldb.depends = FORCE
PRE_TARGETDEPS += $$PWD/src/leveldb/libleveldb.a
QMAKE_EXTRA_TARGETS += genleveldb
# Gross ugly hack that depends on qmake internals, unfortunately there is no other way to do it.
QMAKE_CLEAN += $$PWD/src/leveldb/libleveldb.a; cd $$PWD/src/leveldb ; $(MAKE) clean

# regenerate src/build.h
!win32|contains(USE_BUILD_INFO, 1) {
    genbuild.depends = FORCE
    genbuild.commands = cd $$PWD; /bin/sh share/genbuild.sh $$OUT_PWD/build/build.h
    genbuild.target = $$OUT_PWD/build/build.h
    PRE_TARGETDEPS += $$OUT_PWD/build/build.h
    QMAKE_EXTRA_TARGETS += genbuild
    DEFINES += HAVE_BUILD_INFO
}

contains(USE_O3, 1) {
    message(Building with O3 optimization flag)
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS += -O3
    QMAKE_CFLAGS += -O3
}

contains(QMAKE_TARGET.arch, i386) | contains(QMAKE_TARGET.arch, i686) {
    message("x86 platform, adding -msse2 & -mssse3 flags")
    QMAKE_CXXFLAGS += -msse2 -mssse3
    QMAKE_CFLAGS += -msse2 -mssse3
}

contains(QMAKE_TARGET.arch, x86_64) | contains(QMAKE_TARGET.arch, amd64) {
    message("x86_64 platform, setting -mssse3 flag")
    QMAKE_CXXFLAGS += -mssse3
    QMAKE_CFLAGS += -mssse3
}

QMAKE_CXXFLAGS_WARN_ON = -fdiagnostics-show-option -Wall -Wextra -Wno-ignored-qualifiers -Wformat -Wformat-security -Wno-unused-parameter -Wstack-protector

# Input
DEPENDPATH += src src/json src/qt
HEADERS += src/qt/bitbeangui.h \
    src/qt/transactiontablemodel.h \
    src/qt/addresstablemodel.h \
    src/qt/optionsdialog.h \
    src/qt/beancontroldialog.h \
    src/qt/beancontroltreewidget.h \
    src/qt/sendbeansdialog.h \
    src/qt/addressbookpage.h \
    src/qt/signverifymessagedialog.h \
    src/qt/aboutdialog.h \
    src/qt/editaddressdialog.h \
    src/qt/bitbeanaddressvalidator.h \
    src/alert.h \
    src/addrman.h \
    src/base58.h \
    src/bignum.h \
    src/checkpoints.h \
    src/compat.h \
    src/beancontrol.h \
    src/sync.h \
    src/util.h \
    src/uint256.h \
    src/kernel.h \
    src/scrypt.h \
    src/pbkdf2.h \
    src/serialize.h \
    src/strlcpy.h \
    src/main.h \
    src/miner.h \
    src/net.h \
    src/key.h \
    src/db.h \
    src/txdb.h \
    src/walletdb.h \
    src/script.h \
    src/init.h \
    src/mruset.h \
    src/json/json_spirit_writer_template.h \
    src/json/json_spirit_writer.h \
    src/json/json_spirit_value.h \
    src/json/json_spirit_utils.h \
    src/json/json_spirit_stream_reader.h \
    src/json/json_spirit_reader_template.h \
    src/json/json_spirit_reader.h \
    src/json/json_spirit_error_position.h \
    src/json/json_spirit.h \
    src/qt/clientmodel.h \
    src/qt/guiutil.h \
    src/qt/transactionrecord.h \
    src/qt/guiconstants.h \
    src/qt/optionsmodel.h \
    src/qt/monitoreddatamapper.h \
    src/qt/transactiondesc.h \
    src/qt/transactiondescdialog.h \
    src/qt/bitbeanamountfield.h \
    src/wallet.h \
    src/keystore.h \
    src/qt/transactionfilterproxy.h \
    src/qt/transactionview.h \
    src/qt/walletmodel.h \
    src/bitbeanrpc.h \
    src/qt/overviewpage.h \
    src/qt/csvmodelwriter.h \
    src/crypter.h \
    src/qt/sendbeansentry.h \
    src/qt/qvalidatedlineedit.h \
    src/qt/bitbeanunits.h \
    src/qt/qvaluecombobox.h \
    src/qt/askpassphrasedialog.h \
    src/protocol.h \
    src/qt/notificator.h \
    src/qt/qtipcserver.h \
    src/allocators.h \
    src/ui_interface.h \
    src/qt/rpcconsole.h \
    src/version.h \
    src/netbase.h \
    src/clientversion.h \
    src/qt/macnotificationhandler.h

SOURCES += src/qt/beancash.cpp src/qt/bitbeangui.cpp \
    src/qt/transactiontablemodel.cpp \
    src/qt/addresstablemodel.cpp \
    src/qt/optionsdialog.cpp \
    src/qt/sendbeansdialog.cpp \
    src/qt/beancontroldialog.cpp \
    src/qt/beancontroltreewidget.cpp \
    src/qt/addressbookpage.cpp \
    src/qt/signverifymessagedialog.cpp \
    src/qt/aboutdialog.cpp \
    src/qt/editaddressdialog.cpp \
    src/qt/bitbeanaddressvalidator.cpp \
    src/alert.cpp \
    src/version.cpp \
    src/sync.cpp \
    src/util.cpp \
    src/netbase.cpp \
    src/key.cpp \
    src/script.cpp \
    src/main.cpp \
    src/miner.cpp \
    src/init.cpp \
    src/net.cpp \
    src/checkpoints.cpp \
    src/addrman.cpp \
    src/db.cpp \
    src/walletdb.cpp \
    src/qt/clientmodel.cpp \
    src/qt/guiutil.cpp \
    src/qt/transactionrecord.cpp \
    src/qt/optionsmodel.cpp \
    src/qt/monitoreddatamapper.cpp \
    src/qt/transactiondesc.cpp \
    src/qt/transactiondescdialog.cpp \
    src/qt/bitbeanstrings.cpp \
    src/qt/bitbeanamountfield.cpp \
    src/wallet.cpp \
    src/keystore.cpp \
    src/qt/transactionfilterproxy.cpp \
    src/qt/transactionview.cpp \
    src/qt/walletmodel.cpp \
    src/bitbeanrpc.cpp \
    src/rpcdump.cpp \
    src/rpcnet.cpp \
    src/rpcmining.cpp \
    src/rpcwallet.cpp \
    src/rpcblockchain.cpp \
    src/rpcrawtransaction.cpp \
    src/qt/overviewpage.cpp \
    src/qt/csvmodelwriter.cpp \
    src/crypter.cpp \
    src/qt/sendbeansentry.cpp \
    src/qt/qvalidatedlineedit.cpp \
    src/qt/bitbeanunits.cpp \
    src/qt/qvaluecombobox.cpp \
    src/qt/askpassphrasedialog.cpp \
    src/protocol.cpp \
    src/qt/notificator.cpp \
    src/qt/qtipcserver.cpp \
    src/qt/rpcconsole.cpp \
    src/noui.cpp \
    src/kernel.cpp \
    src/scrypt-arm.S \
    src/scrypt-x86.S \
    src/scrypt-x86_64.S \
    src/scrypt.cpp \
    src/pbkdf2.cpp \

RESOURCES += \
    src/qt/beancash.qrc

FORMS += \
    src/qt/forms/beancontroldialog.ui \
    src/qt/forms/sendbeansdialog.ui \
    src/qt/forms/addressbookpage.ui \
    src/qt/forms/signverifymessagedialog.ui \
    src/qt/forms/aboutdialog.ui \
    src/qt/forms/editaddressdialog.ui \
    src/qt/forms/transactiondescdialog.ui \
    src/qt/forms/overviewpage.ui \
    src/qt/forms/sendbeansentry.ui \
    src/qt/forms/askpassphrasedialog.ui \
    src/qt/forms/rpcconsole.ui \
    src/qt/forms/optionsdialog.ui \

contains(USE_QRCODE, 1) {
    HEADERS += src/qt/qrcodedialog.h
    SOURCES += src/qt/qrcodedialog.cpp
    FORMS += src/qt/forms/qrcodedialog.ui
}

contains(BEANCASH_QT_TEST, 1) {
    SOURCES += src/qt/test/test_main.cpp \
        src/qt/test/uritests.cpp
    HEADERS += src/qt/test/uritests.h
    DEPENDPATH += src/qt/test
    QT += testlib
    TARGET = Beancash-qt_test
    DEFINES += BEANCASH_QT_TEST
}

CODECFORTR = UTF-8

# for lrelease/lupdate
# also add new translations to src/qt/beancash.qrc under translations/
TRANSLATIONS = $$files(src/qt/locale/beancash_*.ts)

isEmpty(QMAKE_LRELEASE) {
    win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}
isEmpty(QM_DIR):QM_DIR = $$PWD/src/qt/locale

# automatically build translations, so they can be included in resource file
TSQM.name = lrelease ${QMAKE_FILE_IN}
TSQM.input = TRANSLATIONS
TSQM.output = $$QM_DIR/${QMAKE_FILE_BASE}.qm
TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
TSQM.CONFIG = no_link
QMAKE_EXTRA_COMPILERS += TSQM

# "Other files" to show in Qt Creator
OTHER_FILES += \
    doc/*.rst doc/*.txt doc/README README.md res/Beancash-qt.rc

# platform specific defaults, if not overridden on command line
!exists(BOOST_LIB_SUFFIX) {
    macx{
        message(since BOOST_LIB_SUFFIX is empty we add to BOOST_LIB_SUFFIX -mt)
        BOOST_LIB_SUFFIX = -mt
    }
    windows{
        message(since BOOST_LIB_SUFFIX is empty we add to BOOST_LIB_SUFFIX -mgw49-mt-s-1_59)
        BOOST_LIB_SUFFIX = -mgw49-mt-s-1_59
    }
}

# usefull debugging
macx{
    !exists(DEPSDIR) {
        # Uses Homebrew if installed, otherwise defaults to MacPorts
        check_dir = /usr/local/Cellar
        exists($$check_dir) {
            message(DEPSIDR its empty, falling back to /usr/local)
            DEPSDIR = /usr/local
        }
        !exists($$check_dir) {
            message(DEPSIDR its empty (ando neither $$check_dir its), falling back to /upt/local)
            DEPSDIR = /opt/local
        }
    }
}

!exists(BOOST_LIB_PATH) {
    message(BOOST_LIB_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/lib)
        BOOST_LIB_PATH = $$DEPSDIR/lib
    }
}

!exists(BOOST_INCLUDE_PATH) {
    message(BOOST_INCLUDE_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/include)
        BOOST_INCLUDE_PATH = $$DEPSDIR/include
    }
}

!exists(BDB_LIB_PATH) {
    message(BDB_LIB_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/lib)
        BDB_LIB_PATH = $$DEPSDIR/lib
    }
}

!exists(BDB_INCLUDE_PATH) {
    message(BDB_INCLUDE_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/include)
        BDB_INCLUDE_PATH = $$DEPSDIR/include
    }
}

!exists(OPENSSL_LIB_PATH) {
    message(OPENSSL_LIB_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/lib)
        OPENSSL_LIB_PATH = $$DEPSDIR/lib
    }
}

!exists(OPENSSL_INCLUDE_PATH) {
    message(OPENSSL_INCLUDE_PATH point to an empty folder)
    macx: {
        message(falling back to: $DEPSDIR/include)
        OPENSSL_INCLUDE_PATH = $$DEPSDIR/include
    }
}

macx: {
    HEADERS += src/qt/macdockiconhandler.h src/qt/macnotificationhandler.h
    OBJECTIVE_SOURCES += src/qt/macdockiconhandler.mm src/qt/macnotificationhandler.mm

    contains(USE_QRCODE, 1){
        !exists(QRCODE_LIB_PATH) {
            message(QRCODE_LIB_PATH point to an empty folder, falling back to: $DEPSDIR/lib)
            QRCODE_LIB_PATH = $$DEPSDIR/lib
        }
        !exists(QRCODE_INCLUDE_PATH) {
            message(QRCODE_INCLUDE_PATH point to an empty folder, falling back to: $DEPSDIR/include)
            QRCODE_INCLUDE_PATH = $$DEPSDIR/include
        }
    }

    contains(RELEASE, 1) {
        LIBS += -framework Foundation -framework ApplicationServices -framework AppKit -framework CoreServices \
        $$BDB_LIB_PATH/libdb_cxx$$BDB_LIB_SUFFIX.a \
        $$BOOST_LIB_PATH/libboost_system.a \
        $$BOOST_LIB_PATH/libboost_filesystem.a \
        $$BOOST_LIB_PATH/libboost_program_options.a \
        $$BOOST_LIB_PATH/libboost_thread.a \
        $$BOOST_LIB_PATH/libboost_chrono.a \
        $$OPENSSL_LIB_PATH/libssl.a \
        $$OPENSSL_LIB_PATH/libcrypto.a
     } else {
        LIBS += -framework Foundation -framework ApplicationServices -framework AppKit -framework CoreServices \
        $$BDB_LIB_PATH/libdb_cxx$$BDB_LIB_SUFFIX.dylib \
        $$BOOST_LIB_PATH/libboost_system-mt.dylib \
        $$BOOST_LIB_PATH/libboost_filesystem-mt.dylib \
        $$BOOST_LIB_PATH/libboost_program_options-mt.dylib \
        $$BOOST_LIB_PATH/libboost_thread-mt.dylib \
        $$BOOST_LIB_PATH/libboost_chrono-mt.dylib \
        $$OPENSSL_LIB_PATH/libssl.a \
        $$OPENSSL_LIB_PATH/libcrypto.a
    }

    DEFINES += MAC_OSX MSG_NOSIGNAL=0
    # osx 10.9 has changed the stdlib default to libc++. To prevent some link error, you may need to use libstdc++
    QMAKE_CXXFLAGS += -stdlib=libstdc++

    ICON = src/qt/res/icons/beancash.icns
    TARGET = "Beancash-Qt"

    QMAKE_CFLAGS_THREAD += -pthread
    QMAKE_CXXFLAGS_THREAD += -pthread
}

windows:DEFINES += WIN32
windows:RC_FILE = src/qt/res/Beancash-qt.rc

windows:!contains(MINGW_THREAD_BUGFIX, 0) {
    # At least qmake's win32-g++-cross profile is missing the -lmingwthrd
    # thread-safety flag. GCC has -mthreads to enable this, but it doesn't
    # work with static linking. -lmingwthrd must come BEFORE -lmingw, so
    # it is prepended to QMAKE_LIBS_QT_ENTRY.
    # It can be turned off with MINGW_THREAD_BUGFIX=0, just in case it causes
    # any problems on some untested qmake profile now or in the future.
    DEFINES += _MT BOOST_THREAD_PROVIDES_GENERIC_SHARED_MUTEX_ON_WIN
    QMAKE_LIBS_QT_ENTRY = -lmingwthrd $$QMAKE_LIBS_QT_ENTRY
}

# Set libraries and includes at end, to use platform-defined defaults if not overridden
INCLUDEPATH += $$BOOST_INCLUDE_PATH $$BDB_INCLUDE_PATH $$OPENSSL_INCLUDE_PATH $$QRENCODE_INCLUDE_PATH
LIBS += $$join(BOOST_LIB_PATH,,-L,) $$join(BDB_LIB_PATH,,-L,) $$join(OPENSSL_LIB_PATH,,-L,) $$join(QRENCODE_LIB_PATH,,-L,)
LIBS += -lssl -lcrypto -ldb_cxx$$BDB_LIB_SUFFIX
# -lgdi32 has to happen after -lcrypto (see  #681)
windows:LIBS += -lws2_32 -lshlwapi -lmswsock -lole32 -loleaut32 -luuid -lgdi32
LIBS += -lboost_system$$BOOST_LIB_SUFFIX -lboost_filesystem$$BOOST_LIB_SUFFIX -lboost_program_options$$BOOST_LIB_SUFFIX -lboost_thread$$BOOST_LIB_SUFFIX
windows:LIBS += -lboost_chrono$$BOOST_LIB_SUFFIX

contains(RELEASE, 1) {
    !windows:!macx {
        # Linux: turn dynamic linking back on for c/c++ runtime libraries
        LIBS += -Wl,-Bdynamic
    }
}

!windows:!macx {
    DEFINES += LINUX
    LIBS += -lrt -ldl
}

netbsd-*|freebsd-*|openbsd-* {
    # libexecinfo is needed for back trace
    LIBS += -lexecinfo
}

system($$QMAKE_LRELEASE -silent $$_PRO_FILE_)
