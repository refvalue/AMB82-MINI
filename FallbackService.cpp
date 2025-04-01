#include "HttpMessageServer.hpp"
#include "HttpService.hpp"
#include "Resources.hpp"

#include <Client.h>
#include <WString.h>

namespace {
    struct FallbackService : HttpService {
        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            const auto index  = methodPath.indexOf(' ');
            const auto method = methodPath.substring(0, index);
            const auto path   = methodPath.substring(index + 1);

            auto writeResponse = [&](const char* content, const char* type = "text/html") {
                HttpMessageServer response{client};

                response.setContentType(type);
                response.setHeader("Connection", "close");
                response.writeHeader();
                client.println(content);
            };

            if (method == "GET") {
                if (path == "/") {
                    return writeResponse(systemInfoHtml);
                }

                if (path == "/system-info.html") {
                    return writeResponse(systemInfoHtml);
                }

                if (path == "/schedule.html") {
                    return writeResponse(scheduleHtml);
                }

                if (path == "/live-streaming.html") {
                    return writeResponse(liveStreamingHtml);
                }

                if (path.endsWith("styles.css")) {
                    return writeResponse(stylesCss, "text/css");
                }

                if (path.endsWith("jmuxer.js")) {
                    return writeResponse(jMuxerScript, "text/javascript");
                }
            }

            return HttpMessageServer::write404NotFound(client);
        }
    };
} // namespace

auto&& fallbackService = []() -> HttpService& {
    static FallbackService service;

    return service;
}();
