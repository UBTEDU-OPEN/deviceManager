#ifndef AIBOXGLOBAL_H
#define AIBOXGLOBAL_H

#include <QtCore/qglobal.h>

#if defined(AIBOXPLUGIN_LIBRARY)
#  define AIBOXPLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define AIBOXPLUGIN_EXPORT Q_DECL_IMPORT
#endif

#endif // AIBOXGLOBAL_H
