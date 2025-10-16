#pragma once

#include "ManagedTask.hpp"

#include <cstdint>

#include <DictionaryDeclarations.h>
#include <WString.h>
#include <WiFi.h>

struct HttpService;

class HttpServer {
public:
    HttpServer(uint16_t port);
    ~HttpServer();
    void addService(const String& methodPath, HttpService* service);
    void setFallbackService(HttpService* service);
    void start();
    void stop();

private:
    static void acceptRoutine(ManagedTask::CheckStoppedHandler checkStopped, void* param);

    Dictionary services_;
    HttpService* fallbackService_;
    WiFiServer impl_;
    ManagedTask task_;
};
