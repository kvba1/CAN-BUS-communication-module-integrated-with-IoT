using esp32CANBUS.Interfaces;
using esp32CANBUS.Models;
using InfluxDB3.Client;
using InfluxDB3.Client.Write;
using System;
using System.Threading.Tasks;

namespace esp32CANBUS
{
    internal class InfluxDbService : IInfluxDbService
    {
        private readonly InfluxDBClient _client;

        private readonly string bucket = Environment.GetEnvironmentVariable("bucket");
        private readonly string org = Environment.GetEnvironmentVariable("org");
        private readonly string url = Environment.GetEnvironmentVariable("url");
        private readonly string token = Environment.GetEnvironmentVariable("token");

        public InfluxDbService()
        {
            _client = new InfluxDBClient(url,token,org,bucket);
        }

        public async Task SendDataAsync(CarDataModel data)
        {
            var point = PointData.Measurement("CarData")
            .SetTag("Device", data.Device)
            .SetTag("Car", data.Car)
            .SetField("Speed", data.Data.VehicleSpeed)
            .SetField("RPM", data.Data.EngineRPM)
            .SetField("EngineTemperature", data.Data.EngineTemperature)
            .SetField("ThrottlePosition", data.Data.ThrottlePosition)
            .SetTimestamp(data.Timestamp);

           await _client.WritePointAsync(point);
        }
    }

}
