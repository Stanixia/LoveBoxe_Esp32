#include "Arduino.h"
#include <ArduinoJson.h>
#include "WiFi.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "FS.h"
#include "SPIFFS.h"

StaticJsonBuffer<44000> jsonBuffer;
JsonObject &root = jsonBuffer.createObject();

const char *ntpServer = "fr.pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

unsigned long timeout = 10000; // 10sec

lv_obj_t *scr_template;

lv_obj_t *bg_top;
lv_obj_t *bg_middle;
lv_obj_t *bg_box;

lv_obj_t *label_time;
lv_obj_t *label_wifi;
lv_obj_t *label_home;

lv_obj_t *dropdown_param_wifi;

static lv_obj_t *kb;
static lv_obj_t *ta_wifi_password;

static void bt_message_event(lv_obj_t *obj, lv_event_t event);
static void bt_param_event(lv_obj_t *obj, lv_event_t event);
static void bt_param_wifi_event(lv_obj_t *obj, lv_event_t event);
static void kb_event_cb(lv_obj_t *keyboard, lv_event_t e);
static void kb_create(void);
static void ta_event_cb(lv_obj_t *ta_local, lv_event_t e);
static void updateTime(String text);
static void bt_back_event_handler(lv_obj_t *obj, lv_event_t event);

TaskHandle_t ntConnectTaskHandler;

void gui_loop()
{
    lv_task_handler(); /* let the GUI do its work */
    delay(5);
}

void init_gui()
{

    Serial.println("template gui");

    scr_template = lv_obj_create(NULL, NULL);

    lv_theme_t *th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_MATERIAL_FLAG_LIGHT, LV_THEME_DEFAULT_FONT_SMALL, LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);
    lv_theme_set_act(th);

    bg_top = lv_obj_create(scr_template, NULL);
    lv_obj_clean_style_list(bg_top, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_top, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_top, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x2f3243));
    lv_obj_set_size(bg_top, LV_HOR_RES, 80);

    bg_middle = lv_obj_create(scr_template, NULL);
    lv_obj_clean_style_list(bg_middle, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_middle, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_middle, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xf3f8fe));
    lv_obj_set_size(bg_middle, LV_HOR_RES, 80);
    lv_obj_set_pos(bg_middle, 0, 80);

    bg_box = lv_obj_create(scr_template, NULL);
    lv_obj_clean_style_list(bg_box, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bg_box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bg_box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bg_box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_border_color(bg_box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xd3d3d7));
    lv_obj_set_style_local_radius(bg_box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 5);
    lv_obj_set_size(bg_box, LV_HOR_RES - 20 * 2, LV_VER_RES - 20 - 40);
    lv_obj_set_pos(bg_box, 20, 40);

    label_time = lv_label_create(scr_template, NULL);
    lv_obj_set_width(label_time, 150);
    lv_label_set_recolor(label_time, true);
    lv_label_set_text(label_time, "#ffffff 01 Jan. 0001 00:00");
    lv_obj_align(label_time, NULL, LV_ALIGN_IN_TOP_RIGHT, -20, 10);

    label_wifi = lv_label_create(scr_template, NULL);
    lv_label_set_recolor(label_wifi, true);
    lv_label_set_text(label_wifi, "#ff0000 \xef\x87\xab");
    lv_obj_align(label_wifi, NULL, LV_ALIGN_IN_TOP_LEFT, 25, 11);

    lv_disp_load_scr(scr_template);
}

