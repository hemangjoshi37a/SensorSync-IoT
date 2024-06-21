#include <Update.h>
#include <SPI.h>
#include <SD.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include <bearssl_hash.h>
// #include <WiFi.h>
#include <Hash.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TOTP.h>

#define EEPROM_HASH_ADDRESS 200  // EEPROM address to store hash

#define RESTART_INTERVAL 600000     // 10 minutes in milliseconds
unsigned long lastRestartTime = 0;  // Track the last restart time
#define MAX_SD_USAGE_PERCENT 95
const char *totpSecret = "YOURTOTPSECRET123";  // TOTP secret key
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);  // 19800 seconds offset for IST (+5 hours 30 minutes)

TOTP totp((uint8_t *)totpSecret, strlen(totpSecret));

lv_obj_t *totp_input_area;
lv_obj_t *totp_submit_button;
lv_obj_t *analog_label;
// Declare the keyboard globally
lv_obj_t *keyboard;
char gaugeTitle[40] = "LNG Fuel";  // Default title for the gauge
char measurementUnit[40] = "kg";
char serialNumber[20] = "SN-000001";  // Default serial number
lv_obj_t *analog_label_info;
lv_obj_t *voltage_label_info;
lv_obj_t *voltage_label_info2;

AsyncWebServer server(80);
int raw_values[2] = { 0, 4095 };  // Raw analog values for calibration
int cal_values[2] = { 0, 100 };   // Desired values for calibration

#define TFT_BL 2
#define ANALOG_PIN 17     // Define the analog input pin
#define GFX_BL DF_GFX_BL  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
lv_obj_t *digital_label;

lv_chart_series_t *ser1;  // Declare globally
lv_obj_t *chart;          // Declare globally
lv_obj_t *table;
lv_meter_scale_t *scale;

lv_color_t bg_color;
lv_color_t fg_color;

const char *password = "mypassword";  // Hardcoded password
const int chipSelect = 10;            // SD card CS pin
WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long dataLoggingInterval = 6000;   // Default interval in milliseconds
unsigned long lastDataLoggingTime = 0;      // To keep track of the last data logging time
char deviceID[9];                           // Random device ID
char mqttBroker[40] = "broker.hivemq.com";  // Default MQTT broker
int mqttPort = 1883;                        // Default MQTT port

// Include button in UI to toggle between graph and table
lv_obj_t *toggle_button;
bool is_table_visible = false;
bool sdLoggingEnabled = true;

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */

#if defined(DISPLAY_DEV_KIT)

Arduino_GFX *gfx = create_default_Arduino_GFX();

#else /* !defined(DISPLAY_DEV_KIT) */

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
  40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
  45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
  5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
  8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
);

// option 1:
// ILI6485 LCD 480x272

Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
  480 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
  272 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 9000000 /* prefer_speed */, true /* auto_flush */);

#endif /* !defined(DISPLAY_DEV_KIT) */
#include "touch.h"

/* Change to your screen resolution */

static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
lv_obj_t *mqtt_status_label;

lv_obj_t *dial;               // Declare the dial object
lv_meter_indicator_t *indic;  // Declare the needle indicator

lv_obj_t *calibration_ui;  // Calibration UI object

// Forward declarations
void create_main_ui();
void update_dial_value();
int map_analog_value(int raw_value);
void toggle_view_event_cb(lv_event_t *e);
void logDataToSD(bool append = true);
void reconnectMqtt();
void update_wifi_status();
void create_totp_ui();
void totp_submit_event_cb(lv_event_t *e);
String listFiles(fs::FS &fs, const char *dirname, uint8_t levels);
void printHeapStatsTimer(void *pvParameters);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PR;
      /*Set the coordinates*/
      data->point.x = touch_last_x;
      data->point.y = touch_last_y;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_REL;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

lv_obj_t *wifi_label;  // Label to show Wi-Fi status

