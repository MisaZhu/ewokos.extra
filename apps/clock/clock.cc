#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/kernel_tic.h>
#include <openlibm.h>
#include <ntpc/ntpc.h>

using namespace Ewok;

class CircularClock : public Widget {
    uint32_t sec, sec_init;
    uint32_t min, min_init;
    uint32_t hour, hour_init;

    // 绘制时钟刻度
    void drawClockTicks(graph_t* g, XTheme* theme, const grect_t& r) {
        int centerX = r.x + r.w / 2;
        int centerY = r.y + r.h / 2;
        int radius = (r.w < r.h ? r.w : r.h) / 2 - 10;

        graph_set(g, r.x, r.y, r.w, r.h, 0x0);
        // 绘制白色圆底
        graph_fill_circle(g, centerX, centerY, radius, 0xff000000);
        radius -= 2;
        graph_fill_circle(g, centerX, centerY, radius, 0xffffffff);
        radius -= 2;

        // 绘制数字
        font_t* font = theme->getFont();
        int fontSize = 10;
        for (int hour = 1; hour <= 12; ++hour) {
            double angle = (hour * 30 - 90) * M_PI / 180; // 计算角度，减去 90 度是为了让 12 点在顶部
            int textX = centerX + (int)((radius - 12) * cos(angle));
            int textY = centerY + (int)((radius - 12) * sin(angle))- fontSize + 2;
            char text[3];
            snprintf(text, sizeof(text), "%d", hour);
            uint32_t textWidth, textHeight;
            font_text_size(text, font, fontSize, &textWidth, &textHeight);
            graph_draw_text_font(g, textX - textWidth / 2, textY + textHeight / 4, text, font, fontSize, 0xff000000);
        }

        for (int i = 0; i < 60; ++i) {
            double angle = i * M_PI / 30;
            int dotX = centerX + (int)(radius * cos(angle));
            int dotY = centerY + (int)(radius * sin(angle));
            int dotRadius;

            if (i % 5 == 0) {
                // 整点刻度，圆点半径大一些
                dotRadius = 1;
                graph_fill_circle(g, dotX, dotY, dotRadius, 0xff000000);
            } else {
                // 非整点刻度，圆点半径小一些
                //dotRadius = 1;
                //graph_fill_circle(g, dotX, dotY, dotRadius, 0xff000000);
            }

        }
    }

    // 绘制指针
    void drawHands(graph_t* g, XTheme* theme, const grect_t& r) {
        int centerX = r.x + r.w / 2;
        int centerY = r.y + r.h / 2;
        int radius = (r.w < r.h ? r.w : r.h) / 2 - 10;
    
        // 定义时针、分针和秒针的宽度
        int hourHandWidth = 4; // 时针最粗
        int minHandWidth = 3;  // 分针比秒针粗
        int secHandWidth = 2;  // 秒针最细
    
        // 时针
        double hourAngle = (hour % 12 + min / 60.0) * (M_PI / 6);
        int hourX = centerX + (int)(radius * 0.5 * cos(hourAngle - M_PI / 2));
        int hourY = centerY + (int)(radius * 0.5 * sin(hourAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, hourX, hourY, 0xff000000, hourHandWidth);
    
        // 分针
        double minAngle = (min + sec / 60.0) * (M_PI / 30);
        int minX = centerX + (int)(radius * 0.7 * cos(minAngle - M_PI / 2));
        int minY = centerY + (int)(radius * 0.7 * sin(minAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, minX, minY, 0xff000000, minHandWidth);
    
        // 秒针
        double secAngle = sec * (M_PI / 30);
        int secX = centerX + (int)(radius * 0.8 * cos(secAngle - M_PI / 2));
        int secY = centerY + (int)(radius * 0.8 * sin(secAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, secX, secY, 0xffff0000, secHandWidth);
    
        // 绘制指针根部的圆形
        int rootRadius = 5; // 根部圆形的半径，可以根据需要调整
        graph_fill_circle(g, centerX, centerY, rootRadius, 0xffff0000);
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        drawClockTicks(g, theme, r);
        drawHands(g, theme, r);
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        uint32_t ksec;
        kernel_tic(&ksec, NULL);
        min = min_init + (ksec / 60);
        hour = hour_init + (min / 60);
        min = min % 60;
        sec = sec_init + (ksec % 60);
        update();
    }

public:
    CircularClock() {
        sec = 0;
        min = 0;
        hour = 0;
        sec_init = 0;
        min_init = 0;
        hour_init = 0;
    }

    void updateTime() {
        time_t current_time = ntpc_get_time(DEFAULT_NTP_SERVER, DEFAULT_NTP_PORT);
        struct tm* time_info = localtime(&current_time);
        if (time_info == NULL) {
            return;
        }
        hour_init = time_info->tm_hour;
        min_init = time_info->tm_min;
        sec_init = time_info->tm_sec;
    }
};

class CircularClockWin : public WidgetWin {
protected:
    void onEvent(xevent_t* ev) {
        if (ev->type == XEVT_MOUSE) {
            if (ev->state == MOUSE_STATE_CLICK)
                this->close();
            else if (ev->state == MOUSE_STATE_DRAG)
                this->move(ev->value.mouse.rx, ev->value.mouse.ry);
        }
    }

public:
    CircularClockWin() {
        theme.setFont("system", 48);
        theme.basic.bgColor = 0xffffffff;
        theme.basic.fgColor = 0xff000000;
    }
};

int main(int argc, char** argv) {
    X x;
    CircularClockWin win;

    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);

    CircularClock* clock = new CircularClock();
    root->add(clock);

    win.open(&x, -1, -1, -1, 240, 240, "Circular Clock", XWIN_STYLE_NO_FRAME);
    clock->updateTime();
    win.setTimer(1);
    win.setAlpha(true);
    widgetXRun(&x, &win);
    return 0;
}