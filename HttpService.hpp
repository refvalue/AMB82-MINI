#pragma once

#include "HttpMessage.hpp"

#include <Client.h>
#include <WString.h>

struct HttpService {
    virtual ~HttpService()                                                           = default;
    virtual void run(const String& methodPath, HttpMessage& message, Client& client) = 0;
};