void setup() {
  Serial.begin(115200);
  Serial.println("LVGL Widgets Demo");

  // Init touch device
  touch_init();

  // Init Display
  gfx->begin();

  xTaskCreate(
    printHeapStatsTimer,
    "HeapStats",
    2048,
    NULL,
    1,
    NULL);

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  lv_init();


  // Initialize Wi-Fi in a separate task
  xTaskCreatePinnedToCore(
    wifiTask,    // Function to be called
    "wifiTask",  // Name of the task
    8192,        // Stack size in words (Increase from 4096 to 8192)
    NULL,        // Task input parameter
    1,           // Priority of the task
    NULL,        // Task handle
    0            // Core where the task should run
  );

  delay(10);

  EEPROM.begin(512);                         // Initialize EEPROM
  dataLoggingInterval = EEPROM.readInt(16);  // Load data logging interval
  EEPROM.get(20, mqttBroker);                // Load MQTT broker
  mqttPort = EEPROM.readInt(60);             // Load MQTT port
  sdLoggingEnabled = EEPROM.readBool(250);
  EEPROM.get(130, measurementUnit);  // Load saved measurement unit
  EEPROM.get(70, gaugeTitle);        // Load saved title
  EEPROM.get(170, serialNumber);     // Load serial number

  // Enhanced SD card initialization logging
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
  } else {
    Serial.println("Card initialized.");
  }


  // // Load theme preference from EEPROM
  // int theme = EEPROM.read(0);

  // if (theme == 1) {
  //   bg_color = lv_palette_main(LV_PALETTE_BLUE);
  //   fg_color = lv_palette_main(LV_PALETTE_RED);
  // } else {
  //   bg_color = lv_palette_main(LV_PALETTE_GREY);
  //   fg_color = lv_palette_main(LV_PALETTE_GREEN);
  // }

  // lv_obj_set_style_bg_color(lv_scr_act(), bg_color, 0);
  // lv_obj_set_style_text_color(lv_scr_act(), fg_color, 0);

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * screenWidth * screenHeight / 4);
#endif

  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 4);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the (dummy) input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // if (sdLoggingEnabled) {
    //   // Create a table to display the logged data and place it below the dial
    //   table = lv_table_create(lv_scr_act());
    //   lv_obj_set_size(table, 200, 150);
    //   lv_obj_align(table, LV_ALIGN_LEFT_MID, 10, 0);  // Align the table similarly to the chart
    //   lv_table_set_col_cnt(table, 2);
    //   lv_table_set_col_width(table, 0, 100);
    //   lv_table_set_col_width(table, 1, 100);
    //   lv_table_set_cell_value(table, 0, 0, "Time");
    //   lv_table_set_cell_value(table, 0, 1, "Value");
    //   lv_obj_add_flag(table, LV_OBJ_FLAG_HIDDEN);
    // }

    // Generate device ID using MAC address
    uint64_t chipid = ESP.getEfuseMac();
    snprintf(deviceID, sizeof(deviceID), "%02X%02X%02X%02X%02X%02X",
             (uint8_t)(chipid >> 40) & 0xFF, (uint8_t)(chipid >> 32) & 0xFF,
             (uint8_t)(chipid >> 24) & 0xFF, (uint8_t)(chipid >> 16) & 0xFF,
             (uint8_t)(chipid >> 8) & 0xFF, (uint8_t)chipid & 0xFF);
    Serial.printf("Device ID: %s\n", deviceID);


    // Create the main UI
    create_main_ui();
  }

  Serial.println("Setup done");

  //////// TOTP AUTH ////////////////
  // Initialize NTP client
  timeClient.begin();
  timeClient.setTimeOffset(19800);  // Set time offset to +5 hours 30 minutes for IST

  // Calculate the hash of the MAC address
  uint64_t chipid = ESP.getEfuseMac();
  String macStr = String((uint16_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
  String macHash = sha1(macStr);


  // Load stored hash from EEPROM
  char storedHash[65];
  EEPROM.get(EEPROM_HASH_ADDRESS, storedHash);

  if (storedHash[0] == 0) {
    // Store the MAC hash if EEPROM is empty
    macHash.toCharArray(storedHash, 65);
    EEPROM.put(EEPROM_HASH_ADDRESS, storedHash);
    EEPROM.commit();
  } else if (macHash != String(storedHash)) {
    // Prompt for TOTP if hash does not match
    create_totp_ui();
  }
  //////////////////////
}


static unsigned long lastSyncTime = 0;
void loop() {
  // Check if 10 minutes have passed to restart the ESP
  if (millis() - lastRestartTime >= RESTART_INTERVAL) {
    ESP.restart();
  }

  lv_timer_handler(); /* let the GUI do its work */

  // Add task delay to yield control and avoid blocking
  delay(5);

  // Update the dial value
  update_dial_value();

  // Sync time from the internet every 10 minutes
  if (WiFi.status() == WL_CONNECTED && millis() - lastSyncTime > 600000) {  // 600000 ms = 10 minutes
    timeClient.update();
    lastSyncTime = millis();
  }

  // sd card dataloggin if from webpage turned on
  if (sdLoggingEnabled) {
    if (millis() - lastDataLoggingTime >= dataLoggingInterval) {
      logDataToSD(true);
      lastDataLoggingTime = millis();
    }
  }

  // Yield control to other tasks
  vTaskDelay(5 / portTICK_PERIOD_MS);
}

void create_main_ui() {

  // if (sdLoggingEnabled) {
  //   // Create a chart to display the recorded data from the SD card
  //   chart = lv_chart_create(lv_scr_act());
  //   lv_obj_set_size(chart, lv_obj_get_width(lv_scr_act()) / 2 - 20, lv_obj_get_height(lv_scr_act()) / 1.5 - 20);
  //   lv_obj_align(chart, LV_ALIGN_LEFT_MID, 40, 15);
  //   lv_chart_set_type(chart, LV_CHART_TYPE_LINE);  // Set the chart type to line
  //   // Add X and Y axis labels
  //   lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 10, 1, true, 30);
  //   lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 10, 1, true, 30);
  //   // Set the chart series
  //   ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
  //   // Call the update_chart_from_sd function to plot the recorded data
  //   update_chart_from_sd(false);
  //   // Create a table to display the logged data and place it below the dial
  //   table = lv_table_create(lv_scr_act());
  //   lv_obj_set_size(table, 200, 150);
  //   lv_obj_align(table, LV_ALIGN_LEFT_MID, 10, 0);  // Align the table similarly to the chart
  //   lv_table_set_col_cnt(table, 2);
  //   lv_table_set_col_width(table, 0, 100);
  //   lv_table_set_col_width(table, 1, 100);
  //   lv_table_set_cell_value(table, 0, 0, "Time");
  //   lv_table_set_cell_value(table, 0, 1, "Value");
  //   lv_obj_add_flag(table, LV_OBJ_FLAG_HIDDEN);

  //   // Create a button to toggle between graph and table
  //   toggle_button = lv_btn_create(lv_scr_act());
  //   lv_obj_set_size(toggle_button, 40, 40);
  //   lv_obj_align(toggle_button, LV_ALIGN_TOP_MID, -20, 45);
  //   lv_obj_t *btn_label = lv_label_create(toggle_button);
  //   lv_label_set_text(btn_label, "T");
  //   // Assign event handler for the button
  //   lv_obj_add_event_cb(toggle_button, toggle_view_event_cb, LV_EVENT_CLICKED, table);
  // }

  // Create a dial

  dial = lv_meter_create(lv_scr_act());
  lv_obj_center(dial);
  lv_obj_set_size(dial, 250, 250);

  // Create a title label
  lv_obj_t *title_label = lv_label_create(lv_scr_act());
  lv_label_set_text(title_label, gaugeTitle);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 85);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);

  // /*Remove the circle from the middle*/
  // lv_obj_remove_style(dial, NULL, LV_PART_INDICATOR);

  // Create a scale for the dial
  scale = lv_meter_add_scale(dial);

  // Update the scale settings to show labels and ticks
  // lv_meter_set_scale_ticks(dial, scale, static_cast<int>(cal_values[1] * 0.01), 2, 2, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(dial, scale, static_cast<int>(cal_values[1] * 0.15), 3, 20, lv_palette_main(LV_PALETTE_BROWN), 10);
  lv_meter_set_scale_range(dial, scale, cal_values[0], cal_values[1], 270, 135);  // Set the range according to loaded values

  // Updated scale settings
  lv_meter_set_scale_ticks(dial, scale, cal_values[1] - cal_values[0] + 1, 2, 2, lv_palette_main(LV_PALETTE_GREY));

  // Updated needle styling
  lv_obj_remove_style(dial, NULL, LV_PART_INDICATOR);
  indic = lv_meter_add_arc(dial, scale, 10, lv_palette_main(LV_PALETTE_GREEN), 0);

  // Add unit labels
  lv_obj_t *unit_label = lv_label_create(dial);
  lv_label_set_text(unit_label, measurementUnit);
  lv_obj_align(unit_label, LV_ALIGN_CENTER, 0, 45);  // Position in the center near the dial
  lv_obj_set_style_text_font(unit_label, &lv_font_montserrat_20, 0);

  // Create a label to display the digital number at the center of the dial
  digital_label = lv_label_create(dial);  // Add the label inside the dial
  lv_obj_align(digital_label, LV_ALIGN_CENTER, 0, 5);
  lv_obj_set_style_text_font(digital_label, &lv_font_digital, 0);  // Use available font   lv_font_montserrat_48
  lv_obj_set_style_text_color(digital_label, lv_palette_main(LV_PALETTE_RED), 0);
  lv_label_set_text(digital_label, "0");
  lv_obj_move_foreground(digital_label);  // Ensure it is in the foreground

  // Display E at the beginning of the dial near minimum valaue
  lv_obj_t *e_label = lv_label_create(dial);  // Add the label inside the dial
  // lv_obj_set_style_text_font(e_label, &lv_font_montserrat_48, 0);  // Use available font
  lv_obj_set_style_text_color(e_label, lv_palette_main(LV_PALETTE_RED), 0);
  lv_label_set_text(e_label, "E");
  // lv_obj_move_foreground(e_label);  // Ensure it is in the foreground
  lv_obj_align(e_label, LV_ALIGN_LEFT_MID, 40, 75);

  // Display F at the end of the dial near maximum valaue
  lv_obj_t *f_label = lv_label_create(dial);  // Add the label inside the dial
  // lv_obj_set_style_text_font(f_label, &lv_font_montserrat_48, 0);  // Use available font
  lv_obj_set_style_text_color(f_label, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_label_set_text(f_label, "F");
  // lv_obj_move_foreground(f_label);  // Ensure it is in the foreground
  lv_obj_align(f_label, LV_ALIGN_RIGHT_MID, -40, 75);


  // Move voltage label to info container
  voltage_label_info2 = lv_label_create(dial);
  lv_label_set_text(voltage_label_info2, " 0");  // Initial text for voltage reading
  lv_obj_align(voltage_label_info2, LV_ALIGN_CENTER, 0, 70);

  // // Move the whole dial to the right side of the display
  // if (sdLoggingEnabled) {
  //   lv_obj_align(dial, LV_ALIGN_RIGHT_MID, -10, 20);
  //   // other UI elements for logging if logging is enabled
  // } else {
  //   lv_obj_align(dial, LV_ALIGN_CENTER, 0, 5);
  // }

  // Load calibration data from EEPROM
  load_calibration_data();

  // Create a container for the additional information
  lv_obj_t *info_container = lv_obj_create(lv_scr_act());
  lv_obj_set_size(info_container, lv_obj_get_width(lv_scr_act()) - 20, 190);
  lv_obj_align(info_container, LV_ALIGN_BOTTOM_MID, 0, 255);
  lv_obj_set_style_pad_all(info_container, 10, 0);
  lv_obj_set_style_border_width(info_container, 1, 0);
  lv_obj_set_style_border_color(info_container, lv_palette_main(LV_PALETTE_BLUE), 0);

  // Create a label for the device ID
  lv_obj_t *serial_number_label = lv_label_create(info_container);
  lv_label_set_text_fmt(serial_number_label, "Serial Number: %s", serialNumber);
  lv_obj_align(serial_number_label, LV_ALIGN_TOP_LEFT, 0, 0);

  // Create a label for the device ID
  lv_obj_t *device_id_label = lv_label_create(info_container);
  lv_label_set_text_fmt(device_id_label, "Device ID: %s", deviceID);
  lv_obj_align(device_id_label, LV_ALIGN_TOP_LEFT, 0, 20);

  // Move analog label to info container
  analog_label_info = lv_label_create(info_container);
  lv_label_set_text(analog_label_info, "Analog Reading: 0");  // Initial text for analog reading
  lv_obj_align(analog_label_info, LV_ALIGN_TOP_LEFT, 0, 40);

  // Move voltage label to info container
  voltage_label_info = lv_label_create(info_container);
  lv_label_set_text(voltage_label_info, "Voltage Reading: 0");  // Initial text for voltage reading
  lv_obj_align(voltage_label_info, LV_ALIGN_TOP_LEFT, 0, 60);

  // Create a label for the WiFi status
  wifi_label = lv_label_create(info_container);

  lv_label_set_text_fmt(wifi_label, "WiFi: Not Connected");
  lv_obj_align(wifi_label, LV_ALIGN_TOP_LEFT, 0, 80);

  // Update mqtt_status_label alignment
  mqtt_status_label = lv_label_create(info_container);
  lv_label_set_text(mqtt_status_label, "MQTT Disconnected");
  lv_obj_align(mqtt_status_label, LV_ALIGN_TOP_LEFT, 0, 100);

  // Create labels for MQTT broker configuration and data logging interval
  lv_obj_t *mqtt_broker_label = lv_label_create(info_container);
  lv_label_set_text_fmt(mqtt_broker_label, "Broker: %s", mqttBroker);
  lv_obj_align(mqtt_broker_label, LV_ALIGN_TOP_LEFT, 0, 120);
  lv_obj_t *logging_interval_label = lv_label_create(info_container);
  lv_label_set_text_fmt(logging_interval_label, "SD Data Interval: %lu ms", dataLoggingInterval);
  lv_obj_align(logging_interval_label, LV_ALIGN_TOP_LEFT, 0, 140);
}



