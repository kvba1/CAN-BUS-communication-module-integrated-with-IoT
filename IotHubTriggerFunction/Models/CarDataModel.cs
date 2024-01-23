using Newtonsoft.Json;
using System;

namespace esp32CANBUS.Models
{
    public class CarDataModel
    {
        [JsonProperty("device")]
        public string Device { get; set; }

        [JsonProperty("timestamp")]
        public DateTime Timestamp { get; set; }

        [JsonProperty("carName")]
        public string Car { get; set; }

        [JsonProperty("data")]
        public DataModel Data { get; set; }
    }

    public class DataModel
    {
        [JsonProperty("vehicleSpeed")]
        public float VehicleSpeed { get; set; }

        [JsonProperty("engineRPM")]
        public float EngineRPM { get; set; }

        [JsonProperty("coolantTemp")]
        public float EngineTemperature { get; set; }

        [JsonProperty("runtime")]
        public float CarRuntime { get; set; }

        [JsonProperty("engineLoad")]
        public float EngineLoad { get; set; }

        [JsonProperty("intakePressure")]
        public float IntakePressure { get; set; }

    }

}
