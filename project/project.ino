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
#include <Preferences.h>

std::vector<int> fetchNoonData(String str, double lat, double lon);
std::vector<int> fetchPastData(int data, int station);

static void create_temperature_chart(const std::vector<int>& data);
static void create_forecast_table(const std::vector<int>& temps);
static void create_settings_button();

void printVector(std::vector<int> vector){
  for (int i : vector){
    Serial.print(i);
    Serial.print(",");
  }
}

// Wi-Fi credentials (Delete these before commiting to GitHub)
static const char* WIFI_SSID     = "Agartha Entry Point";
static const char* WIFI_PASSWORD = "mackan12";

LilyGo_Class amoled;

static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t4;
static lv_obj_t* t1_label;
static lv_obj_t* t2_label;
static lv_obj_t* t3_label;
static lv_obj_t* t4_label;
static lv_obj_t* t2_content;
static lv_obj_t* t3_content;

// Coords for Karlskrona
const double LAT = 56.2;
const double LON = 15.5869;

// Last tile before the switch to settings
int currentTile = 0;

// Global variables

String selectedParam;
String selectedCity;
String str;

Preferences prefs;
// Saves the selected parameters so that they are saved even after restart.
void saveDefaults() {
  prefs.begin("weather", false);
  prefs.putString("city", selectedCity);
  prefs.putString("param", selectedParam);
  prefs.putString("str", str);
  prefs.end();
}
// Loads the selected parameters after a restart and when going to press the default button.
void loadDefaults() {
  prefs.begin("weather", true);
  selectedCity = prefs.getString("city", "Karlskrona");
  selectedParam = prefs.getString("param", "Temperature");
  str = prefs.getString("str", "air_temperature");
  prefs.end();

  updateWeatherUI();
}

struct CityParam{
  float lat;
};



void updateWeatherUI() {
  Serial.println("Updating UI based on selected settings...");

  double lat, lon;
  int station;
  int param;
  String str;

  if(selectedCity == "Karlskrona"){
    lat = 56.160820; lon = 15.586710; station = 65090;
  } else if(selectedCity == "Stockholm"){
    lat = 59.329323; lon = 18.068581; station = 97400;
  } else if(selectedCity == "Goteborg"){
    lat = 57.708870; lon = 11.974560; station = 72420;
  } else if(selectedCity == "Malmo"){
    lat = 55.604980; lon = 13.003822; station = 53300;
  } else if(selectedCity == "Kiruna"){
    lat = 67.855797; lon = 20.225283; station = 180940;
  }

  if(selectedParam == "Temperature"){
    // Code for temperature in the API
    param = 1;
    str = "air_temperature";
  } else if (selectedParam == "Humidity"){
    // Code for humidity in the API
    param = 6;
    str = "relative_humidity";
  } else if (selectedParam == "Wind_speed"){
    // Code for Wind_speed in the API
    param = 4;
    str = "wind_speed";
    // Code for air pressure
  } else if (selectedParam == "Air_pressure"){
    param = 9;
    str = "air_pressure_at_mean_sea_level";
  }

  // Fetch data
  auto noonData = fetchNoonData(str, lat, lon);
  auto pastData = fetchPastData(param, station);

  // Update chart
  // SAFELY DELETE OLD CHART
  lv_obj_t* child_chart = lv_obj_get_child(t3_content, 0);
  while (child_chart) {
    lv_obj_del(child_chart);
    child_chart = lv_obj_get_child(t3_content, 0);
  }
  lv_refr_now(NULL);  // IMPORTANT

  // Now create the new chart
  if (!pastData.empty()) {
      lv_obj_update_layout(t3_content);
      create_temperature_chart(pastData);
  } else {
      lv_obj_t* label = lv_label_create(t3_content);
      lv_label_set_text(label, "No historical data");
      lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  }


  // Update table
  // SAFELY DELETE OLD TABLE
  lv_obj_t* child_table = lv_obj_get_child(t2_content, 0);
  while (child_table) {
    lv_obj_del(child_table);
    child_table = lv_obj_get_child(t2_content, 0);
  }
  lv_refr_now(NULL);

  // Now create the new table
  if (!noonData.empty()){
    lv_obj_update_layout(t2_content);
    create_forecast_table(noonData);
  } else {
    lv_obj_t* label = lv_label_create(t2_content);
    lv_label_set_text(label, "No forecast data");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  }
}

static void on_dd_city_clicked(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    char buf[32];
    lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
    LV_LOG_USER("Option: %s", buf);
    selectedCity = buf;
    selectedCity.trim();
    updateWeatherUI();
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
    selectedParam = buf;
    selectedParam.trim();
    updateWeatherUI();
  }
}

