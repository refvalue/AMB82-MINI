#pragma once

#include "HttpMessage.hpp"
#include "cJSON.hpp"

#include <Client.h>
#include <DictionaryDeclarations.h>
#include <WString.h>
#include <stdint.h>

class HttpMessageServer : public HttpMessage {
public:
    HttpMessageServer(Client& client);
    ~HttpMessageServer();
    bool hasContentLength() override;
    uint32_t contentLength() override;
    void setContentLength(uint32_t value) override;
    String contentType() override;
    void setContentType(const String& value) override;
    String transferEncoding() override;
    void setTransferEncoding(const String& value) override;
    String getHeader(const String& name) override;
    void setHeader(const String& name, const String& value) override;
    String getBody() override;
    cJSONPtr getBodyAsJson() override;
    void writeHeader(uint32_t code = 200) override;
    void writeJson(const cJSON* json, uint32_t code = 200) override;
    static void write404NotFound(Client& client);

private:
    Client& client_;
    Dictionary headers_;
};
