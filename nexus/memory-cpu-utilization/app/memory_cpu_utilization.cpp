// Copyright (C) 2025 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <csignal>
#include <cstdarg>
#include <nexus/client.hpp>
#include <nlohmann/json.hpp>
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

class ResourceUtilizationLogger : public TopicDataSubscriberListener {
  public:
    ResourceUtilizationLogger(string memory_topic, string cpu_topic)
        : m_memory_topic(move(memory_topic)), m_cpu_topic(move(cpu_topic)) {}

    virtual void OnData(unique_ptr<TopicSample> sample) override {
        if (sample->topic_name == m_memory_topic) {
            syslog(LOG_INFO,
                   "Received memory utilization message: %s",
                   sample->topic_data.ToJson().c_str());
        } else if (sample->topic_name == m_cpu_topic) {
            try {
                nlohmann::json value = nlohmann::json::parse(sample->topic_data.ToJson());
                int total            = value["total_utilization"].get<int>();
                syslog(LOG_INFO, "Received CPU utilization message. Total utilization: %d", total);
            } catch (const exception& ex) {
                panic("Error when handling received CPU data: %s", ex.what());
            }
        } else {
            panic("Received unexpected topic: %s", sample->topic_name.c_str());
        }
    }

  private:
    const string m_memory_topic;
    const string m_cpu_topic;
};

static auto initialize_nexus(const string& client_name) {
    auto client = Client::Create(client_name);
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
    (void)signal(SIGTERM, sig_handler);

    // The topics below are public. All users are allowed to read data from them.
    const string memory_topic = "axis.device.memory_utilization_v1";
    const string cpu_topic    = "axis.device.cpu_utilization_v1";

    vector<string> topics = {memory_topic, cpu_topic};

    try {
        auto client = initialize_nexus("Client for memory-cpu-utilization");

        auto logger = make_shared<ResourceUtilizationLogger>(memory_topic, cpu_topic);

        auto subscriber =
            create_subscriber_and_subscribe(*client,
                                            "Data subscriber for resource utilization",
                                            logger,
                                            topics);
        pause();
    } catch (const bad_expected_access<ApiError>& ex) {
        panic("Failed during Nexus operation: %s", ex.error().GetMessage().c_str());
    }

    return 0;
}
