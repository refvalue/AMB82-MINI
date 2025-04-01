#pragma once

#include "cJSON.hpp"

#include <WString.h>
#include <stdint.h>

struct HttpMessage {
    virtual ~HttpMessage()                                          = default;
    virtual bool hasContentLength()                                 = 0;
    virtual uint32_t contentLength()                                = 0;
    virtual void setContentLength(uint32_t value)                   = 0;
    virtual String contentType()                                    = 0;
    virtual void setContentType(const String& value)                = 0;
    virtual String transferEncoding()                               = 0;
    virtual void setTransferEncoding(const String& value)           = 0;
    virtual String getHeader(const String& name)                    = 0;
    virtual void setHeader(const String& name, const String& value) = 0;
    virtual String getBody()                                        = 0;
    virtual cJSONPtr getBodyAsJson()                                = 0;
    virtual void writeHeader(uint32_t code = 200)                   = 0;
    virtual void writeJson(const cJSON* json, uint32_t code = 200)  = 0;
};
