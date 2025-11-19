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
std::vector<int> fetchPastData(int data, int station);

static void create_temperature_chart(const std::vector<int>& data);
static void create_forecast_table(const std::vector<int>& temps);

void printVector(std::vector<int> vector){
  for (int i : vector){
    Serial.print(i);
    Serial.print(",");
  }
}

// Wi-Fi credentials (Delete these before commiting to GitHub)
static const char* WIFI_SSID     = "BTH_Guest";
static const char* WIFI_PASSWORD = "oliv95lila";

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

// Strings for the selected city in settings
String selectedCity = "Karlskrona";


// Strings for the selected parameter in settings
String selectedParam = "Temperature";

enum Karlskrona{
  Station = 65090,
  Average_AirTemp = 2,
  Max_AirTemp = 20,
  Min_AirTemp = 19
};

void updateWeatherUI() {
  Serial.println("Updating UI based on selected settings...");

  double lat, lon;
  int station;
  int param;

  if(selectedCity == "Karlskrona"){
    lat = 56.160820; lon = 15.586710; station = 65090;
  } else if(selectedCity == "Stockholm"){
    lat = 59.329323; lon = 18.068581; station = 98230;
  } else if(selectedCity == "Goteborg"){
    lat = 57.708870; lon = 11.974560; station = 71420;
  }

  if(selectedParam == "Temperature"){
    // Code for temperature in the API
    param = 2;
  } else if (selectedParam == "Rain"){
    // Code for rain amount in the API
    param = 5;
  } else if (selectedParam == "Wind Speed"){
    // Code for wind speed in the API
    param = 4;
  }

  // Fetch data
  auto noonTemps = fetchNoonTemps(lat, lon);
  auto pastData = fetchPastData(param, station);

  // Update chart
  lv_obj_clean(t3_content);
  if (!pastData.empty()){
    create_temperature_chart(pastData);
  }

  // Update table
  lv_obj_clean(t2_content);
  if (!noonTemps.empty()){
    create_forecast_table(noonTemps);
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


// Function: Create chart for tile 3 (Used for testing fuctionality)
static void create_temperature_chart(const std::vector<int>& data)
{
  if (data.empty()) {
    Serial.println("No temperature data to plot");
    return;
  }

  // Create chart object on tile 3
  lv_obj_t* chart = lv_chart_create(t3_content);
  lv_obj_set_size(chart, lv_disp_get_hor_res(NULL) - 40, lv_disp_get_ver_res(NULL) - 100);
  lv_obj_align(chart, LV_ALIGN_CENTER, 0, 20);
  
  // Chart settings
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, data.size());
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -10, 30);  // Temperature range -10 to 30°C
  
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


static void create_forecast_table(const std::vector<int>& temps)
{
  // We want exactly 7 days: today + next 6
  const int DAYS = 7;

  lv_obj_t* table = lv_table_create(t2_content);
  lv_table_set_row_cnt(table, DAYS + 1);   // +1 for header
  lv_table_set_col_cnt(table, 3);

  // Header
  lv_table_set_cell_value(table, 0, 0, "Day");
  lv_table_set_cell_value(table, 0, 1, "Icon");
  lv_table_set_cell_value(table, 0, 2, "Temperature");
  lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
  // Fill rows with temperatures
  for (int day = 0; day < DAYS; day++)
  {
    // Make sure we don't read past available data
    if (day >= temps.size()) break;

    char dayStr[16];

    snprintf(dayStr, sizeof(dayStr), "Day %d", day + 1);
    lv_table_set_cell_value(table, day + 1, 0, dayStr);
    lv_table_set_cell_value(table, day + 1, 1, "-");  // placeholder icon

    char tempStr[16];

    snprintf(tempStr, sizeof(tempStr), "%d°C", temps[day]);
    lv_table_set_cell_value(table, day + 1, 2, tempStr);
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
    lv_label_set_text(text, "Version: 0.8");
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
    lv_obj_set_size(t2_content, lv_disp_get_hor_res(NULL) - 40, lv_disp_get_ver_res(NULL) - 100);
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
    "Goteborg\n");
    lv_obj_align(dd_city, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_event_cb(dd_city, on_dd_city_clicked, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Parameter

    lv_obj_t* dd_par = lv_dropdown_create(t4);
    lv_dropdown_set_options(dd_par, "Temperature\n"
    "Rain\n"
    "Wind Speed\n");
    lv_obj_align(dd_par, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_add_event_cb(dd_par, on_dd_par_clicked, LV_EVENT_VALUE_CHANGED, NULL);
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

  // Proceed to chart creation upon boot
  std::vector<int> noonTemps = fetchNoonTemps(LAT, LON);
  std::vector<int> pastData = fetchPastData(Average_AirTemp, 65090);
  
  printVector(pastData);
  // Create and plot the chart
  if (!pastData.empty()) {
    create_temperature_chart(pastData);
  } else {
    lv_label_set_text(t3_label, "Failed to load historical data");
    Serial.println("No temperature data received");
    
  }
  
  
  if (!noonTemps.empty()) {
    create_forecast_table(noonTemps);
  } else {
    lv_label_set_text(t2_label, "Failed to load forecast data");
    Serial.println("No temperature data received");
  }
  

}

// Must have function: Loop runs continously on device after setup

void loop()
{
  lv_timer_handler();
  delay(5);
}