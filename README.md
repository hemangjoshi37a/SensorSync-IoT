Please adda a ⭐

# SensorSync-IoT
Advanced 0-5V Analog Data Logger &amp; Real-Time Monitor | Wi-Fi, MQTT, SD Card | ESP32 Sunton HMI

![SensorSync-IoT](screenshots/9.png)

[BUY NOW](https://hjlabs.in/product/sensorsync-iot-advanced-0-5v-analog-data-logger-real-time-monitor-wi-fi-mqtt-sd-card/)

### Table of Contents

[Introduction](#introduction) [Setup](#setup) [Usage](#usage) [Features](#features) [Potential Use Cases](#potential-use-cases) [Troubleshooting](#troubleshooting) [Web Interface](#web-interface) [Wi-Fi Setup](#wifi-setup) [Calibration](#calibration) [API Endpoints](#api-endpoints)

Introduction
------------

The SensorSync IoT is a versatile analogue IoT data-logger and indicator designed to provide seamless integration with various sensors that output analog signals ranging from 0-5V. It enables real-time monitoring and analysis of sensor data, making it an essential tool for various industrial and research applications.

![Introduction](introduction.jpg)

Setup
-----

Follow these simple steps to set up your SensorSync IoT:

1.  Unbox the SensorSync IoT and ensure all components are present.
2.  Connect the power adapter to the device and plug it into an electrical outlet.
3.  Mount the device near your sensor and connect the analog sensors appropriately.
4.  Turn on the device and wait for it to boot up.
5.  Connect to the device's Wi-Fi hotspot to access the configuration portal or use the provided app for seamless setup. Configure your device by following the on-screen instructions to connect it to your local Wi-Fi network.

![Setup Instructions](setup.jpg)

Usage
-----

After setting up your SensorSync IoT, you can start monitoring and logging sensor data. Here's how to use it:

*   **Monitor Data:** View real-time sensor data on the device's LCD screen and through the web interface or app.
*   **Log Data:** Log data to the SD card or transmit it to a cloud server via MQTT for further analysis.
*   **Access Data:** Download logged data from the SD card or view it remotely on your smartphone or computer.

![Usage Example](usage.jpg)

![SensorSync-IoT](screenshots/screenshot.png)

Features
--------

##### Real-time Monitoring

The SensorSync IoT offers real-time monitoring of analog input values, which can be visualized on the LCD screen or recorded for further analysis.

![Real-time Monitoring](real-time.jpg)

##### MQTT Integration

Seamlessly connect the SensorSync IoT to your MQTT broker for real-time data transmission and remote monitoring.

![MQTT Integration](mqtt.jpg)

##### SD Card Logging

Log data locally on an SD card with customizable intervals for offline analysis.

![SD Card Logging](sd-card.jpg)

##### Programmable Scale and Measurement Units

Configure the input scale, title, and measurement units to match your specific application needs.

![Programmable Scale](scaling.jpg)

##### Advanced Calibration

Perform precise calibration of analog inputs to ensure accurate and reliable data readings.

![Advanced Calibration](calibration.jpg)

Potential Use Cases
-------------------

*   **Industrial Monitoring:** Monitor industrial processes and equipment health.
*   **Environmental Monitoring:** Track environmental parameters such as temperature, humidity, and air quality.
*   **Research and Development:** Use data logging and real-time monitoring for experiments and industrial research projects.

![SensorSync-IoT](screenshots/lcd_screenshot.png)

Troubleshooting
---------------

If you encounter any issues, consult the following troubleshooting steps:

*   **SD Card Initialization Failed:** Ensure the SD card is properly inserted and formatted.
*   **Wi-Fi Connection Issues:** Verify Wi-Fi credentials and ensure the device is within range of the network.
*   **MQTT Connection Failed:** Check the MQTT broker address and port configuration.

![Troubleshooting](troubleshooting.jpg)

Web Interface
-------------

The SensorSync IoT offers an intuitive web interface for configuration and monitoring. Here's how to access and use it:

1.  Open a web browser and enter the device's IP address.
2.  Log in with your credentials.
3.  Navigate through the dashboard to monitor real-time data, configure settings, and more.

![Web Interface](web-interface.jpg)

### Dashboard

View real-time sensor data on the main dashboard.

![Dashboard](dashboard.jpg)

### Settings

Configure device settings, including calibration, MQTT broker settings, and SD card logging options.

![Settings](settings.jpg)

Wi-Fi Setup
-----------

To set up Wi-Fi on your SensorSync IoT device, follow these steps:

1.  Turn on the device and wait for it to boot up.
2.  Connect to the device-generated Wi-Fi hotspot named "AutoConnectAP".
3.  Open a web browser and go to `192.168.4.1` to access the Wi-Fi manager.
4.  Select your Wi-Fi network and enter the password, then click "Save".
5.  The device will reboot and connect to the configured Wi-Fi network.

![Wi-Fi Setup](wifi-setup.jpg)

Calibration
-----------

Calibrate your SensorSync IoT using the raw analog values from the info panel:

1.  Measure the maximum and minimum raw analog values from the sensor.
2.  Access the web interface to enter these values under the "Calibration" section.
3.  Enter the desired calibrated min-max values.
4.  Click "Save" to apply the calibration settings.
5.  Check the info panel to verify the calibrated values.

![Calibration Procedure](calibration-procedure.jpg)

API Endpoints
-------------

Interact with the SensorSync IoT programmatically using the following API endpoints:

### Retrieve Live Data

Get live data from the device using the following endpoint:

    GET /liveData

Example: curl -X GET http://DEVICE\_IP/liveData

### Save Calibration Data

Save calibration data using the following endpoint:

    POST /calibrate

Example: curl -X POST -d "raw\_min=0&raw\_max=4095&cal\_min=0&cal\_max=100&interval=6&mqtt\_broker=broker.hivemq.com&mqtt\_port=1883" http://DEVICE\_IP/calibrate

[Made by hjLabs.in](https://hjlabs.in) | 2024




## 📫 How to reach me
[<img height="36" src="https://cdn.simpleicons.org/similarweb"/>](https://hjlabs.in/) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/WhatsApp"/>](https://wa.me/917016525813) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/telegram"/>](https://t.me/hjlabs) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/Gmail"/>](mailto:hemangjoshi37a@gmail.com) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/LinkedIn"/>](https://www.linkedin.com/in/hemang-joshi-046746aa) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/facebook"/>](https://www.facebook.com/hemangjoshi37) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/Twitter"/>](https://twitter.com/HemangJ81509525) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/tumblr"/>](https://www.tumblr.com/blog/hemangjoshi37a-blog) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/StackOverflow"/>](https://stackoverflow.com/users/8090050/hemang-joshi) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/Instagram"/>](https://www.instagram.com/hemangjoshi37) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/Pinterest"/>](https://in.pinterest.com/hemangjoshi37a) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/Blogger"/>](http://hemangjoshi.blogspot.com) &nbsp;
[<img height="36" src="https://cdn.simpleicons.org/gitlab"/>](https://gitlab.com/hemangjoshi37a) &nbsp;

## Checkout Cool GitHub Other Repositories:
- [pyPortMan](https://github.com/hemangjoshi37a/pyPortMan)
- [transformers_stock_prediction](https://github.com/hemangjoshi37a/transformers_stock_prediction)
- [TrendMaster](https://github.com/hemangjoshi37a/TrendMaster)
- [hjAlgos_notebooks](https://github.com/hemangjoshi37a/hjAlgos_notebooks)
- [AutoCut](https://github.com/hemangjoshi37a/AutoCut)
- [My_Projects](https://github.com/hemangjoshi37a/My_Projects)
- [Cool Arduino and ESP8266 or NodeMCU Projects](https://github.com/hemangjoshi37a/my_Arduino)
- [Telegram Trade Msg Backtest ML](https://github.com/hemangjoshi37a/TelegramTradeMsgBacktestML)

## Checkout Our Other Products:
- [WiFi IoT LED Matrix Display](https://hjlabs.in/product/wifi-iot-led-display)
- [SWiBoard WiFi Switch Board IoT Device](https://hjlabs.in/product/swiboard-wifi-switch-board-iot-device)
- [Electric Bicycle](https://hjlabs.in/product/electric-bicycle)
- [Product 3D Design Service with Solidworks](https://hjlabs.in/product/product-3d-design-with-solidworks/)
- [AutoCut : Automatic Wire Cutter Machine](https://hjlabs.in/product/automatic-wire-cutter-machine/)
- [Custom AlgoTrading Software Coding Services](https://hjlabs.in/product/custom-algotrading-software-for-zerodha-and-angel-w-source-code//)
- [SWiBoard :Tasmota MQTT Control](https://play.google.com/store/apps/details?id=in.hjlabs.swiboard)
- [Custom Token Classification or Named Entity Recognition (NER) model as in Natural Language Processing (NLP) Machine Learning](https://hjlabs.in/product/custom-token-classification-or-named-entity-recognition-ner-model-as-in-natural-language-processing-nlp-machine-learning/)

## Some Cool Arduino and ESP8266 (or NodeMCU) IoT projects:
- [IoT_LED_over_ESP8266_NodeMCU : Turn LED on and off using web server hosted on a nodemcu or esp8266](https://github.com/hemangjoshi37a/my_Arduino/tree/master/IoT_LED_over_ESP8266_NodeMCU)
- [ESP8266_NodeMCU_BasicOTA : Simple OTA (Over The Air) upload code from Arduino IDE using WiFi to NodeMCU or ESP8266](https://github.com/hemangjoshi37a/my_Arduino/tree/master/ESP8266_NodeMCU_BasicOTA)  
- [IoT_CSV_SD : Read analog value of Voltage and Current and write it to SD Card in CSV format for Arduino, ESP8266, NodeMCU etc](https://github.com/hemangjoshi37a/my_Arduino/tree/master/IoT_CSV_SD)  
- [Honeywell_I2C_Datalogger : Log data in A SD Card from a Honeywell I2C HIH8000 or HIH6000 series sensor having external I2C RTC clock](https://github.com/hemangjoshi37a/my_Arduino/tree/master/Honeywell_I2C_Datalogger)
- [IoT_Load_Cell_using_ESP8266_NodeMC : Read ADC value from High Precision 12bit ADS1015 ADC Sensor and Display on SSD1306 SPI Display as progress bar for Arduino or ESP8266 or NodeMCU](https://github.com/hemangjoshi37a/my_Arduino/tree/master/IoT_Load_Cell_using_ESP8266_NodeMC)
- [IoT_SSD1306_ESP8266_NodeMCU : Read from High Precision 12bit ADC seonsor ADS1015 and display to SSD1306 SPI as progress bar in ESP8266 or NodeMCU or Arduino](https://github.com/hemangjoshi37a/my_Arduino/tree/master/IoT_SSD1306_ESP8266_NodeMCU)  

## Our HuggingFace Models :
- [hemangjoshi37a/autotrain-ratnakar_1000_sample_curated-1474454086 : Stock tip message NER(Named Entity Recognition or Token Classification) using HUggingFace-AutoTrain and LabelStudio and Ratnakar Securities Pvt. Ltd.](https://huggingface.co/hemangjoshi37a/autotrain-ratnakar_1000_sample_curated-1474454086)

## Our HuggingFace Datasets :
- [hemangjoshi37a/autotrain-data-ratnakar_1000_sample_curated : Stock tip message NER(Named Entity Recognition or Token Classification) using HUggingFace-AutoTrain and LabelStudio and Ratnakar Securities Pvt. Ltd.](https://huggingface.co/datasets/hemangjoshi37a/autotrain-data-ratnakar_1000_sample_curated)

## Awesome Youtube Videos :
- [❤️ હદય અને હદયના ધબકારા 💙 दिल और दिल की धड़कन 💖 Heart and beating of heart by Priyanka madam. 💕](https://www.youtube.com/watch?v=9v3MK6oTOeA)
- [🩸 રુધિર વહીનીઓ અને એના કર્યો. 🩸 Blood Vessels And Working of Blood Vessels 🩸 By Priyankama'am](https://www.youtube.com/watch?v=T7mMcEYNKyQ)
- [🩸 મનુષ્યમાં પરિવહન તંત્ર 🩸 परिसंचरण तंत्र 🩸 Blood Circulation System in Humans🩸 By Priyanka madam](https://www.youtube.com/watch?v=vxa6o_wrWnY)
- [AutoCut V2 - The World's Most Powerful Arduino Automatic Wire Cutting Machine](https://www.youtube.com/watch?v=oGr0mWmNhKY)
- [SWiBoard - A Killer Gadget to Boost Your Boring Switchboard](https://www.youtube.com/watch?v=ftza6WM4LiE)
- [🧪 મનુષ્યમાં ઉત્સર્જન-તંત્ર 🦠 मानव उत्सर्जन तंत्र ⚗️ excretory system 🩺](https://www.youtube.com/watch?v=UUGI-CFKsWI)
- [🌳વનસ્પતિમાં પાણી અને ખનીજ તત્વોનું વહન 🌲](https://youtu.be/1da9p6iYlr4)
- [🌲 વનસ્પતિમાં બાષ્પોત્સર્જન 🌳 पेड़ में वाष्पोत्सर्जन 🎄Transpiration in Trees](https://youtu.be/I9Sirc42Ktg)
- [🫁 સજીવોમાં શ્વસન 🧬 जीवों में श्वास 🫀 Breathing in organisms 👩🏻‍🔬](https://youtu.be/sIMl4t2OFmY)
- [🫁 શ્વસનની પ્રક્રિયા 🫀Respiratory System 🦠](https://youtu.be/hua8ZD5Ge1w)
- [🫁 મનુષ્યમાં શ્વાસ અને ઉચ્છશ્વાસ ⚛️ ](https://youtu.be/BI-CYgnkGCw)

## My Quirky Blog :
- [Hemang Joshi](http://hemangjoshi.blogspot.com/)

## Awesome Android Apps :
- [SWiBoard :Tasmota MQTT Control](https://play.google.com/store/apps/details?id=in.hjlabs.swiboard)
 
## Checkout Cool GitLab Other Repositories:
- [pyPortMan](https://gitlab.com/hemangjoshi37a/pyPortMan)
- [transformers_stock_prediction](https://gitlab.com/hemangjoshi37a/transformers_stock_prediction)
- [TrendMaster](https://gitlab.com/hemangjoshi37a/TrendMaster)
- [hjAlgos_notebooks](https://gitlab.com/hemangjoshi37a/hjAlgos_notebooks)
- [AutoCut](https://gitlab.com/hemangjoshi37a/AutoCut)
- [My_Projects](https://gitlab.com/hemangjoshi37a/My_Projects)
- [Cool Arduino and ESP8266 or NodeMCU Projects](https://gitlab.com/hemangjoshi37a/my_Arduino)
- [Telegram Trade Msg Backtest ML](https://gitlab.com/hemangjoshi37a/TelegramTradeMsgBacktestML)
