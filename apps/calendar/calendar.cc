#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/SpriteWin.h>
#include <Widget/Container.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace Ewok;

// 颜色定义 - 按照图片风格
static const uint32_t COLOR_HEADER_RED = 0xFFD32F2F;      // 红色顶部标题栏
static const uint32_t COLOR_HEADER_TEXT = 0xFFFFFFFF;      // 白色标题文字
static const uint32_t COLOR_CELL_BG = 0xFFFFFFFF;          // 白色日期格子背景
static const uint32_t COLOR_CELL_TEXT = 0xFF000000;        // 黑色日期数字
static const uint32_t COLOR_TODAY_BG = 0xFFFFCDD2;         // 今天高亮背景（浅红色）
static const uint32_t COLOR_TODAY_TEXT = 0xFFD32F2F;       // 今天高亮文字（红色）
static const uint32_t COLOR_BORDER = 0xFF888888;           // 边框颜色
static const uint32_t COLOR_WEEKDAY_BG = 0xFFEEEEEE;       // 星期行背景
static const uint32_t COLOR_WEEKDAY_TEXT = 0xFF333333;     // 星期文字颜色
static const uint32_t COLOR_OTHER_MONTH = 0xFFAAAAAA;      // 其他月份日期颜色
static const uint32_t COLOR_SHADOW = 0xFF666666;           // 阴影颜色

// 月份名称
static const char* MONTH_NAMES[] = {
    "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December"
};

