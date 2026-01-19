// Copyright (C) 2025 Axis Communications AB, Lund, Sweden
// Licensed under the MIT License. See LICENSE file for details.

#include <atomic>
#include <csignal>
#include <cstdarg>
#include <nexus/client.hpp>
#include <syslog.h>

using namespace axis_os_nexus;
using namespace std;

static atomic_bool terminate_application = false;

const string topic_name = "acap.object_detector";

const string topic_definition = R"(
{
    "topic_name": ")" + topic_name +
                                R"(",
    "description": "Detected objects are written to this topic",
    "version": "1.0.0",
    "data_schema": {
        "type": "object",
        "properties": {
            "object": {
                "type": "string"
            },
            "distance": {
                "type": "integer"
            }
        },
        "required": ["object"]
    }
})";

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
    terminate_application = true;
}

class ConsumerMatchListener : public TopicDataWriterListener {
  public:
    ConsumerMatchListener(shared_ptr<atomic<bool>> consumers_exist)
        : m_consumers_exist(move(consumers_exist)) {}

    virtual void OnConsumerMatchUpdate([[maybe_unused]] ProductionId id,
                                       ConsumerMatchStatus& status) override {
        if (status == ConsumerMatchStatus::MATCH) {
            syslog(LOG_INFO, "Consumers exist");
            *m_consumers_exist = true;
        } else if (status == ConsumerMatchStatus::NO_MATCH) {
            syslog(LOG_INFO, "Consumers do not exist");
            *m_consumers_exist = false;
        } else {
            // We should never come here
            syslog(LOG_WARNING, "Received invalid ConsumerMatchStatus");
        }
    }

  private:
    shared_ptr<atomic<bool>> m_consumers_exist;
};

class Application {
  public:
    Application() = default;

    ~Application() {
        // The topic is not deleted when the client disconnects, so if we want to delete it,
        // we have to use the DeleteTopic function. Subscribers that have requested topic
        // updates will get a notification that the topic has been deleted (and therefore
        // no more data will be written).
        m_client->DeleteTopic(topic_name);  // Ignore failure to delete non-existent topic

        // When the Client object is destroyed, the client is disconnected.
        // It is also possible to explicitly disconnect with the Disconnect() function.
    }

    void Initialize() {
        InitializeNexus("Client for object-detector");

        CreateTopicAndWriter("Data writer for object-detector");

        m_consumersExist = make_shared<atomic<bool>>(false);
        auto listener    = make_shared<ConsumerMatchListener>(m_consumersExist);

        SetListenerAndRegisterProduction(listener);
    }

    void Run() { PublishFakeObjectDetections(); }

  private:
    void InitializeNexus(const string& client_name) {
        ClientOptions options;
        options.log_config.level  = LogLevel::INFO;
        options.log_config.target = LogTarget::SYSLOG;
        options.dbus_bus_type     = DBusBusType::SYSTEM;

        m_client = Client::Create(client_name, options);
        m_client->Connect().value();  // Throws bad_expected_access on failure
    }

    void CreateTopicAndWriter(const string& writer_name) {
        m_client->DeleteTopic(topic_name);  // Ignore failure to delete non-existent topic
        auto topic = m_client->CreateTopic(topic_definition).value();

        m_writer = m_client->CreateTopicDataWriter(writer_name).value();
        m_writer->Initialize(topic->GetName()).value();  // Throws bad_expected_access on failure
    }

    void SetListenerAndRegisterProduction(shared_ptr<TopicDataWriterListener> listener) {
        m_writer->SetListener(listener);

        auto topic_data = TopicData::FromJson("{}").value();
        m_writer->RegisterProduction(topic_data).value();  // Throws bad_expected_access on failure

        // The production will be unregistered when the client disconnects.
        // It is also possible to explicitly unregister it with the UnregisterProduction() function.
    }

    void PublishFakeObjectDetections() {
        struct Object {
            const string type;
            int distance;
            int speed;
        };

        array<Object, 3> objects = {{{.type = "human", .distance = 100, .speed = -1},
                                     {.type = "bird", .distance = 100, .speed = +5},
                                     {.type = "dog", .distance = 100, .speed = +2}}};

        while (!terminate_application) {
            for (auto& obj : objects) {
                obj.distance += obj.speed;

                if (obj.distance <= 50 || obj.distance >= 1000) {
                    obj.speed *= -1;
                }
            }

            if (*m_consumersExist) {
                for (auto& obj : objects) {
                    string json = "{ \"object\": \"" + obj.type +
                                  "\", \"distance\": " + to_string(obj.distance) + "}";

                    TopicData topic_data = TopicData::FromJson(json).value();
                    m_writer->WriteData(topic_data, nullopt)
                        .value();  // Throws bad_expected_access on failure
                }
            }

            sleep(2);
        }
    }

  private:
    unique_ptr<Client> m_client;
    unique_ptr<TopicDataWriter> m_writer;
    shared_ptr<atomic<bool>> m_consumersExist;
};

int main() {
    syslog(LOG_INFO, "Application started");
    signal(SIGTERM, sig_handler);

    try {
        Application app = Application();
        app.Initialize();
        app.Run();
    } catch (const bad_expected_access<ApiError>& exc) {
        panic("Failed during Nexus operation: %s", exc.error().GetMessage().c_str());
    }

    syslog(LOG_INFO, "Application terminated");
    return 0;
}