void save_calibration_data() {
  EEPROM.writeBool(250, sdLoggingEnabled);
  EEPROM.writeInt(0, raw_values[0]);
  EEPROM.writeInt(4, raw_values[1]);
  EEPROM.writeInt(8, cal_values[0]);
  EEPROM.writeInt(12, cal_values[1]);
  EEPROM.writeInt(16, dataLoggingInterval);  // Save data logging interval
  EEPROM.put(20, mqttBroker);                // Save MQTT broker
  EEPROM.put(130, measurementUnit);          // Save measurement unit to EEPROM
  EEPROM.put(70, gaugeTitle);
  EEPROM.writeInt(60, mqttPort);  // Save MQTT port
  EEPROM.put(170, serialNumber);  // Save serial number
  EEPROM.commit();
}

void update_dial_value() {
  static int analogValues[27] = { 0 };  // Array to store the last 27 analog readings
  static int index = 0;                 // Index for the circular buffer
  static bool bufferFilled = false;     // Flag to check if the buffer is filled

  // Read the current analog value
  int currentAnalogValue = analogRead(ANALOG_PIN);

  // Store the current analog value in the array
  analogValues[index] = currentAnalogValue;
  index = (index + 1) % 27;  // Move to the next index in a circular manner

  // Check if the buffer is filled
  if (index == 0) {
    bufferFilled = true;
  }

  // Calculate the average of the latest 27 readings
  int sum = 0;
  int count = bufferFilled ? 27 : index;
  for (int i = 0; i < count; i++) {
    sum += analogValues[i];
  }

  int averageAnalogValue = sum / count;
  lv_label_set_text_fmt(analog_label_info, "Analog Reading: %d", averageAnalogValue);  // Initial text for analog reading

  float voltage = float(averageAnalogValue) * (5.0 / 4470);
  float floatNum = voltage;
  int intPart = (int)floatNum;
  int decimalPart = (int)((floatNum - intPart) * 100);
  // Serial.print("Original float: ");
  // Serial.println(floatNum, 3);
  String intPartStr = String(intPart);
  String decimalPartStr = String(decimalPart);
  String floatStr = intPartStr + "." + decimalPartStr;
  // Serial.print("Reconstructed float: ");
  // Serial.println(float(floatStr), 3);
  lv_label_set_text_fmt(voltage_label_info, "Voltage Reading: %s V", floatStr);  // Adjusted precision for voltage reading
  lv_label_set_text_fmt(voltage_label_info2, "%s V DC", floatStr);               // Adjusted precision for voltage reading

  // Map the average analog value using calibration
  int calibratedValue = map_analog_value(averageAnalogValue);

  // // Update the dial value
  // lv_meter_set_indicator_value(dial, indic, calibratedValue);
  lv_meter_set_indicator_end_value(dial, indic, calibratedValue);

  // Update the scale range dynamically
  lv_meter_set_scale_ticks(dial, scale, cal_values[1] - cal_values[0] + 1, 2, 2, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_range(dial, scale, cal_values[0], cal_values[1], 270, 135);

  // Update the digital number
  char buf[10];
  snprintf(buf, sizeof(buf), "%d", calibratedValue);
  lv_label_set_text(digital_label, buf);

  // // Update raw analog reading
  // char analog_buf[20];
  // snprintf(analog_buf, sizeof(analog_buf), "%d", currentAnalogValue);
  // lv_label_set_text(analog_label, analog_buf);


  // Stream live data to MQTT if connected
  if (mqttClient.connected()) {
    mqttClient.publish("livedata", buf);
  }
}

void load_calibration_data() {
  raw_values[0] = EEPROM.readInt(0);
  raw_values[1] = EEPROM.readInt(4);
  cal_values[0] = EEPROM.readInt(8);
  cal_values[1] = EEPROM.readInt(12);
}

int map_analog_value(int raw_value) {
  int cal_value = map(raw_value, raw_values[0], raw_values[1], cal_values[0], cal_values[1]);
  return cal_value;
}


void wifiTask(void *pvParameters) {
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");



  // Endpoint to list files on the SD card
  server.on("/listFiles", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!SD.begin(chipSelect)) {
      request->send(500, "text/plain", "SD card initialization failed!");
      return;
    }

    String jsonResponse = "[";
    File root = SD.open("/");
    File file = root.openNextFile();
    bool first = true;
    while (file) {
      if (!first) {
        jsonResponse += ",";
      }
      jsonResponse += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
      file = root.openNextFile();
      first = false;
    }
    jsonResponse += "]";
    request->send(200, "application/json", jsonResponse);
  });

  server.on("/deleteFile", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      if (SD.exists("/" + filename)) {
        SD.remove("/" + filename);
        request->send(200, "text/plain", "File deleted.");
      } else {
        request->send(404, "text/plain", "File not found.");
      }
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Add this endpoint to serve file content
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      File dataFile = SD.open("/" + filename, FILE_READ);
      if (!dataFile) {
        request->send(400, "text/plain", "Bad Request");
        return;
      }

      AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [dataFile](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
        size_t readLength = 0;
        if (dataFile.available()) {
          readLength = dataFile.read(buffer, maxLen);
          if (readLength == 0) {
            dataFile.close();
          }
        } else {
          dataFile.close();
        }
        return readLength;
      });

      request->send(response);
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });



  // Endpoint to get current configuration values
  server.on("/getConfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Create a JSON object with the current configuration values
    String jsonResponse = "{";
    jsonResponse += "\"raw_min\":" + String(raw_values[0]) + ",";
    jsonResponse += "\"raw_max\":" + String(raw_values[1]) + ",";
    jsonResponse += "\"cal_min\":" + String(cal_values[0]) + ",";
    jsonResponse += "\"cal_max\":" + String(cal_values[1]) + ",";
    jsonResponse += "\"interval\":" + String(dataLoggingInterval / 1000) + ",";
    jsonResponse += "\"mqtt_broker\":\"" + String(mqttBroker) + "\",";
    jsonResponse += "\"gauge_title\":\"" + String(gaugeTitle) + "\",";
    jsonResponse += "\"mqtt_port\":" + String(mqttPort) + ",";
    jsonResponse += "\"measurement_unit\":\"" + String(measurementUnit) + "\",";
    jsonResponse += "\"device_id\":\"" + String(deviceID) + "\",";
    jsonResponse += "\"serial_number\":\"" + String(serialNumber) + "\",";
    jsonResponse += "\"sd_logging\":" + String(sdLoggingEnabled ? 1 : 0);
    jsonResponse += "}";
    request->send(200, "application/json", jsonResponse);
  });


  // OTA Update handling
  server.on(
    "/otaUpdate", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200);
      ESP.restart();
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      }
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %uB\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
    });


  // Initialize web server

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate("admin", password)) {
      return request->requestAuthentication();
    }


    String htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>SensorSync IoT WebUI</title>
