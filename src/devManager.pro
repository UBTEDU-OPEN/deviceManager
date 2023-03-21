include(proConfig/proConfig.pri)

CONFIG += c++14
TEMPLATE = subdirs

SUBDIRS += \
    launcher \
    utils \
    mainFrame \
    plugins

mainFrame.depends = \
    utils

plugins.depends = \
    utils \
    mainFrame

launcher.depends = \
    utils
