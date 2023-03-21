greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET_ARCH=$${QT_ARCH}
} else {
    TARGET_ARCH=$${QMAKE_HOST.arch}
}

contains(TARGET_ARCH, x86_64) {
#    message("64 bit")
    PLATFORM = win64
} else {
#    message("32 bit")
    PLATFORM = win32
}

CONFIG(release, debug|release) {
#    message("release")
    BUILD_MODE = release
} else {
#    message("debug")
    BUILD_MODE = debug
}

LAUNCHER_VERSION = 1.0.0.6
MAINFRAME_VERSION = 1.2.0.6

BASE      = $$PWD/../..
BIN       = $${BASE}/bin
SRC       = $${BASE}/src
TRD       = $${SRC}/trd
UTILS     = $${BIN}/$${PLATFORM}/$${BUILD_MODE}/utils
LAUNCHER  = $${BIN}/$${PLATFORM}/$${BUILD_MODE}/launcher/$${LAUNCHER_VERSION}
MAINFRAME = $${BIN}/$${PLATFORM}/$${BUILD_MODE}/mainFrame/$${MAINFRAME_VERSION}
PLUGINS   = $${BIN}/$${PLATFORM}/$${BUILD_MODE}/plugins
CONFIGS   = $${BIN}/$${PLATFORM}/$${BUILD_MODE}/configs

cache(BUILD_MODE, set, BUILD_MODE)
cache(PLATFORM, set, PLATFORM)
cache(BASE, set, BASE)
cache(BIN, set, BIN)
cache(SRC, set, SRC)
cache(TRD, set, TRD)
cache(UTILS, set, UTILS)
cache(LAUNCHER, set, LAUNCHER)
cache(MAINFRAME, set, MAINFRAME)
cache(PLUGINS, set, PLUGINS)
cache(CONFIGS, set, CONFIGS)
cache(LAUNCHER_VERSION, set, LAUNCHER_VERSION)
cache(MAINFRAME_VERSION, set, MAINFRAME_VERSION)

export(BUILD_MODE)
export(PLATFORM)
export(BASE)
export(SRC)
export(TRD)
export(UTILS)
export(LAUNCHER)
export(MAINFRAME)
export(PLUGINS)
export(CONFIGS)
export(LAUNCHER_VERSION)
export(MAINFRAME_VERSION)