<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css' rel='stylesheet'>
<script src='https://code.jquery.com/jquery-3.5.1.min.js'></script>
<script src='https://kit.fontawesome.com/a076d05399.js'></script>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.1/css/all.min.css">
 <style>
   .file-list {
       list-style-type: none;
       padding: 0;
   }
   .file-list-item {
       display: flex;
       justify-content: space-between;
       margin-bottom: 10px;
       padding: 10px;
       border: 1px solid #ccc;
       border-radius: 5px;
   }
   .file-download {
       margin-left: 10px;
       display: block;
   }
   body {
       font-family: Arial, sans-serif;
       background-color: #f8f9fa;
   }
   .container {
       padding-top: 20px;
   }
   .form-row {
       display: flex;
       flex-wrap: wrap;
       gap: 10px;
   }
   .modal-content {
       backdrop-filter: blur(6px);
       box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
   }
   .card {
       margin-bottom: 20px;
   }
   .navbar {
       flex-wrap: wrap;
   }
   .form-group {
       margin-bottom: 1rem;
   }
   .row > div {
       padding: 5px;
   }
   .btn-custom {
       background-color: #0275d8;
       border-color: #0275d8;
   }
 </style>

</head>
 <body>
  <nav class="navbar navbar-expand-lg navbar-dark" style="background-color: #333; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.1);">
    <a class="navbar-brand" href="#" style="color: #f3f3f3; font-weight: 600; letter-spacing: 1px;">SensorSync IoT</a>
    <button class="navbar-toggler" type="button" data-toggle="collapse" data-target="#navbarNavAltMarkup" aria-controls="navbarNavAltMarkup" aria-expanded="false" aria-label="Toggle navigation">
      <span class="navbar-toggler-icon"></span>
    </button>
     <div class="collapse navbar-collapse" id="navbarNavAltMarkup">
       <div class="navbar-nav ml-auto">
        <button id="apiModalButton" type="button" class="btn btn-success mx-2" data-toggle="modal" data-target="#api-docs-dialog"><i class="fas fa-code"></i> API</button>
        <button id="updateModalButton" type="button" class="btn btn-primary" data-toggle="modal" data-target="#ota-dialog"><i class="fas fa-upload"></i> Update</button>
       </div>
    </div>
 </nav>
 
 <div class="container mt-4">
 
   <div class="row">
     <div class="col-md-6">
       <div class="card">
         <div class="card-body">
           <h3 class="card-title">Gauge</h3>
           <div id="gauge" class="mb-4"></div>
         </div>
       </div>
     </div>
     
     <div class="col-md-6">
       <div class="card">
         <div class="card-body">
           <h3 class="card-title">Settings</h3>
           <form id="calibrationForm">
             <h4>Calibration</h4>
             <div class="form-group row">
               <div class="col-md-6">
                 <label for="raw_min" class="col-sm-4 col-form-label">Raw Min:</label>
                 <input type="text" class="form-control" id="raw_min" name="raw_min" value="0">
               </div>
               <div class="col-md-6">
                 <label for="raw_max" class="col-sm-4 col-form-label">Raw Max:</label>
                 <input type="text" class="form-control" id="raw_max" name="raw_max" value="4095">
               </div>
               <div class="col-md-6 mt-2">
                 <label for="cal_min" class="col-sm-4 col-form-label">Cal Min:</label>
                 <input type="text" class="form-control" id="cal_min" name="cal_min" value="0">
               </div>
               <div class="col-md-6 mt-2">
                 <label for="cal_max" class="col-sm-4 col-form-label">Cal Max:</label>
                 <input type="text" class="form-control" id="cal_max" name="cal_max" value="100">
               </div>
             </div>
             <hr>
             
             <h4>MQTT Settings</h4>
             <div class="form-group row">
               <div class="col-md-6">
                 <label for="mqtt_broker" class="col-sm-4 col-form-label"><i class="fas fa-server"></i> MQTT Broker:</label>
                 <input type="text" class="form-control" id="mqtt_broker" name="mqtt_broker" value="broker.hivemq.com">
               </div>
               <div class="col-md-6">
                 <label for="mqtt_port" class="col-sm-4 col-form-label"><i class="fas fa-plug"></i> MQTT Port:</label>
                 <input type="text" class="form-control" id="mqtt_port" name="mqtt_port" value="1883">
               </div>
               <div class="col-md-6 mt-2">
                 <label for="device_id" class="col-sm-4 col-form-label"><i class="fas fa-id-card"></i> Device ID:</label>
                 <input type="text" class="form-control" id="device_id" name="device_id" value="yourDeviceID" readonly>
               </div>
               <div class="col-md-6 mt-2">
                  <label for="serial_number" class="col-sm-4 col-form-label"><i class="fas fa-barcode"></i> Serial Number:</label>
                  <input type="text" class="form-control" id="serial_number" name="serial_number" value="SN-000001">
                </div>
               <div class="col-md-6 mt-2">
                 <label for "mqtt_topics" class="col-sm-4 col-form-label"><i class="fas fa-list"></i> MQTT Topics:</label>
                 <textarea class="form-control" id="mqtt_topics" name="mqtt_topics" rows="3" readonly>yourTopics</textarea>
               </div>
             </div>
             <hr>
               
             <h4><i class="fas fa-sd-card"></i> SD Card Logging</h4>
             <div class="form-group row">
               <div class="col-md-6">
                   <label for="interval" class="col-sm-4 col-form-label"><i class="fas fa-clock"></i> Logging Interval (seconds):</label>
                   <input type="text" class="form-control" id="interval" name="interval" value="6">                   
               </div>
               <div class="col-md-6">
                   <label for="sd_logging" class="col-sm-4 col-form-label"><i class="fas fa-database"></i> SD Card Logging (Beta):</label>
                   <input type="checkbox" id="sd_logging" name="sd_logging" value="1">                 
               </div>
             </div> 
             <hr>
           
          <!--   <h4><i class="fas fa-palette"></i> Theme</h4>
             <div class="form-group row">
               <div class="col-md-6">
                 <label for="theme" class="col-sm-4 col-form-label"><i class="fas fa-paint-brush"></i> Select Theme:</label>
                 <select class="form-control" id="theme" name="theme">
                   <option value="0">Light</option>
                   <option value="1">Dark</option>
                 </select>
               </div>
             </div>
             <hr>
             -->

             <h4><i class="fas fa-ruler"></i> Measurement Unit</h4>
             <div class="form-group row">
               <div class="col-md-6">
                <label for="measurement_unit" class="col-sm-4 col-form-label"><i class="fas fa-ruler"></i> Measurement Unit:</label>
                <input type="text" class="form-control" id="measurement_unit" name="measurement_unit" value="%">
               </div>
             </div>
             <h4>Gauge Title</h4>
             <div class="form-group row">
               <div class="col-md-6">
                 <label for="gauge_title" class="col-sm-4 col-form-label">Title:</label>
                 <input type="text" class="form-control" id="gauge_title" name="gauge_title" value="LNG Fuel">
               </div>
             </div>
             <button type="submit" class="btn btn-primary">Save</button>
           </form>
         </div>

       </div>
     </div>

     <div class="col-md-6">
       <div class="card">
         <div class="card-body">
           <h3 class="card-title">SD Card Files</h3>
           <ul id="fileList" class="file-list"></ul>
         </div>
       </div>
     </div>
    </div>

  <div id="api-docs-dialog" class="modal fade">
    <div class="modal-dialog">
      <div class="modal-content">
        <div class="modal-header">
          <h5 class="modal-title">API Documentation</h5>
          <button type="button" class="close" data-dismiss="modal">&times;</button>
        </div>
        <div class="modal-body">
          <p><strong>POST /calibrate</strong> - Save calibration data</p>
          <p><strong>Example:</strong> <code>curl -X POST -d "raw_min=0&raw_max=4095&cal_min=0&cal_max=100&interval=6&mqtt_broker=broker.hivemq.com&mqtt_port=1883" http://DEVICE_IP_PLACEHOLDER/calibrate</code></p>
          <p><strong>GET /liveData</strong> - Get live data from the device</p>
          <p><strong>Example:</strong> <code>curl -X GET http://DEVICE_IP_PLACEHOLDER/liveData</code></p>
        </div>
        <div class="modal-footer">
          <button type="button" class="btn btn-secondary" data-dismiss="modal">Close</button>
        </div>
      </div>
    </div>
  </div>

  <div id="ota-dialog" class="modal fade" tabindex="-1" role="dialog">
    <div class="modal-dialog" role="document">
      <div class="modal-content">
        <div class="modal-header">
          <h5 class="modal-title">OTA Firmware Update</h5>
          <button type="button" class="close" data-dismiss="modal" aria-label="Close">
            <span aria-hidden="true">&times;</span>
          </button>
        </div>
        <div class="modal-body">
          <form id="otaForm">
            <input type="file" id="otaFile" name="otaFile" class="form-control-file" required>
          </form>
        </div>
        <div class="modal-footer">
          <button type="button" class="btn btn-secondary" data-dismiss="modal">Close</button>
          <button type="button" class="btn btn-primary" onclick="submitOtaForm()">Upload</button>
        </div>
      </div>
    </div>
  </div>


 <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"></script>
 <script src="https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js"></script>
  <script>


