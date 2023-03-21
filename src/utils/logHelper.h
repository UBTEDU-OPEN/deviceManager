#ifndef LOGHELPER_H
#define LOGHELPER_H

#include "utilsGlobal.h"

#include "glog/logging.h"

class UTILS_EXPORT LogHelper
{
public:
    LogHelper(const char* argv);
    ~LogHelper();

    static void initLog(const char* argv="");

private:
    static LogHelper *self;
};

#endif // LOGHELPER_H