void home()
{
    Serial.println("home gui");

    init_gui();

    lv_obj_t *bt_message = lv_btn_create(bg_box, NULL);
    lv_obj_set_event_cb(bt_message, bt_message_event);
    lv_obj_clean_style_list(bt_message, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_message, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_message, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_message, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_message, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_message, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_message, 110, 140);
    lv_obj_align(bt_message, bg_box, LV_ALIGN_IN_TOP_LEFT, 20, 20);
    lv_obj_t *label_bt_message = lv_label_create(bt_message, NULL);
    lv_label_set_text(label_bt_message, "Message");

    lv_obj_t *bt_param = lv_btn_create(bg_box, NULL);
    lv_obj_set_event_cb(bt_param, bt_param_event);
    lv_obj_clean_style_list(bt_param, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_param, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_param, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_param, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_param, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_param, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_param, 110, 140);
    lv_obj_align(bt_param, bg_box, LV_ALIGN_IN_TOP_RIGHT, -20, 20);
    lv_obj_t *label_bt_param = lv_label_create(bt_param, NULL);
    lv_label_set_text(label_bt_param, "Parametres");

    lv_scr_load(scr_template);
}

void gui_param()
{
    Serial.println("param gui");

    init_gui();

    lv_obj_t *bt_param_wifi = lv_btn_create(bg_box, NULL);
    lv_obj_set_event_cb(bt_param_wifi, bt_param_wifi_event);
    lv_obj_clean_style_list(bt_param_wifi, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_param_wifi, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_param_wifi, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_param_wifi, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_param_wifi, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_param_wifi, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_param_wifi, 110, 60);
    lv_obj_align(bt_param_wifi, bg_box, LV_ALIGN_IN_TOP_LEFT, 20, 20);
    lv_obj_t *label_bt_param_wifi = lv_label_create(bt_param_wifi, NULL);
    lv_label_set_text(label_bt_param_wifi, "Wifi");

    lv_obj_t *bt_param_led = lv_btn_create(bg_box, NULL);
    // lv_obj_set_event_cb(bt_param_led, event_handler);
    lv_obj_clean_style_list(bt_param_led, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_param_led, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_param_led, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_param_led, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_param_led, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_param_led, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_param_led, 110, 60);
    lv_obj_align(bt_param_led, bg_box, LV_ALIGN_IN_TOP_RIGHT, -20, 20);
    lv_obj_t *label_bt_param_led = lv_label_create(bt_param_led, NULL);
    lv_label_set_text(label_bt_param_led, "Led");

    lv_obj_t *bt_back = lv_btn_create(bg_box, NULL);
    lv_obj_set_event_cb(bt_back, bt_back_event_handler);
    lv_obj_clean_style_list(bt_back, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_back, 110, 60);
    lv_obj_align(bt_back, bg_box, LV_ALIGN_IN_BOTTOM_MID, 0, 0 - 20);
    lv_obj_t *label_bt_back = lv_label_create(bt_back, NULL);
    lv_label_set_text(label_bt_back, "Retour");

    lv_scr_load(scr_template);
}

void wirelessTask(void *pvParameters)
{
    unsigned long startingTime = millis();

    File file = SPIFFS.open("/config.json", "r");
    size_t size = file.size();
    std::unique_ptr<char[]> buf(new char[size]);
    file.readBytes(buf.get(), size);
    JsonObject &root = jsonBuffer.parseObject(buf.get());

    const char *ssidName = root["ssid"];
    const char *password = root["pdw"];

    Serial.println(ssidName);
    Serial.println(password);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidName, password);
    while (WiFi.status() != WL_CONNECTED && (millis() - startingTime) < timeout)
    {
        Serial.print(".");
        vTaskDelay(250);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        lv_label_set_text(label_wifi, "#ffffff \xef\x87\xab");
    }

    configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", ntpServer);

    while (WiFi.status() == WL_CONNECTED)
    {
        lv_label_set_text(label_wifi, "#ffffff \xef\x87\xab");
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time");
            return;
        }
        char timeStringBuff[19];
        strftime(timeStringBuff, sizeof(timeStringBuff), "%d %b. %Y %H:%M", &timeinfo);
        updateTime(String(timeStringBuff));
        vTaskDelay(1000);
    }

    vTaskDelete(NULL);
}

static void updateTime(String text)
{
    lv_label_set_text(label_time, ("#ffffff " + text).c_str());
}