static void create_settings_button()
{
  lv_obj_t* label;

  // Settings button on Tile 2
  lv_obj_t* setting_button1 = lv_btn_create(t2);
  lv_obj_add_event_cb(setting_button1, [](lv_event_t* e){
      if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
          currentTile = 1; // we came from tile 2
          lv_obj_scroll_to_view(t4, LV_ANIM_ON); // go to settings
      }
  }, LV_EVENT_ALL, NULL);
  lv_obj_align(setting_button1, LV_ALIGN_TOP_RIGHT, 0, 0);
  label = lv_label_create(setting_button1);
  lv_label_set_text(label, "Settings");
  lv_obj_center(label);

  // Settings button on Tile 3
  lv_obj_t* setting_button2 = lv_btn_create(t3);
  lv_obj_add_event_cb(setting_button2, [](lv_event_t* e){
      if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
          currentTile = 2; // we came from tile 3
          lv_obj_scroll_to_view(t4, LV_ANIM_ON); // go to settings
      }
  }, LV_EVENT_ALL, NULL);
  lv_obj_align(setting_button2, LV_ALIGN_TOP_RIGHT, 0, 0);
  label = lv_label_create(setting_button2);
  lv_label_set_text(label, "Settings");
  lv_obj_center(label);

  // Back button on Settings tile
  lv_obj_t* back_btn = lv_btn_create(t4);
  lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_add_event_cb(back_btn, [](lv_event_t* e){
      if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
          if (currentTile == 1) lv_obj_scroll_to_view(t2, LV_ANIM_ON);
          else if (currentTile == 2) lv_obj_scroll_to_view(t3, LV_ANIM_ON);
          else lv_obj_scroll_to_view(t1, LV_ANIM_ON); // fallback
      }
  }, LV_EVENT_ALL, NULL);

  lv_obj_t* back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, "Back");
  lv_obj_center(back_label);
}

static void create_save_buttons()
{
  lv_obj_t* default_button = lv_btn_create(t4);
  lv_obj_align(default_button, LV_ALIGN_CENTER, 0, 40);
  lv_obj_add_event_cb(default_button, [](lv_event_t* e){
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
      loadDefaults();
    }
  }, LV_EVENT_ALL, NULL);
  lv_obj_t* default_label = lv_label_create(default_button);
  lv_label_set_text(default_label, "Default");
  lv_obj_center(default_label);

  lv_obj_t* save_button = lv_btn_create(t4);
  lv_obj_align(save_button, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(save_button, [](lv_event_t* e){
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
      saveDefaults();
    }
  }, LV_EVENT_ALL, NULL);
  lv_obj_t* save_label = lv_label_create(save_button);
  lv_label_set_text(save_label, "Save");
  lv_obj_center(save_label);

}

// Function: Create chart for tile 3 (Used for testing fuctionality)
static void create_temperature_chart(const std::vector<int>& data)
{
  if (data.empty()) {
    Serial.println("No temperature data to plot");
    return;
  }

  // Create chart object on tile 3
  lv_obj_t* chart = lv_chart_create(t3_content);
  lv_obj_set_size(chart, lv_obj_get_width(t3_content) - 50, lv_obj_get_height(t3_content) - 80);
  lv_obj_align_to(chart, NULL, LV_ALIGN_CENTER, 0, 0);
  
  // Chart settings
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, data.size());
  if(selectedParam == "Temperature") {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -30, 30);  // Temperature range -10 to 30°C
  } else if(selectedParam == "Air_pressure") {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 950, 1050); // Air pressure range 950 to 1050
  } else if(selectedParam == "Humidity") {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100); // Humidity range 0 to 100
  } else if(selectedParam == "Wind_speed") {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 30); // Wind speed range 0 to 30
  }

  // Style
  lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(chart, lv_color_white(), 0);
  lv_obj_set_style_border_width(chart, 2, 0);
  lv_obj_set_style_border_color(chart, lv_color_black(), 0);
  lv_obj_set_style_radius(chart, 5, 0);
  
  // Add series
  lv_chart_series_t* series = lv_chart_add_series(chart, lv_color_hex(0x1E90FF), LV_CHART_AXIS_PRIMARY_Y);
  
  // Set temperature data
  for (size_t i = 0; i < data.size(); i++) {
    lv_chart_set_next_value(chart, series, data[i]);
  }
  
  // Update label to show it's a temperature graph
  lv_label_set_text(t3_label, "Historical Weather Graph");
  lv_obj_align(t3_label, LV_ALIGN_TOP_MID, 0, 10);
  
  Serial.printf("Chart created with %d temperature points\n", data.size());
}


