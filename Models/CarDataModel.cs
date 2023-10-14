using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace esp32CANBUS.Models
{
    public class CarDataModel
    {
        [JsonProperty("device")]
        public string Device { get; set; }

        [JsonProperty("timestamp")]
        public DateTime Timestamp { get; set; }

        [JsonProperty("car_name")]
        public string Car { get; set; }

        [JsonProperty("data")]
        public DataModel Data { get; set; }
    }

    public class DataModel
    {
        [JsonProperty("speed")]
        public int Speed { get; set; }

        [JsonProperty("rpm")]
        public int Rpm { get; set; }

        [JsonProperty("engine_temperature")]
        public int EngineTemperature { get; set; }
    }

}
