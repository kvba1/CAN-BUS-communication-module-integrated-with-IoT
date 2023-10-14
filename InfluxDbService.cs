using esp32CANBUS.Interfaces;
using esp32CANBUS.Models;
using InfluxDB3.Client;
using InfluxDB3.Client.Write;
using System.Threading.Tasks;

namespace esp32CANBUS
{
    internal class InfluxDbService : IInfluxDbService
    {
        private readonly InfluxDBClient _client;
        private const string _bucket = "CarData";
        private const string _org = "dev";

        public InfluxDbService(string influxDbUrl, string token)
        {
            _client = new InfluxDBClient(influxDbUrl, token);
        }

        public async Task SendDataAsync(CarDataModel data)
        {
            var point = PointData.Measurement("CarData")
            .SetTag("Device", data.Device)
            .SetTag("Car", data.Car)
            .SetField("Speed", data.Data.Speed)
            .SetField("RPM", data.Data.Rpm)
            .SetField("EngineTemperature", data.Data.EngineTemperature)
            .SetTimestamp(data.Timestamp);

            await _client.WritePointAsync(point, _bucket);
        }
    }


}
