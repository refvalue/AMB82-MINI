#include "HttpMessage.hpp"
#include "HttpMessageServer.hpp"
#include "HttpServer.hpp"
#include "HttpService.hpp"

namespace {
    constexpr char httpHeaderTail[] = " HTTP/1.1";

    String getRequestMethodPath(const String& line) {
        const auto queryIndex = line.indexOf('?');
        const auto size       = queryIndex > -1
                                  ? queryIndex
                                  : (line.length() - (line.endsWith(httpHeaderTail) ? (sizeof(httpHeaderTail) - 1) : 0));

        return line.substring(0, size);
    }

    HttpService* getHttpService(const String& line, String& methodPath, Dictionary& services) {
        methodPath = getRequestMethodPath(line);

        Serial.print("HTTP Request: ");
        Serial.println(line);
        Serial.print("Method Path: ");
        Serial.println(methodPath);

        if (const auto str = services[methodPath]; str.length() > 0) {
            return reinterpret_cast<HttpService*>(str.toInt());
        }

        return nullptr;
    }

    void parseAddHeader(const String& line, HttpMessage& message) {
        const auto index = line.indexOf(":");
        auto name        = line.substring(0, index);
        auto value       = line.substring(index + 1);

        name.trim();
        value.trim();

        message.setHeader(name, value);
    }
} // namespace

HttpServer::HttpServer(uint16_t port) : fallbackService_{}, impl_{port}, task_{} {}

HttpServer::~HttpServer() = default;

void HttpServer::addService(const String& methodPath, HttpService* service) {
    services_(methodPath, reinterpret_cast<int32_t>(service));
}

void HttpServer::setFallbackService(HttpService* service) {
    fallbackService_ = service;
}

void HttpServer::start() {
    impl_.begin();
    xTaskCreate(&HttpServer::acceptRoutine, ("HttpServer" + String{reinterpret_cast<uint32_t>(this)}).c_str(), 4096,
        this, 1, &task_);
    Serial.print("Task Created: ");
    Serial.println(String{reinterpret_cast<uint32_t>(task_)});
}

void HttpServer::stop() {
    if (task_) {
        vTaskDelete(task_);
        task_ = nullptr;
    }

    impl_.stop();
}

void HttpServer::acceptRoutine(void* param) {
    auto&& self = *static_cast<HttpServer*>(param);

    for (;;) {
        auto client = self.impl_.available();

        if (!client) {
            continue;
        }

        String line;
        String methodPath;
        HttpService* service{};
        HttpMessageServer message{client};

        bool firstCr            = true;
        bool currentLineIsBlank = true;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();

                if (c == '\r') {
                    // Extracts the method and path, e.g. `GET /api/test`.
                    if (firstCr) {
                        service = getHttpService(line, methodPath, self.services_);
                        service = service ? service : self.fallbackService_;
                        firstCr = false;
                    } else {
                        parseAddHeader(line, message);
                    }

                    line = String{};
                    continue;
                }

                line += c;

                // Invokes the registered service.
                if (currentLineIsBlank && c == '\n') {
                    if (service) {
                        service->run(methodPath, message, client);
                    } else {
                        HttpMessageServer::write404NotFound(client);
                    }

                    client.flush();
                    client.stop();

                    break;
                }

                currentLineIsBlank = c == '\n';
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
}
