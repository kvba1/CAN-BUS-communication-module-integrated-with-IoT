// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// CAN libraries
#include <mcp_can.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Additional sample headers
#include "AzIoTSasToken.h"
#include "SerialLogger.h"
#include "iot_configs.h"

#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

// CAN variables
const int SPI_CS_PIN = 2;
const int CAN_INT_PIN = 4;
MCP_CAN CAN0(SPI_CS_PIN);

void canBusReaderTask(void *pvParameters);
void wifiMqttTask(void *pvParameters);

// Global variables for shared data (use mutexes for synchronization if needed)
volatile float vehicleSpeed = 0.0;
volatile float engineRPM = 0.0;
volatile float coolantTemp = 0.0;
volatile float oilTemp = 0.0;
volatile float throttlePosition = 0.0;
volatile bool engineStart = false;

byte speedRequest[] = {0x02, 0x01, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00};
byte rpmRequest[] = {0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00};
byte coolantTempRequest[] = {0x02, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
byte oilTempRequest[] = {0x02, 0x01, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00};
byte throttlePositionRequest[] = {0x02, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00};

// Translate iot_configs.h defines into variables used by the sample
static const char *ssid = IOT_CONFIG_WIFI_SSID;
static const char *password = IOT_CONFIG_WIFI_PASSWORD;
static const char *host = IOT_CONFIG_IOTHUB_FQDN;
static const char *mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char *device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static unsigned long next_telemetry_send_time_ms = 0;
static char telemetry_topic[128];
static uint32_t telemetry_send_count = 0;
static String telemetry_payload = "{}";

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

// Auxiliary functions
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

static void connectToWiFi()
{
  Logger.Info("Connecting to WIFI SSID " + String(ssid));

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Logger.Info("WiFi connected, IP address: " + WiFi.localIP().toString());
}

static void initializeTime()
{
  Logger.Info("Setting time using SNTP");

  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  Logger.Info("Time initialized!");
}

void receivedCallback(char *topic, byte *payload, unsigned int length)
{
  Logger.Info("Received [");
  Logger.Info(topic);
  Logger.Info("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
    int i, r;

  case MQTT_EVENT_ERROR:
    Logger.Info("MQTT event MQTT_EVENT_ERROR");
    break;
  case MQTT_EVENT_CONNECTED:
    Logger.Info("MQTT event MQTT_EVENT_CONNECTED");

    r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
    if (r == -1)
    {
      Logger.Error("Could not subscribe for cloud-to-device messages.");
    }
    else
    {
      Logger.Info("Subscribed for cloud-to-device messages; message id:" + String(r));
    }

    break;
  case MQTT_EVENT_DISCONNECTED:
    Logger.Info("MQTT event MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    Logger.Info("MQTT event MQTT_EVENT_SUBSCRIBED");
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    Logger.Info("MQTT event MQTT_EVENT_UNSUBSCRIBED");
    break;
  case MQTT_EVENT_PUBLISHED:
    Logger.Info("MQTT event MQTT_EVENT_PUBLISHED");
    break;
  case MQTT_EVENT_DATA:
    Logger.Info("MQTT event MQTT_EVENT_DATA");

    for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
    {
      incoming_data[i] = event->topic[i];
    }
    incoming_data[i] = '\0';
    Logger.Info("Topic: " + String(incoming_data));

    for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
    {
      incoming_data[i] = event->data[i];
    }
    incoming_data[i] = '\0';
    Logger.Info("Data: " + String(incoming_data));

    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    Logger.Info("MQTT event MQTT_EVENT_BEFORE_CONNECT");
    break;
  default:
    Logger.Error("MQTT event UNKNOWN");
    break;
  }

  return ESP_OK;
}

static void initializeIoTHubClient()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t *)host, strlen(host)),
          az_span_create((uint8_t *)device_id, strlen(device_id)),
          &options)))
  {
    Logger.Error("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Logger.Error("Failed getting client id");
    return;
  }

  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    Logger.Error("Failed to get MQTT clientId, return code");
    return;
  }

  Logger.Info("Client ID: " + String(mqtt_client_id));
  Logger.Info("Username: " + String(mqtt_username));
}