void wirelessScan()
{

    int numSsid = WiFi.scanNetworks();
    String SsidFind = "";
    if (numSsid == -1)
    {
        lv_dropdown_set_options(dropdown_param_wifi, "Aucune connexion Wifi disponnible");
        while (true)
            ;
    }
    for (int thisNet = 0; thisNet < numSsid; thisNet++)
    {
        SsidFind += WiFi.SSID(thisNet) + "\n";
    }
    if (SsidFind != "")
    {
        int str_len = SsidFind.length() + 1;
        char char_array[str_len];
        SsidFind.toCharArray(char_array, str_len);
        lv_dropdown_set_options(dropdown_param_wifi, char_array);
    }
}

void gui_param_wifi()
{

    Serial.println("param wifi gui");

    init_gui();

    dropdown_param_wifi = lv_dropdown_create(bg_box, NULL);
    // v_obj_set_event_cb(dropdown_param_wifi, event_handler_wifi);
    lv_obj_set_size(dropdown_param_wifi, 240, 30);
    lv_obj_align(dropdown_param_wifi, bg_box, LV_ALIGN_IN_TOP_MID, 0, 10);

    ta_wifi_password = lv_textarea_create(bg_box, NULL);
    lv_textarea_set_one_line(ta_wifi_password, true);
    lv_obj_set_event_cb(ta_wifi_password, ta_event_cb);
    lv_obj_set_size(ta_wifi_password, 240, 35);
    lv_obj_align(ta_wifi_password, NULL, LV_ALIGN_IN_TOP_MID, 0, 50);
    lv_textarea_set_text(ta_wifi_password, "");
    lv_textarea_set_placeholder_text(ta_wifi_password, "Password");

    lv_obj_t *bt_back = lv_btn_create(bg_box, NULL);
    lv_obj_set_event_cb(bt_back, bt_back_event_handler);
    lv_obj_clean_style_list(bt_back, LV_OBJ_PART_MAIN);
    lv_obj_set_style_local_bg_opa(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_bg_color(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
    lv_obj_set_style_local_border_width(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x49a3b1));
    lv_obj_set_style_local_radius(bt_back, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_obj_set_size(bt_back, 110, 60);
    lv_obj_align(bt_back, bg_box, LV_ALIGN_IN_BOTTOM_MID, 0, 0 - 20);
    lv_obj_t *label_bt_back = lv_label_create(bt_back, NULL);
    lv_label_set_text(label_bt_back, "Retour");

    lv_scr_load(scr_template);

    wirelessScan();
}

static void bt_message_event(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        printf("Clicked\n");
    }
}

static void bt_param_event(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        gui_param();
    }
}

static void bt_param_wifi_event(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        gui_param_wifi();
    }
}

static void kb_event_cb(lv_obj_t *keyboard, lv_event_t e)
{
    lv_keyboard_def_event_cb(kb, e);
    if (e == LV_EVENT_CANCEL)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(kb);
        kb = NULL;
    }
    if (e == LV_EVENT_APPLY)
    {
        char buf[32];
        lv_dropdown_get_selected_str(dropdown_param_wifi, buf, sizeof(buf));
        root["ssid"] = buf;
        root["pdw"] = lv_textarea_get_text(ta_wifi_password);
        File configFile = SPIFFS.open("/config.json", "w");
        root.printTo(configFile);
        configFile.close();

        ESP.restart();
    }
}

static void kb_create(void)
{
    kb = lv_keyboard_create(lv_scr_act(), NULL);
    lv_keyboard_set_cursor_manage(kb, true);
    lv_obj_set_event_cb(kb, kb_event_cb);
    lv_keyboard_set_textarea(kb, ta_wifi_password);
}

static void ta_event_cb(lv_obj_t *ta_local, lv_event_t e)
{
    if (e == LV_EVENT_CLICKED && kb == NULL)
    {
        kb_create();
    }
}

static void bt_back_event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED)
    {
        home();
    }
}