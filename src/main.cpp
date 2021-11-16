#include <Arduino.h>
#include "WiFi.h"
#include <stdio.h>
#include "gui.h"
#include <FS.h>
#include "SPIFFS.h"

int screenWidth = 320;
int screenHeight = 240;
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

TFT_eSPI tft = TFT_eSPI();

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char *file, uint32_t line, const char *fn_name, const char *dsc)
{
    Serial.printf("%s(%s)@%d->%s\r\n", file, fn_name, line, dsc);
    Serial.flush();
}
#endif

/* Display flushing */
void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

bool touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY, 600);

    if (!touched)
    {
        return false;
    }

    if (touchX > screenWidth || touchY > screenHeight)
    {
        // Serial.println("Y or y outside of expected parameters..");
        // Serial.print("y:");
        // Serial.print(touchX);
        // Serial.print(" x:");
        // Serial.print(touchY);
    }
    else
    {

        // Serial.print("y:");
        // Serial.print(touchX);
        // Serial.print(" x:");
        // Serial.print(touchY);

        data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        data->point.x = touchX;
        data->point.y = touchY;
    }

    return false; /*Return `false` because we are not buffering and no more data to read*/
}

void setup()
{
    Serial.begin(115200);

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    lv_init();

    tft.begin();
    tft.setRotation(1);

    uint16_t calData[5] = {391, 3511, 204, 3605, 7};
    tft.setTouch(calData);

    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    xTaskCreate(&wirelessTask, "wireless", 2048, NULL, 0, &ntConnectTaskHandler);

    home();
    // xTaskCreate(&guiTask, "gui", 2048, NULL, 2, NULL);
}

void loop()
{
    gui_loop();
}