function fetchFiles() {
  $.ajax({
    url: '/listFiles',
    method: 'GET',
    success: function(response) {
      var fileList = $('#fileList');
      fileList.empty();
      response.forEach(function(file) {
        var listItem = $('<li class="file-list-item"></li>');
        listItem.text(file.name + ' (' + file.size + ' bytes)');
        var downloadLink = $('<a class="file-download btn btn-secondary ml-3" download></a>').text('Download').attr('href', '/download?file=' + encodeURIComponent(file.name));
        var deleteButton = $('<button class="file-delete btn btn-danger ml-3"></button>').text('Delete').attr('data-file', file.name);
        deleteButton.on('click', function() {
          deleteFile(file.name);
        });
        listItem.append(downloadLink);
        listItem.append(deleteButton);
        fileList.append(listItem);
      });
    },
    error: function() {
      console.error('Error fetching file list');
    }
  });
}

function deleteFile(filename) {
  $.ajax({
    url: '/deleteFile?file=' + encodeURIComponent(filename),
    type: 'GET',
    success: function(response) {
      alert(response);
      fetchFiles();  // Refresh file list after deletion
    },
    error: function() {
      alert('Failed to delete file: ' + filename);
    }
  });
}




 $(document).ready(function() {
  fetchFiles();  // Fetch and display files on page load
  
    // Function to update form inputs with the device's current configuration values
    function fetchConfigValues() {
      $.ajax({
        url: '/getConfig',
        type: 'GET',
        success: function(response) {
          $('#raw_min').val(response.raw_min);
          $('#raw_max').val(response.raw_max);
          $('#cal_min').val(response.cal_min);
          $('#cal_max').val(response.cal_max);
          $('#interval').val(response.interval);
          $('#mqtt_broker').val(response.mqtt_broker);
          $('#mqtt_port').val(response.mqtt_port);
          $('#device_id').val(response.device_id);
          $('#serial_number').val(response.serial_number);
          $('#gauge_title').val(response.gauge_title);
          $('#measurement_unit').val(response.measurement_unit);
          $('#sd_logging').prop('checked', response.sd_logging == 1);
          $('#gauge_title').val(response.gauge_title);
          document.getElementById('device_id').value = response.device_id;
          document.getElementById('serial_number').value = response.serial_number;
          document.getElementById('mqtt_topics').value = 'TOPICS_PLACEHOLDER';
        },
        error: function() {
          console.error('Error fetching configuration values');
        }
      });
    }
    // Fetch config values on page load
    fetchConfigValues();

    var gauge = new JustGage({
      id: "gauge", // the id of the html element
      value: 50,
      min: 0,
      max: 100,
      decimals: 2,
      gaugeWidthScale: 0.6,
      label: "kg" // Initial unit label
    });

    $('#calibrationForm').on('submit', function(event) {
      event.preventDefault();
      $.ajax({
        url: '/calibrate',
        type: 'POST',
        data: $(this).serialize(),
        success: function(response) {
          // Show notification on success
          $('body').append('<div class="alert alert-success" role="alert" id="success-alert">Calibration data saved</div>');
          fetchConfigValues();  // Re-fetch config values to update the form            
          $("#success-alert").fadeTo(2000, 500).slideUp(500, function() { $(this).remove(); });
        },
        error: function() {
          // Show notification on error
          $('body').append('<div class="alert alert-danger" role="alert" id="error-alert">Error saving data</div>');
          $("#error-alert").fadeTo(2000, 500).slideUp(500, function() { $(this).remove(); });
        }
      });
    });

    // Fetch live data every second and update the gauge
    setInterval(fetchLiveData, 2000);

    function fetchLiveData() {
      $.ajax({
        url: '/liveData',
        type: 'GET',
        success: function(response) {
          var unit = document.getElementById('measurement_unit').value;
          gauge.refresh(response.value);
          gauge.config.label = unit;
        },
        error: function() {
          console.error('Error fetching live data');
        }
      });
    }

    document.getElementById('device_id').value = 'DEVICE_ID_PLACEHOLDER';
    document.getElementById('mqtt_topics').value = 'TOPICS_PLACEHOLDER';
  });

  function showApiDocs() {
    $('#api-docs-dialog').modal('show');
  }

  function showOtaDialog() {
    $('#ota-dialog').modal('show');
  }

  function submitOtaForm() {
    var formData = new FormData();
    var fileInput = document.getElementById('otaFile');
    formData.append('otaFile', fileInput.files[0]);

      $.ajax({
        url: '/otaUpdate',
        type: 'POST',
        data: formData,
        processData: false,
        contentType: false,
        success: function(response) {
          alert('OTA Update Successful! Device will reboot.');
          $('#ota-dialog').modal('hide');
          setTimeout(function() { location.reload(); }, 3000);
        },
        error: function(response) {
          alert('OTA Update Failed! Please try again.');
        }
      });
    }

 // Event handler for form submission
   $('#calibrationForm').on('submit', function(event) {
     event.preventDefault();
     $.ajax({
       url: '/calibrate',
       type: 'POST',
       data: $(this).serialize(),
       success: function(response) {
         // Show notification on success
         $('body').append('<div class="alert alert-success" role="alert" id="success-alert">Calibration data saved</div>');
         fetchConfigValues();  // Re-fetch config values to update the form            
         $("#success-alert").fadeTo(2000, 500).slideUp(500, function() { $(this).remove(); });
       },
       error: function() {
         // Show notification on error
         $('body').append('<div class="alert alert-danger" role="alert" id="error-alert">Error saving data</div>');
         $("#error-alert").fadeTo(2000, 500).slideUp(500, function() { $(this).remove(); });
       }
     });
   });

  </script>

  <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
  <script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/raphael/2.1.4/raphael-min.js"></script>
  <script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/justgage/1.2.9/justgage.min.js"></script>