static int initializeMqttClient()
{
#ifndef IOT_CONFIG_USE_X509_CERT
  if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
  {
    Logger.Error("Failed generating SAS token");
    return 1;
  }
#endif

  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = mqtt_broker_uri;
  mqtt_config.port = mqtt_port;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;

#ifdef IOT_CONFIG_USE_X509_CERT
  Logger.Info("MQTT client using X509 Certificate authentication");
  mqtt_config.client_cert_pem = IOT_CONFIG_DEVICE_CERT;
  mqtt_config.client_key_pem = IOT_CONFIG_DEVICE_CERT_PRIVATE_KEY;
#else // Using SAS key
  mqtt_config.password = (const char *)az_span_ptr(sasToken.Get());
#endif

  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqtt_event_handler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char *)ca_pem;

  mqtt_client = esp_mqtt_client_init(&mqtt_config);

  if (mqtt_client == NULL)
  {
    Logger.Error("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

  if (start_result != ESP_OK)
  {
    Logger.Error("Could not start mqtt client; error code:" + start_result);
    return 1;
  }
  else
  {
    Logger.Info("MQTT client started");
    return 0;
  }
}

/*
 * @brief           Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */

static void initializeCanBus()
{
  // Initialize MCP2515 running at 8MHz with a baudrate of 500kb/s
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
  {
    Serial.println("MCP2515 Initialized Successfully!");
  }
  else
  {
    Serial.println("Error Initializing MCP2515... Check your connections and try again.");
    while (1)
      ;
  }

  // Set normal operation mode
  CAN0.setMode(MCP_NORMAL);

  // Configure CAN interrupt pin
  pinMode(CAN_INT_PIN, INPUT);
  Serial.println("CAN OBD-II Data Reader");
}

static uint32_t getEpochTimeInSecs() { return (uint32_t)time(NULL); }

static void establishConnection()
{
  connectToWiFi();
  initializeTime();
  initializeIoTHubClient();
  (void)initializeMqttClient();
}

void sendMessage(byte *pidMsg)
{

  if (CAN0.sendMsgBuf(0x7DF, 0, 8, pidMsg) == CAN_OK)
  {
    Serial.println("PID request sent successfully!");
  }
  else
  {
    Serial.println("Error sending PID request");
  }
  // wait for a bit to receive the message

  // Read data, if available
  if (!digitalRead(CAN_INT_PIN))
  {
    unsigned long replyId;
    unsigned char dlc, data[8];
    if (CAN0.readMsgBuf(&replyId, &dlc, data) == CAN_OK)
    {
      Serial.print("Message received with ID: 0x");
      Serial.println(replyId, HEX);
      Serial.print("Data: ");
      for (int i = 0; i < dlc; i++)
      {
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Filter out unexpected messages
      if (replyId != 0x7E8 || dlc < 3)
      {
        Serial.println("Unexpected reply ID or message length");
        return;
      }

      // Verify response is for RPM PID
      if (data[1] == 0x41 && data[2] == 0x0D)
      {

        int A = data[3];
        int B = data[4];
        vehicleSpeed = A;

        Serial.print("Vehicle speed = ");
        Serial.println(vehicleSpeed);
      }
      else if (data[1] == 0x41 && data[2] == 0x0C)
      {

        int A = data[3];
        int B = data[4];
        engineRPM = ((A * 256) + B) / 4.0;
        if (engineRPM > 0)
          engineStart = true;
        Serial.print("Engine RPM = ");
        Serial.println(engineRPM);
        Serial.println();
      }

      else if (data[1] == 0x41 && data[2] == 0x11)
      {

        int A = data[3];
        int B = data[4];
        throttlePosition = (A * 100) / 255.0;

        Serial.print("Throttle position = ");
        Serial.println(throttlePosition);
        Serial.println();
      }

      else if (data[1] == 0x41 && data[2] == 0x05)
      {

        int A = data[3];
        int B = data[4];
        coolantTemp = A - 40;

        Serial.print("Coolant temperature = ");
        Serial.println(coolantTemp);
        Serial.println();
      }
      else if (data[1] == 0x41 && data[2] == 0x5C)
      {
        int A = data[3];
        int B = data[4];
        float temp = A - 40;

        Serial.print("Oil temperature = ");
        Serial.println(temp);
        Serial.println();
      }
      else
      {
        Serial.println("Error reading message");
      }
    }
    else
    {
      Serial.println("No message received");
    }
  }
  delay(100);
}

void generateTelemetryPayload()
{
  const int capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 200;
  StaticJsonDocument<capacity> doc;

  String deviceName = "ESP32_CANBUS";
  unsigned long currentTimestamp = getEpochTimeInSecs();
  String carName = "BMW E92";
  String formattedTime = getISO8601Time(currentTimestamp);

  doc["device"] = deviceName;
  doc["timestamp"] = formattedTime;
  doc["carName"] = carName;
  doc["data"]["vehicleSpeed"] = vehicleSpeed;
  doc["data"]["engineRPM"] = engineRPM;
  doc["data"]["coolantTemp"] = coolantTemp;
  doc["data"]["throttlePosition"] = throttlePosition;

  // Serialize JSON document to String
  serializeJson(doc, telemetry_payload);

  Logger.Info("Message json:");
  Logger.Info(telemetry_payload);
}

String getISO8601Time(unsigned long epochTime)
{
  // Convert the epoch time to a tm struct
  struct tm *ptm;
  ptm = gmtime((time_t *)&epochTime);

  // Format the date and time in the ISO 8601 format with "T" in the middle and "Z" at the end
  char dateTimeString[21];
  strftime(dateTimeString, 21, "%Y-%m-%dT%H:%M:%SZ", ptm);

  return String(dateTimeString);
}

static void sendTelemetry()
{
  Logger.Info("Sending telemetry ...");

  // The topic could be obtained just once during setup,
  // however if properties are used the topic need to be generated again to reflect the
  // current values of the properties.
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
  {
    Logger.Error("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  generateTelemetryPayload();

  if (esp_mqtt_client_publish(
          mqtt_client,
          telemetry_topic,
          (const char *)telemetry_payload.c_str(),
          telemetry_payload.length(),
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG) == 0)
  {
    Logger.Error("Failed publishing");
  }
  else
  {
    Logger.Info("Message published successfully");
  }
  telemetry_payload = "";
}

void setup()
{

  // Create the FreeRTOS tasks
  xTaskCreatePinnedToCore(
      canBusReaderTask, // Task function
      "CANBusReader",   // Name of the task (for debugging)
      10000,            // Stack size (bytes)
      NULL,             // Parameter to pass to the task
      1,                // Task priority
      NULL,             // Task handle
      0                 // Core where the task should run
  );

  xTaskCreatePinnedToCore(
      wifiMqttTask,
      "WiFiMQTT",
      10000,
      NULL,
      1,
      NULL,
      1 // Running on a different core
  );
}

void loop()
{
}

void canBusReaderTask(void *pvParameters)
{
  // Initialize CAN bus
  initializeCanBus();

  while (1)
  {
    // Read from CAN bus and update global variables
    sendMessage(speedRequest);
    sendMessage(coolantTempRequest);
    sendMessage(rpmRequest);
    sendMessage(oilTempRequest);
    sendMessage(throttlePositionRequest);

    // Delay for a while to control the frequency of CAN bus reading
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void wifiMqttTask(void *pvParameters)
{
  // Connect to WiFi and initialize MQTT
  establishConnection();

  while (1)
  {
    // Check WiFi and MQTT client status, reconnect if necessary
    manageWiFiAndMQTTConnection();

    // Send telemetry data if the engine is started
    if (engineStart)
    {
      sendTelemetry();
    }

    // Delay before next telemetry send
    vTaskDelay(pdMS_TO_TICKS(TELEMETRY_FREQUENCY_MILLISECS));
  }
}

void manageWiFiAndMQTTConnection()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectToWiFi();
  }
#ifndef IOT_CONFIG_USE_X509_CERT
  else if (sasToken.IsExpired())
  {
    Logger.Info("SAS token expired; reconnecting with a new one.");
    (void)esp_mqtt_client_destroy(mqtt_client);
    initializeMqttClient();
  }
#endif
}