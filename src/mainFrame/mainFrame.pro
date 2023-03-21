CONFIG += c++14
TEMPLATE = subdirs

SUBDIRS += \
    plugin \
    mainFrame

mainFrame.depends = \
    plugin

TRANSLATIONS = mainFrame_cn.ts