</body>
</html>

)rawliteral";

    htmlContent.replace("DEVICE_ID_PLACEHOLDER", String(deviceID));
    htmlContent.replace("DEVICE_IP_PLACEHOLDER", String(WiFi.localIP().toString()));
    htmlContent.replace("TOPICS_PLACEHOLDER", String("livedata, yourTopic"));
    request->send(200, "text/html", htmlContent);
  });

  server.on("/liveData", HTTP_GET, [](AsyncWebServerRequest *request) {
    int currentAnalogValue = analogRead(ANALOG_PIN);
    int calibratedValue = map_analog_value(currentAnalogValue);
    String json = "{\"value\":" + String(calibratedValue) + "}";
    request->send(200, "application/json", json);
  });

  server.on("/calibrate", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("sd_logging", true)) {
      sdLoggingEnabled = request->getParam("sd_logging", true)->value().toInt() == 1;
    } else {
      sdLoggingEnabled = false;
    }
    if (request->hasParam("gauge_title", true)) {
      strncpy(gaugeTitle, request->getParam("gauge_title", true)->value().c_str(), sizeof(gaugeTitle));
    }
    if (request->hasParam("measurement_unit", true)) {
      strncpy(measurementUnit, request->getParam("measurement_unit", true)->value().c_str(), sizeof(measurementUnit));
    }

    if (request->hasParam("serial_number", true)) {
      strncpy(serialNumber, request->getParam("serial_number", true)->value().c_str(), sizeof(serialNumber));
    }

    save_calibration_data();

    if (!request->authenticate("admin", password)) {
      return request->requestAuthentication();
    }

    if (request->hasParam("raw_min", true) && request->hasParam("raw_max", true) && request->hasParam("cal_min", true) && request->hasParam("cal_max", true)) {
      raw_values[0] = request->getParam("raw_min", true)->value().toInt();
      raw_values[1] = request->getParam("raw_max", true)->value().toInt();
      cal_values[0] = request->getParam("cal_min", true)->value().toInt();
      cal_values[1] = request->getParam("cal_max", true)->value().toInt();
      if (request->hasParam("interval", true)) {
        dataLoggingInterval = request->getParam("interval", true)->value().toInt() * 1000;
      }
      if (request->hasParam("mqtt_broker", true)) {
        strncpy(mqttBroker, request->getParam("mqtt_broker", true)->value().c_str(), sizeof(mqttBroker));
      }
      if (request->hasParam("mqtt_port", true)) {
        mqttPort = request->getParam("mqtt_port", true)->value().toInt();
      }

      if (request->hasParam("theme", true)) {
        int theme = request->getParam("theme", true)->value().toInt();
        EEPROM.write(0, theme);
        EEPROM.commit();
      }

      save_calibration_data();
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Missing parameters\"}");
    }
    ESP.restart();
  });

  server.begin();
  // Initialize MQTT server and client
  mqttClient.setServer(mqttBroker, mqttPort);

  while (true) {
    // Update Wi-Fi status
    update_wifi_status();

    // MQTT loop
    if (!mqttClient.connected()) {
      reconnectMqtt();
    }
    mqttClient.loop();

    delay(1000);
  }
}