// 星期名称
static const char* WEEKDAY_NAMES[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// 日历组件
class CalendarWidget : public Widget {
private:
    int currentYear;
    int currentMonth;
    int currentDay;
    int todayYear;
    int todayMonth;
    int todayDay;
    
    // 计算某月有多少天
    int getDaysInMonth(int year, int month) {
        static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month == 2 && isLeapYear(year)) return 29;
        return days[month - 1];
    }
    
    // 判断闰年
    bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }
    
    // 计算某月第一天是星期几 (0=周日, 1=周一, ...)
    int getFirstDayOfMonth(int year, int month) {
        // Zeller公式简化版
        int m = month;
        int y = year;
        if (m < 3) {
            m += 12;
            y--;
        }
        int c = y / 100;
        int d = y % 100;
        int f = 1; // 1号
        
        int w = (d + d/4 + c/4 - 2*c + 26*(m+1)/10 + f - 1) % 7;
        if (w < 0) w += 7;
        return w;
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 绘制阴影边框（3D效果）
        int shadowOffset = 4;
        graph_fill(g, r.x + shadowOffset, r.y + shadowOffset, r.w, r.h, COLOR_SHADOW);
        
        // 绘制主背景
        graph_fill(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);
        
        // 绘制外边框
        graph_box(g, r.x, r.y, r.w, r.h, COLOR_BORDER);
        
        // 计算布局
        int padding = 13;
        int headerHeight = 50;
        int weekdayHeight = 30;
        int cellHeight = (r.h - headerHeight - weekdayHeight - padding * 2) / 6;
        // 内容区域宽度（不包括外边框）
        int contentW = r.w - padding * 2;
        // 每个格子宽度 = 内容宽度 / 7（包含边框在内）
        int cellWidth = contentW / 7;

        int contentX = r.x + padding;
        int contentY = r.y + padding;
        
        // 绘制红色顶部标题栏
        graph_fill(g, contentX, contentY, contentW, headerHeight, COLOR_HEADER_RED);
        
        // 绘制左右切换按钮区域
        int buttonWidth = 40;
        int buttonHeight = headerHeight - 10;
        int buttonY = contentY + 5;
        
        // 左按钮（上一个月）
        int leftBtnX = contentX + 5;
        graph_fill_round(g, leftBtnX, buttonY, buttonWidth, buttonHeight, 8, 0x99FFFFFF);
        // 绘制左箭头 <
        int arrowY = buttonY + buttonHeight / 2;
        int arrowLeftX = leftBtnX + 12;
        int arrowRightX = leftBtnX + buttonWidth - 12;
        int arrowTopY = arrowY - 8;
        int arrowBottomY = arrowY + 8;
        graph_line(g, arrowRightX, arrowTopY, arrowLeftX, arrowY, COLOR_HEADER_TEXT);
        graph_line(g, arrowRightX, arrowBottomY, arrowLeftX, arrowY, COLOR_HEADER_TEXT);
        
        // 右按钮（下一个月）
        int rightBtnX = contentX + contentW - buttonWidth - 5;
        graph_fill_round(g, rightBtnX, buttonY, buttonWidth, buttonHeight, 8, 0x99FFFFFF);
        // 绘制右箭头 >
        arrowLeftX = rightBtnX + 12;
        arrowRightX = rightBtnX + buttonWidth - 12;
        graph_line(g, arrowLeftX, arrowTopY, arrowRightX, arrowY, COLOR_HEADER_TEXT);
        graph_line(g, arrowLeftX, arrowBottomY, arrowRightX, arrowY, COLOR_HEADER_TEXT);
        
        // 绘制月份标题文字
        char title[64];
        snprintf(title, sizeof(title), "%s %d", MONTH_NAMES[currentMonth - 1], currentYear);
        
        uint32_t tw, th;
        font_text_size(title, theme->getFont(), theme->basic.fontSize + 12, &tw, &th);
        int titleX = contentX + (contentW - (int32_t)tw) / 2;
        int titleY = contentY + (headerHeight - (int32_t)th) / 2;
        graph_draw_text_font(g, titleX, titleY, title, theme->getFont(),
                            theme->basic.fontSize + 12, COLOR_HEADER_TEXT);
        
        // 绘制星期行背景
        int weekdayY = contentY + headerHeight;
        graph_fill(g, contentX, weekdayY, contentW, weekdayHeight, COLOR_WEEKDAY_BG);
        
        // 绘制星期文字
        for (int i = 0; i < 7; i++) {
            int cellX = contentX + i * cellWidth;
            font_text_size(WEEKDAY_NAMES[i], theme->getFont(), theme->basic.fontSize, &tw, &th);
            int textX = cellX + (cellWidth - (int32_t)tw) / 2;
            int textY = weekdayY + (weekdayHeight - (int32_t)th) / 2;
            graph_draw_text_font(g, textX, textY, WEEKDAY_NAMES[i], theme->getFont(),
                                theme->basic.fontSize, COLOR_WEEKDAY_TEXT);
        }
        
        // 绘制日期格子
        int calendarStartY = weekdayY + weekdayHeight;
        int firstDay = getFirstDayOfMonth(currentYear, currentMonth);
        int daysInMonth = getDaysInMonth(currentYear, currentMonth);
        
        // 上个月的日期
        int prevMonthDays = getDaysInMonth(currentMonth == 1 ? currentYear - 1 : currentYear, 
                                           currentMonth == 1 ? 12 : currentMonth - 1);
        int prevMonthStartDay = prevMonthDays - firstDay + 1;
        
        // 绘制6行 x 7列的格子
        for (int row = 0; row < 6; row++) {
            for (int col = 0; col < 7; col++) {
                int cellX = contentX + col * cellWidth;
                int cellY = calendarStartY + row * cellHeight;

                // 绘制格子边框
                graph_box(g, cellX, cellY, cellWidth, cellHeight, COLOR_BORDER);

                // 计算日期
                int dayNum = 0;
                bool isCurrentMonth = false;
                bool isToday = false;

                int cellIndex = row * 7 + col;
                if (cellIndex < firstDay) {
                    // 上个月的日期
                    dayNum = prevMonthStartDay + cellIndex;
                } else if (cellIndex < firstDay + daysInMonth) {
                    // 当前月的日期
                    dayNum = cellIndex - firstDay + 1;
                    isCurrentMonth = true;
                    // 检查是否是今天
                    if (dayNum == todayDay && currentMonth == todayMonth && currentYear == todayYear) {
                        isToday = true;
                    }
                } else {
                    // 下个月的日期
                    dayNum = cellIndex - firstDay - daysInMonth + 1;
                }

                // 绘制背景
                if (isToday) {
                    graph_fill(g, cellX + 1, cellY + 1, cellWidth - 2, cellHeight - 2, COLOR_TODAY_BG);
                } else {
                    graph_fill(g, cellX + 1, cellY + 1, cellWidth - 2, cellHeight - 2, COLOR_CELL_BG);
                }

                // 绘制日期数字
                char dayStr[16];
                snprintf(dayStr, sizeof(dayStr), "%d", dayNum);

                uint32_t textColor = isCurrentMonth ?
                    (isToday ? COLOR_TODAY_TEXT : COLOR_CELL_TEXT) : COLOR_OTHER_MONTH;

                font_text_size(dayStr, theme->getFont(), theme->basic.fontSize + 4, &tw, &th);
                int textX = cellX + (cellWidth - (int32_t)tw) / 2;
                int textY = cellY + (cellHeight - (int32_t)th) / 2;
                graph_draw_text_font(g, textX, textY, dayStr, theme->getFont(),
                                    theme->basic.fontSize + 4, textColor);
            }
        }
    }
    
    bool onMouse(xevent_t* ev) {
        if (ev->state == MOUSE_STATE_UP) {
            // 获取点击位置
            gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
            int x = pos.x;
            int y = pos.y;

            // 计算按钮区域
            int padding = 10;
            int headerHeight = 50;
            int buttonWidth = 40;
            int buttonHeight = headerHeight - 10;
            int buttonY = padding + 5;
            int leftBtnX = padding + 5;
            int rightBtnX = area.w - padding - buttonWidth - 5;

            // 检查是否点击在标题栏区域
            if (y >= buttonY && y <= buttonY + buttonHeight) {
                if (x >= leftBtnX && x <= leftBtnX + buttonWidth) {
                    // 点击左按钮：上一个月
                    currentMonth--;
                    if (currentMonth < 1) {
                        currentMonth = 12;
                        currentYear--;
                    }
                    update();
                } else if (x >= rightBtnX && x <= rightBtnX + buttonWidth) {
                    // 点击右按钮：下一个月
                    currentMonth++;
                    if (currentMonth > 12) {
                        currentMonth = 1;
                        currentYear++;
                    }
                    update();
                }
                return true;
            }
        }
        return false;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        // 如果当前年份是1970，每隔200毫秒重新获取当前日期
        if (currentYear == 1970) {
            time_t now = time(NULL);
            struct tm time_info;
            localtime_r(&now, &time_info);

            int newYear = time_info.tm_year + 1900;
            int newMonth = time_info.tm_mon + 1;
            int newDay = time_info.tm_mday;

            // 如果日期有变化，更新显示
            if (newYear != currentYear || newMonth != currentMonth || newDay != currentDay) {
                currentYear = newYear;
                currentMonth = newMonth;
                currentDay = newDay;
                todayYear = newYear;
                todayMonth = newMonth;
                todayDay = newDay;
                update();
            }
        }
    }

public:
    CalendarWidget() : Widget() {
        // 获取当前日期
        time_t now = time(NULL);
        struct tm time_info;
        localtime_r(&now, &time_info);

        todayYear = currentYear = time_info.tm_year + 1900;
        todayMonth = currentMonth = time_info.tm_mon + 1;
        todayDay = currentDay = time_info.tm_mday;
    }
};

int main(int argc, char** argv) {
    X x;
    SpriteWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);
    
    CalendarWidget* calendar = new CalendarWidget();
    root->add(calendar);
    
    grect_t wr;
    wr.x = 200;
    wr.y = 100;
    wr.w = 320;
    wr.h = 300;
    
    win.open(&x, -1, wr, "Calendar", XWIN_STYLE_SPRITE);
    win.setTimer(2);
    widgetXRun(&x, &win);
    return 0;
}
