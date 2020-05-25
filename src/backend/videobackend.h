#ifndef CHIPPUHACHI_VIDEOBACKEND_H
#define CHIPPUHACHI_VIDEOBACKEND_H

#include <string>

struct videobackendResult {
    bool isSuccess = true;
    std::string errorMessage{};
    int errorCode{};

public:
    static videobackendResult *createSuccessful() {
        return new videobackendResult();
    }

    static videobackendResult *createWithError(const std::string &message, int code = -1) {
        auto result = new videobackendResult();

        result->errorMessage = message;
        result->errorCode = code;
        result->isSuccess = false;

        return result;
    }
};

class videobackend {
public:
    virtual void init(int width, int height, const char* appName) = 0;
    virtual videobackendResult *run() = 0;
};


#endif