void update_wifi_status() {
  if (WiFi.status() == WL_CONNECTED) {
    String ipString = WiFi.localIP().toString().c_str();
    lv_label_set_text_fmt(wifi_label, "IP: %s", ipString);  // Show IP address
    // lv_label_set_text(wifi_label, ipString);                      // Show not connected
  } else {
    lv_label_set_text(wifi_label, "WiFi Not Connected");  // Show not connected
  }
}

void deleteOldestFile() {
  File root = SD.open("/");
  File file = root.openNextFile();
  File oldestFile;
  uint32_t oldestTime = UINT32_MAX;

  while (file) {
    if (!file.isDirectory()) {
      uint32_t fileTime = file.getLastWrite();
      if (fileTime < oldestTime) {
        oldestTime = fileTime;
        oldestFile = file;
      }
    }
    file = root.openNextFile();
  }

  if (oldestFile) {
    SD.remove(oldestFile.name());
  }
}


bool isSDFull() {
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  float usedPercentage = (float)usedBytes / totalBytes * 100;
  return usedPercentage > MAX_SD_USAGE_PERCENT;
}

void logDataToSD(bool append) {
  if (!sdLoggingEnabled) return;
  if (!SD.begin(chipSelect)) {
    Serial.println("logDataToSD: SD card initialization failed!");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();  // Sync time from NTP
    lastSyncTime = millis();
  }

  char formattedTime[20];
  unsigned long rawTime = timeClient.getEpochTime();
  char filename[30];
  // Check if the time is valid
  if (rawTime > 0) {
    struct tm *timeInfo = localtime((time_t *)&rawTime);
    snprintf(formattedTime, sizeof(formattedTime), "%04d-%02d-%02d %02d:%02d:%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    snprintf(filename, sizeof(filename), "/%02d-%02d-%04d.csv", timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900);
  } else {
    snprintf(formattedTime, sizeof(formattedTime), "1970-01-01 00:00:00");  // Fallback to Unix epoch start time
    snprintf(filename, sizeof(filename), "/01-01-1970.csv");                // Fallback filename
  }

  while (isSDFull()) {
    deleteOldestFile();
  }

  File dataFile = SD.open(filename, FILE_APPEND);

  if (!dataFile) {
    Serial.println("logDataToSD: Failed to open file for writing");
    return;
  }

  dataFile.print(formattedTime);
  dataFile.print(",");
  dataFile.println(lv_label_get_text(digital_label));
  dataFile.close();
}



void reconnectMqtt() {
  if (mqttClient.connect(deviceID)) {
    Serial.println("MQTT connected");
    mqttClient.subscribe("yourTopic");
    lv_label_set_text(mqtt_status_label, "MQTT Connected");
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println("MQTT try again in 5 seconds");
    lv_label_set_text(mqtt_status_label, "MQTT Disconnected");
    delay(5000);
  }
}

// Event handler for toggle button
void toggle_view_event_cb(lv_event_t *e) {
  table = (lv_obj_t *)lv_event_get_user_data(e);
  lv_obj_t *btn_label = lv_obj_get_child(toggle_button, 0);

  if (is_table_visible) {
    lv_obj_clear_flag(table, LV_OBJ_FLAG_HIDDEN);  // Show table
    lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);    // Hide chart
    lv_label_set_text(btn_label, "C");
  } else {
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_HIDDEN);  // Show chart
    lv_obj_add_flag(table, LV_OBJ_FLAG_HIDDEN);    // Hide table
    lv_label_set_text(btn_label, "T");
  }
  is_table_visible = !is_table_visible;
}

