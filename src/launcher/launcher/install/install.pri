win32 {
    include(windowsInstall.pri)
}
unix: !macx {
    include(linuxInstall.pri)
}
macx {
    include(macInstall.pri)
}
