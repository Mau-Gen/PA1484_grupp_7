#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <vector>
#include <math.h>

std::vector<int> fetchNoonTemps(double lat, double lon);

// Wi-Fi credentials (Delete these before commiting to GitHub)
static const char* WIFI_SSID     = "SSID";
static const char* WIFI_PASSWORD = "PWD";

LilyGo_Class amoled;

static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t1_label;
static lv_obj_t* t2_label;
static lv_obj_t* t3_label;
static bool t2_dark = false;  // start tile #2 in light mode

// Coords for Karlskrona
const double LAT = 56.2;
const double LON = 15.5869;

enum Karlskrona{
  Station = 65090,
  Average_AirTemp = 2,
  Max_AirTemp = 20,
  Min_AirTemp = 19
};

static void on_dd_city_clicked(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    LV_LOG_USER("Option: %s", buf);
  }
}

static void on_dd_par_clicked(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    LV_LOG_USER("Option: %s", buf);
  }
}

/*
// Function: Create chart for tile 2 (Used for testing fuctionality)
static void create_temperature_chart(const std::vector<int>& temps)
{
  if (temps.empty()) {
    Serial.println("No temperature data to plot");
    return;
  }

  // Create chart object on tile 2
  lv_obj_t* chart = lv_chart_create(t2);
  lv_obj_set_size(chart, lv_disp_get_hor_res(NULL) - 40, lv_disp_get_ver_res(NULL) - 100);
  lv_obj_align(chart, LV_ALIGN_CENTER, 0, 20);
  
  // Chart settings
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, temps.size());
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -10, 30);  // Temperature range -10 to 30°C
  
  // Style
  lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(chart, lv_color_white(), 0);
  lv_obj_set_style_border_width(chart, 2, 0);
  lv_obj_set_style_border_color(chart, lv_color_black(), 0);
  lv_obj_set_style_radius(chart, 5, 0);
  
  // Add series
  series = lv_chart_add_series(chart, lv_color_hex(0x1E90FF), LV_CHART_AXIS_PRIMARY_Y);
  
  // Set temperature data
  for (size_t i = 0; i < temps.size(); i++) {
    lv_chart_set_next_value(chart, series, temps[i]);
  }
  
  // Update label to show it's a temperature graph
  lv_label_set_text(t2_label, "7-Day Temperature Forecast");
  lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 10);
  
  Serial.printf("Chart created with %d temperature points\n", temps.size());
}
*/


// Function: Creates UI
static void create_ui()
{
  // Fullscreen Tileview
  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

  // Add two horizontal tiles
  t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);

  // Tile #1
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Start screen");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t1_label, LV_ALIGN_TOP_MID, 0, 10);
  }

  // Tile #2
  {
    t2_label = lv_label_create(t2);
    lv_label_set_text(t2_label, "Line graph");
    lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 10);
  }

  // Tile #3
  {
    t3_label = lv_label_create(t3);
    lv_label_set_text(t3_label, "Settings");
    lv_obj_set_style_text_font(t3_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t3_label, LV_ALIGN_TOP_MID, 0, 10);

    // Settings

    // City

    lv_obj_t* dd_city = lv_dropdown_create(t3);
    lv_dropdown_set_options(dd_city, "Karlskrona\n"
    "Stockholm\n"
    "Visby\n");
    lv_obj_align(dd_city, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(dd_city, on_dd_city_clicked, LV_EVENT_ALL, NULL);
    
    // Parameter

    lv_obj_t* dd_par = lv_dropdown_create(t3);
    lv_dropdown_set_options(dd_par, "Temperature\n"
    "Humidity\n"
    "Rain\n");
    lv_obj_align(dd_par, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_add_event_cb(dd_par, on_dd_par_clicked, LV_EVENT_ALL, NULL);
  }

}

// Function: Connects to WIFI
static void connect_wifi()
{
  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected.");
  } else {
    Serial.println("WiFi could not connect (timeout).");
  }
}

// Must have function: Setup is run once on startup
void setup()
{
  Serial.begin(115200);
  delay(200);

  if (!amoled.begin()) {
    Serial.println("Failed to init LilyGO AMOLED.");
    while (true) delay(1000);
  }

  beginLvglHelper(amoled);   // init LVGL for this board

  create_ui();
  connect_wifi();

  // Proceed to chart creation upon boot
  std::vector<int> noonTemps = fetchNoonTemps(LAT, LON);
  
  // Create and plot the chart
  if (!noonTemps.empty()) {
    create_temperature_chart(noonTemps);
  } else {
    lv_label_set_text(t2_label, "Failed to load forecast");
    Serial.println("No temperature data received");
  }

}

// Must have function: Loop runs continously on device after setup
void loop()
{
  lv_timer_handler();
  delay(5);
}