void text_area_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_FOCUSED) {
    // Show the keyboard when the text area is focused
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED) {
    // Hide the keyboard when the text area is defocused
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  }
}

void totp_submit_event_cb(lv_event_t *e) {
  String totpInput = lv_textarea_get_text(totp_input_area);
  timeClient.update();
  lastSyncTime = millis();
  int time = timeClient.getEpochTime();
  String generatedTotp = totp.getCode(time);

  // Serial.printf("Generated TOTP: %s at Time: %d\n", generatedTotp.c_str(), time);  // Debug statement

  if (totpInput == generatedTotp) {
    // Update EEPROM with correct hash
    uint64_t chipid = ESP.getEfuseMac();
    String macStr = String((uint16_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
    String macHash = sha1(macStr);
    char newStoredHash[65];
    macHash.toCharArray(newStoredHash, 65);
    EEPROM.put(EEPROM_HASH_ADDRESS, newStoredHash);
    EEPROM.commit();

    // Reboot the device
    ESP.restart();
  } else {
    // Display error
    lv_label_set_text(lv_label_create(lv_scr_act()), "Invalid TOTP. Try again.");
  }
}

void create_totp_ui() {
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Enter TOTP to authenticate:");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

  totp_input_area = lv_textarea_create(lv_scr_act());
  lv_obj_set_size(totp_input_area, 150, 50);
  lv_obj_align(totp_input_area, LV_ALIGN_CENTER, 0, -20);

  totp_submit_button = lv_btn_create(lv_scr_act());
  lv_obj_set_size(totp_submit_button, 100, 50);
  lv_obj_align(totp_submit_button, LV_ALIGN_CENTER, 0, 50);
  lv_obj_t *btn_label = lv_label_create(totp_submit_button);
  lv_label_set_text(btn_label, "Submit");
  lv_obj_add_event_cb(totp_submit_button, totp_submit_event_cb, LV_EVENT_CLICKED, NULL);

  // Create the keyboard and hide it initially
  keyboard = lv_keyboard_create(lv_scr_act());
  lv_keyboard_set_textarea(keyboard, totp_input_area);  // Link the keyboard to the text area
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

  // Add an event handler to show the keyboard when the text area is focused
  lv_obj_add_event_cb(totp_input_area, text_area_event_cb, LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(totp_input_area, text_area_event_cb, LV_EVENT_DEFOCUSED, NULL);
}

String listFiles(fs::FS &fs, const char *dirname, uint8_t levels) {
  File root = fs.open(dirname);
  if (!root) {
    return "[]";
  }
  if (!root.isDirectory()) {
    return "[]";
  }
  String jsonResponse = "[";
  File file = root.openNextFile();
  while (file) {
    if (jsonResponse.length() > 1) {
      jsonResponse += ',';
    }
    jsonResponse += "{\"name\":\"";
    jsonResponse += file.name();
    jsonResponse += "\",\"size\":";
    jsonResponse += file.size();
    jsonResponse += "}";
    file = root.openNextFile();
  }
  jsonResponse += "]";
  return jsonResponse;
}

void printHeapStatsTimer(void *pvParameters) {
  while (1) {
    Serial.print("Heap Memory Available: ");
    Serial.println(ESP.getFreeHeap());
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // Print every 10 seconds
  }
}
