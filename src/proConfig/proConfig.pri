win32 {
    include(winProConfig.pri)
}
unix: !macx {
    include(linuxProConfig.pri)
}
macx {
    include(macProConfig.pri)
}
