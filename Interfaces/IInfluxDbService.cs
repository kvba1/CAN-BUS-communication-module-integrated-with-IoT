using esp32CANBUS.Models;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace esp32CANBUS.Interfaces
{
    public interface IInfluxDbService
    {
        public Task SendDataAsync(CarDataModel model);
    }
}
