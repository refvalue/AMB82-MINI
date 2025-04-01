#include "HttpMessageServer.hpp"
#include "Resources.hpp"

#include <string.h>

namespace {
    Dictionary statusCodeDescriptionMapping = [] {
        Dictionary result;

        result("100", "Continue");
        result("101", "Switching Protocols");
        result("102", "Processing");
        result("200", "OK");
        result("201", "Created");
        result("202", "Accepted");
        result("203", "Non-Authoritative Information");
        result("204", "No Content");
        result("205", "Reset Content");
        result("206", "Partial Content");
        result("207", "Multi-Status");
        result("208", "Already Reported");
        result("226", "IM Used");
        result("300", "Multiple Choices");
        result("301", "Moved Permanently");
        result("302", "Found");
        result("303", "See Other");
        result("304", "Not Modified");
        result("305", "Use Proxy");
        result("306", "Switch Proxy");
        result("307", "Temporary Redirect");
        result("308", "Permanent Redirect");
        result("400", "Bad Request");
        result("401", "Unauthorized");
        result("402", "Payment Required");
        result("403", "Forbidden");
        result("404", "Not Found");
        result("405", "Method Not Allowed");
        result("406", "Not Acceptable");
        result("407", "Proxy Authentication Required");
        result("408", "Request Timeout");
        result("409", "Conflict");
        result("410", "Gone");
        result("411", "Length Required");
        result("412", "Precondition Failed");
        result("413", "Payload Too Large");
        result("414", "URI Too Long");
        result("415", "Unsupported Media Type");
        result("416", "Range Not Satisfiable");
        result("417", "Expectation Failed");
        result("418", "I'm a teapot");
        result("421", "Misdirected Request");
        result("422", "Unprocessable Entity");
        result("423", "Locked");
        result("424", "Failed Dependency");
        result("425", "Too Early");
        result("426", "Upgrade Required");
        result("427", "Precondition Required");
        result("428", "Too Many Requests");
        result("429", "Request Header Fields Too Large");
        result("431", "Unavailable For Legal Reasons");
        result("451", "Unavailable For Legal Reasons");
        result("500", "Internal Server Error");
        result("501", "Not Implemented");
        result("502", "Bad Gateway");
        result("503", "Service Unavailable");
        result("504", "Gateway Timeout");
        result("505", "HTTP Version Not Supported");
        result("506", "Variant Also Negotiates");
        result("507", "Insufficient Storage");
        result("508", "Loop Detected");
        result("510", "Not Extended");
        result("511", "Network Authentication Required");

        return result;
    }();

    constexpr char contentLengthName[]    = "Content-Length";
    constexpr char contentTypeName[]      = "Content-Type";
    constexpr char transferEncodingName[] = "Transfer-Encoding";
} // namespace

HttpMessageServer::HttpMessageServer(Client& client) : client_{client} {}

HttpMessageServer::~HttpMessageServer() = default;

bool HttpMessageServer::hasContentLength() {
    return headers_(contentLengthName);
}

uint32_t HttpMessageServer::contentLength() {
    return headers_[contentLengthName].toInt();
}

void HttpMessageServer::setContentLength(uint32_t value) {
    headers_(contentLengthName, String{value});
}

String HttpMessageServer::contentType() {
    return headers_[contentTypeName];
}

void HttpMessageServer::setContentType(const String& value) {
    headers_(contentTypeName, value);
}

String HttpMessageServer::transferEncoding() {
    return headers_[transferEncodingName];
}

void HttpMessageServer::setTransferEncoding(const String& value) {
    headers_(transferEncodingName, value);
}

String HttpMessageServer::getHeader(const String& name) {
    return headers_[name];
}

void HttpMessageServer::setHeader(const String& name, const String& value) {
    headers_(name, value);
}

String HttpMessageServer::getBody() {
    String reuslt;

    // TODO: implements transfer-encoding support.
    if (hasContentLength()) {
        const auto size = contentLength();

        while (client_.connected()) {
            if (client_.available()) {
                reuslt += static_cast<char>(client_.read());

                if (reuslt.length() >= size) {
                    break;
                }
            }
        }
    }

    return reuslt;
}

cJSONPtr HttpMessageServer::getBodyAsJson() {
    const auto body = getBody();

    return cJSONPtr{cJSON_ParseWithLength(body.c_str(), body.length())};
}

void HttpMessageServer::writeHeader(uint32_t code) {
    const String codeStr{code};

    client_.print("HTTP/1.1 ");
    client_.print(codeStr);
    client_.print(" ");
    client_.println(statusCodeDescriptionMapping[codeStr]);

    for (size_t i = 0; i < headers_.count(); i++) {
        client_.print(headers_.key(i));
        client_.print(": ");
        client_.println(headers_.value(i));
    }

    client_.println();
}

void HttpMessageServer::writeJson(const cJSON* json, uint32_t code) {
    const auto jsonStr = cJSON_PrintUnformatted(json);

    setContentType("application/json");
    setContentLength(strlen(jsonStr));
    writeHeader(code);

    client_.print(jsonStr);
    cJSON_free(jsonStr);
}

void HttpMessageServer::write404NotFound(Client& client) {
    HttpMessageServer response{client};

    response.setContentType("text/html");
    response.setHeader("Connection", "close");
    response.writeHeader(404);

    client.print(notFoundHtml);
}
