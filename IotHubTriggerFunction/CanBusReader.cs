using IoTHubTrigger = Microsoft.Azure.WebJobs.EventHubTriggerAttribute;

using Microsoft.Azure.WebJobs;
using Microsoft.Azure.EventHubs;
using System.Text;
using System.Net.Http;
using Microsoft.Extensions.Logging;
using System.Text.Json;
using esp32CANBUS.Models;
using Newtonsoft.Json;
using System.Threading.Tasks;
using System;

namespace esp32CANBUS
{
    public class CanBusReader
    {
        private static HttpClient client = new HttpClient();
        private readonly InfluxDbService _influxClient = new InfluxDbService();

        [FunctionName("CanBusReader")]
        public async Task Run([IoTHubTrigger("messages/events", Connection = "connectionString")]EventData message, ILogger log)
        {
            try
            {
                string incomingMessage = Encoding.UTF8.GetString(message.Body.Array);

                JsonDocument parsedMessage = JsonDocument.Parse(incomingMessage);

                if (parsedMessage.Deserialize<CarDataModel>() != null)
                {
                    var carData = JsonConvert.DeserializeObject<CarDataModel>(incomingMessage);
                    await _influxClient.SendDataAsync(carData);
                }

                log.LogInformation($"C# IoT Hub trigger function processed a message: {incomingMessage}");
            }
            catch (Exception ex)
            {
                log.LogInformation($"{ex.Message}");
                throw;
            }
        }
    }
}
