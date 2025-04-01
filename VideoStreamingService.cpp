#include "CryptoUtil.hpp"
#include "HttpMessageServer.hpp"
#include "HttpService.hpp"

#include <Client.h>
#include <VideoStream.h>
#include <WString.h>
#include <mmf2_module.h>
#include <stdint.h>

namespace {
    enum websocketLeadingBytes : uint8_t {
        leadingByteSingleFrame       = 0x81,
        leadingByteFirstSegment      = 0x01,
        leadingByteContinuousSegment = 0x00,
        leadingByteLastSegment       = 0x80,
    };

    enum liveStreamingModuleCommands {
        liveStreamingModuleCmdSetClient = MM_CMD_MODULE_BASE + 1,
        liveStreamingModuleCmdSetSendHandler,
    };

    struct liveStreamingContext {
        Client* client{};
        void (*send)(Client& client, const uint8_t* buffer, size_t size){};
    };

    void* createLiveStreamingContext(void* parent) {
        const auto context = new liveStreamingContext;

        return context;
    }

    void* destroyLiveStreamingContext(void* ptr) {
        if (const auto context = static_cast<liveStreamingContext*>(ptr)) {
            delete context;
        }

        return nullptr;
    }

    int liveStreamingControllingHandler(void* ptr, int cmd, int arg) {
        const auto context = static_cast<liveStreamingContext*>(ptr);

        switch (cmd) {
        case liveStreamingModuleCmdSetClient:
            context->client = reinterpret_cast<Client*>(arg);
            break;
        case liveStreamingModuleCmdSetSendHandler:
            context->send = reinterpret_cast<decltype(context->send)>(arg);
            break;
        };

        return {};
    }

    int liveStreamingDataHandler(void* ptr, void* input, void* output) {
        const auto context = static_cast<liveStreamingContext*>(ptr);
        const auto item    = static_cast<mm_queue_item_t*>(input);

        if (context && item && context->client && context->send) {
            context->send(*context->client, reinterpret_cast<const uint8_t*>(item->data_addr), item->size);
        }

        return {};
    }

    mm_module_t liveStreamingModule = {
        .create      = &createLiveStreamingContext,
        .destroy     = &destroyLiveStreamingContext,
        .control     = &liveStreamingControllingHandler,
        .handle      = &liveStreamingDataHandler,
        .output_type = MM_TYPE_NONE,
        .module_type = MM_TYPE_VSINK,
        .name        = "websocket live streaming module",
    };

    String makeHandshakeSignature(const String& key) {
        const auto input     = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        const auto signature = CryptoUtil::sha1(input, true);

        return signature;
    }

    void sendDataFrame(Client& client, const uint8_t* buffer, size_t size, websocketLeadingBytes leadingByte) {
        if (buffer == nullptr || size == 0) {
            return;
        }

        static constexpr uint8_t payloadSize16Bit = 126;
        static constexpr uint8_t payloadSize64Bit = 127;

        client.write(leadingByte);

        // Process different frame sizes.
        if (size < 126) {
            client.write(static_cast<uint8_t>(size));
        } else if (size >= 126 && size <= 65535) {
            client.write(payloadSize16Bit);
            client.write(static_cast<uint8_t>(size >> 8));
            client.write(static_cast<uint8_t>(size));
        } else {
            client.write(payloadSize64Bit);
            client.write(0x00);
            client.write(0x00);
            client.write(0x00);
            client.write(0x00);
            client.write(static_cast<uint8_t>(size >> 24));
            client.write(static_cast<uint8_t>(size >> 16));
            client.write(static_cast<uint8_t>(size >> 8));
            client.write(static_cast<uint8_t>(size));
        }

        client.write(buffer, size);
    }

    void sendData(Client& client, const uint8_t* buffer, size_t size, bool binary = true) {
        static constexpr size_t frameSize = 2048;
        const auto frameCount             = size / frameSize;
        const auto remainingSize          = size % frameSize;
        const auto singleFrameByte        = static_cast<websocketLeadingBytes>(leadingByteSingleFrame + binary);
        const auto firstSegmentByte       = static_cast<websocketLeadingBytes>(leadingByteFirstSegment + binary);

        if (frameCount == 0) {
            return sendDataFrame(client, buffer, remainingSize, singleFrameByte);
        }

        auto ptr = buffer;

        for (const auto ptrEnd = buffer + frameSize * (frameCount - (remainingSize == 0)); ptr < ptrEnd;
             ptr += frameSize) {
            sendDataFrame(client, ptr, frameSize, ptr == buffer ? firstSegmentByte : leadingByteContinuousSegment);
        }

        sendDataFrame(client, ptr, remainingSize == 0 ? frameSize : remainingSize,
            ptr == buffer ? singleFrameByte : leadingByteLastSegment);
    }

    void sendData(Client& client, const String& str) {
        sendData(client, reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), false);
    }

    bool processWebsocketHandshake(HttpMessage& message, Client& client) {
        // https://datatracker.ietf.org/doc/html/rfc6455
        const auto key = message.getHeader("Sec-WebSocket-Key");

        // WebSocket handshake.
        if (message.getHeader("Upgrade") != "websocket" || message.getHeader("Connection") != "Upgrade"
            || message.getHeader("Sec-WebSocket-Version") != "13" || key.length() == 0) {

            HttpMessageServer::write404NotFound(client);

            return false;
        }

        HttpMessageServer response{client};

        response.setHeader("Upgrade", "websocket");
        response.setHeader("Connection", "Upgrade");
        response.setHeader("Sec-WebSocket-Accept", makeHandshakeSignature(key));
        response.writeHeader(101);

        return true;
    }

    struct VideoStreamingService : HttpService, MMFModule {
        VideoStreamingService() {
            static constexpr auto send = static_cast<decltype(liveStreamingContext{}.send)>(
                [](Client& client, const uint8_t* buffer, size_t size) { sendData(client, buffer, size); });

            _p_mmf_context = mm_module_open(&liveStreamingModule);
            mm_module_ctrl(_p_mmf_context, liveStreamingModuleCmdSetSendHandler, reinterpret_cast<int32_t>(send));
        }

        ~VideoStreamingService() {
            if (_p_mmf_context) {
                mm_module_close(_p_mmf_context);
                _p_mmf_context = nullptr;
            }
        }

        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            if (processWebsocketHandshake(message, client)) {
                bool first = true;

                while (client.connected()) {
                    if (first) {
                        mm_module_ctrl(
                            _p_mmf_context, liveStreamingModuleCmdSetClient, reinterpret_cast<int32_t>(&client));
                        first = false;
                    }

                    vTaskDelay(5 / portTICK_PERIOD_MS);
                }

                mm_module_ctrl(_p_mmf_context, liveStreamingModuleCmdSetClient, {});
            }
        }
    };
} // namespace

auto&& videoStreamingService = []() -> HttpService& {
    static VideoStreamingService service;

    return service;
}();

auto&& videoStreamingMMFModule = dynamic_cast<MMFModule&>(videoStreamingService);