static void create_forecast_table(const std::vector<int>& data)
{
  // We want exactly 7 days: today + next 6
  const int DAYS = 7;

  lv_obj_t* table = lv_table_create(t2_content);
  lv_table_set_row_cnt(table, DAYS + 1);   // +1 for header
  lv_table_set_col_cnt(table, 3);
  lv_obj_set_size(table, lv_obj_get_width(t2_content) - 180, lv_obj_get_height(t2_content) - 60);
  lv_obj_align_to(table, NULL, LV_ALIGN_CENTER, 0, 0);

  // Header
  lv_table_set_cell_value(table, 0, 0, "Day");
  lv_table_set_cell_value(table, 0, 1, "Icon");
  if(selectedParam == "Temperature"){
    lv_table_set_cell_value(table, 0, 2, "Temperature");
  }
  if(selectedParam == "Air_pressure"){
    lv_table_set_cell_value(table, 0, 2, "Air_pressure");
  }
  if(selectedParam == "Wind_speed"){
    lv_table_set_cell_value(table, 0, 2, "Wind_speed");
  }
  if(selectedParam == "Humidity"){
    lv_table_set_cell_value(table, 0, 2, "Humidity");
  }
  lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
  // Fill rows with temperatures
  for (int day = 0; day < DAYS; day++)
  {
    // Make sure we don't read past available data
    if (day >= data.size()) break;

    char dayStr[16];

    snprintf(dayStr, sizeof(dayStr), "Day %d", day + 1);
    lv_table_set_cell_value(table, day + 1, 0, dayStr);
    lv_table_set_cell_value(table, day + 1, 1, "-");  // placeholder icon

    char dataStr[16];
    if(selectedParam == "Temperature"){
      snprintf(dataStr, sizeof(dataStr), "%d°C", data[day]);
      lv_table_set_cell_value(table, day + 1, 2, dataStr);
    }

    if(selectedParam == "Air_pressure"){
      snprintf(dataStr, sizeof(dataStr), "%dhPa", data[day]);
      lv_table_set_cell_value(table, day + 1, 2, dataStr);
    }

    if(selectedParam == "Wind_speed"){
      snprintf(dataStr, sizeof(dataStr), "%dm/s", data[day]);
      lv_table_set_cell_value(table, day + 1, 2, dataStr);
    }

    if(selectedParam == "Humidity"){
      snprintf(dataStr, sizeof(dataStr), "%", data[day]);
      lv_table_set_cell_value(table, day + 1, 2, dataStr);
    }
  }
}


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
  t4 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);
  

  // Tile #1
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Group 7");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_32, 0);
    lv_obj_align(t1_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_t* text = lv_label_create(t1);
    lv_label_set_text(text, "Version: 2.7");
    lv_obj_set_style_text_font(text, &lv_font_montserrat_20, 0);
    lv_obj_align(text, LV_ALIGN_CENTER, 0, 10);
  }

  // Tile #2
  {
    t2_label = lv_label_create(t2);
    lv_label_set_text(t2_label, "Weather forecast");
    lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 10);

    // Persistent content container for forecast table
    t2_content = lv_obj_create(t2);
    lv_obj_set_size(t2_content, lv_disp_get_hor_res(NULL) - 20, lv_disp_get_ver_res(NULL) - 100);
    lv_obj_align(t2_content, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(t2_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(t2_content, 0, 0);
  }

  // Tile #3
  {
    t3_label = lv_label_create(t3);
    lv_label_set_text(t3_label, "Line Graph");
    lv_obj_set_style_text_font(t3_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t3_label, LV_ALIGN_TOP_MID, 0, 10);

    // Persistent content container for chart
    t3_content = lv_obj_create(t3);
    lv_obj_set_size(t3_content, lv_disp_get_hor_res(NULL) - 40, lv_disp_get_ver_res(NULL) - 100);
    lv_obj_align(t3_content, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(t3_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(t3_content, 0, 0);
  }
  // Tile #4
  {
    t4_label = lv_label_create(t4);
    lv_label_set_text(t4_label, "Settings");
    lv_obj_set_style_text_font(t4_label, &lv_font_montserrat_28, 0);
    lv_obj_align(t4_label, LV_ALIGN_TOP_MID, 0, 10);

    // Settings

    // City

    lv_obj_t* dd_city = lv_dropdown_create(t4);
    lv_dropdown_set_options(dd_city, "Karlskrona\n"
    "Stockholm\n"
    "Goteborg\n"
    "Malmo\n"
    "Kiruna\n");
    lv_obj_align(dd_city, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_event_cb(dd_city, on_dd_city_clicked, LV_EVENT_VALUE_CHANGED, NULL);

    // Show saved default city
    lv_dropdown_set_text(dd_city, selectedCity.c_str());
    
    // Parameter

    lv_obj_t* dd_par = lv_dropdown_create(t4);
    lv_dropdown_set_options(dd_par, "Temperature\n"
    "Humidity\n"
    "Wind_speed\n"
    "Air_pressure\n");
    lv_obj_align(dd_par, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_add_event_cb(dd_par, on_dd_par_clicked, LV_EVENT_VALUE_CHANGED, NULL);

    // Show saved default parameter

    lv_dropdown_set_text(dd_par, selectedParam.c_str());
  }

  lv_obj_scroll_to(tileview, 0, 0, LV_ANIM_OFF);
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

  loadDefaults();

  create_settings_button();
  create_save_buttons();
}

// Must have function: Loop runs continously on device after setup

void loop()
{
  lv_timer_handler();
  delay(5);
}