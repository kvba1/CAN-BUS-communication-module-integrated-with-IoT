using esp32CANBUS.Interfaces;
using Microsoft.Azure.Functions.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection;

[assembly: FunctionsStartup(typeof(esp32CANBUS.Startup))]

namespace esp32CANBUS
{
    public class Startup : FunctionsStartup
    {
        public override void Configure(IFunctionsHostBuilder builder)
        {
            builder.Services.AddSingleton<IInfluxDbService, InfluxDbService>();
        }
    }
}

