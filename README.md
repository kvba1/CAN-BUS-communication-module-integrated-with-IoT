![DALL·E 2023-10-14 18 44 32 - Illustration of a car emitting radiant waves of data  The waves move towards a detailed microcontroller board  From the microcontroller, a series of b](https://github.com/kvba1/CAN-BUS-communication-module-integrated-with-IoT/assets/128424095/1ed26d3f-1c71-43dc-8e86-7c299052062a)

# My project harnesses the capabilities of the ESP32 microcontroller to interface and extract real-time data from cars. Once the data is captured, it is securely transmitted to the Azure IoT Hub. Leveraging the power of Azure, a C# IoT trigger function processes the incoming data stream. Post-processing, this data is then efficiently directed into an InfluxDB database. This system provides a seamless pipeline for automotive data, ensuring timely and reliable storage for further analytics.

## To launch frontend Angular app in the frontend folder run:
 ```bash
docker build -t dashboard-app .
docker run -d -p 8080:8080 dashboard-app
```bash

Data flow diagram:

![dataflowdiag](https://github.com/kvba1/CAN-BUS-communication-module-integrated-with-IoT/assets/128424095/6896a8d6-4ceb-4bf4-a869-0e3c6638083f)

Electric circuit diagram:

![obraz](https://github.com/kvba1/CAN-BUS-communication-module-integrated-with-IoT/assets/128424095/0dcd50f1-b7e7-4bfa-a1b7-aaa975542e73)

Board:

![obraz](https://github.com/kvba1/CAN-BUS-communication-module-integrated-with-IoT/assets/128424095/b6ddb217-3d31-459f-b7bf-4f87a610fb4b)

Web application to visualize car's data, implemented in Angular with Chart.js:

![obraz](https://github.com/kvba1/CAN-BUS-communication-module-integrated-with-IoT/assets/128424095/48995e7c-ec93-461e-a724-2b5abcc3b94f)

