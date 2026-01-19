// Copyright (C) 2025 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <csignal>
#include <cstdarg>
#include <nexus/client.hpp>
#include <syslog.h>

using namespace axis_os_nexus;
using namespace std;

// Print an error to syslog and exit the application if a fatal error occurs
__attribute__((noreturn)) __attribute__((format(printf, 1, 2))) static void
panic(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    vsyslog(LOG_ERR, format, arg);
    va_end(arg);
    exit(1);
}

static void sig_handler(int signum) {
    (void)signum;
    // Do nothing, just let pause() in main() return.
}

class ObjectLogger : public TopicDataSubscriberListener {
    virtual void OnData(unique_ptr<TopicSample> sample) override {
        string data = sample->topic_data.ToJson();
        syslog(LOG_INFO, "Received data: %s", data.c_str());
    }
};

static auto initialize_nexus(const string& client_name) {
    ClientOptions options;
    options.log_config.level  = LogLevel::INFO;
    options.log_config.target = LogTarget::SYSLOG;
    options.dbus_bus_type     = DBusBusType::SYSTEM;

    auto client = Client::Create(client_name, options);
    client->Connect().value();  // Throws bad_expected_access on failure
    return client;
}

static auto create_subscriber_and_subscribe(Client& client,
                                            const string& subscriber_name,
                                            shared_ptr<TopicDataSubscriberListener> listener,
                                            const vector<string>& topics) {
    auto subscriber = client.CreateTopicDataSubscriber(subscriber_name).value();
    subscriber->SetListener(listener);

    for (auto& x : topics) {
        subscriber->Subscribe(x, nullopt, false).value();  // Throws bad_expected_access on failure
    }

    return subscriber;
}

int main() {
    syslog(LOG_INFO, "Application started");
    signal(SIGTERM, sig_handler);

    vector<string> topics = {"acap.object_detector"};

    try {
        auto client = initialize_nexus("Client for object-consumer");

        auto logger = make_shared<ObjectLogger>();

        auto subscriber = create_subscriber_and_subscribe(*client,
                                                          "Data subscriber for object-consumer",
                                                          logger,
                                                          topics);
        pause();
    } catch (const bad_expected_access<ApiError>& exc) {
        panic("Failed during Nexus operation: %s", exc.error().GetMessage().c_str());
    }

    syslog(LOG_INFO, "Application terminated");
    return 0;
}
