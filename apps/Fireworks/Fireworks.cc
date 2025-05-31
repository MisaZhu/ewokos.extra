#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph_png.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/basic_math.h>
#include <openlibm.h>

using namespace Ewok;

// 烟花粒子结构体
struct Particle {
    int x, y;
    int vx, vy;
    uint32_t color;
    int lifetime;

    // 添加默认构造函数
    Particle() : x(0), y(0), vx(0), vy(0), color(0), lifetime(0) {}

    Particle(int _x, int _y, int _vx, int _vy, uint32_t _color, int _lifetime)
        : x(_x), y(_y), vx(_vx), vy(_vy), color(_color), lifetime(_lifetime) {}
};

// 烟花结构体
struct Firework {
    int x, y;
    int vy;
    uint32_t color;
    bool exploded;

    // 添加默认构造函数
    Firework() : x(0), y(0), vy(0), color(0), exploded(false) {}

    Firework(int _x, int _y, int _vy, uint32_t _color)
        : x(_x), y(_y), vy(_vy), color(_color), exploded(false) {}
};

// 定义最大烟花和粒子数量
const int MAX_FIREWORKS = 16;
const int MAX_PARTICLES = 512;

// 自定义静态数组模板类
template <typename T, int MaxSize>
class StaticArray {
private:
    T* data;
    int size;

public:
    StaticArray() : size(0) {
        data =  (T*)malloc(MaxSize*sizeof(T));
    }
    ~StaticArray() {
        free(data);
    }

    void push_back(const T& value) {
        if (size < MaxSize) {
            data[size++] = value;
        }
    }

    T& operator[](int index) {
        return data[index];
    }

    const T& operator[](int index) const {
        return data[index];
    }

    int getSize() const {
        return size;
    }

    void erase(int index) {
        for (int i = index; i < size - 1; ++i) {
            data[i] = data[i + 1];
        }
        --size;
    }
};

// 自定义 min 函数
template<typename T>
T myMin(T a, T b) {
    return a < b ? a : b;
}

class FireworksWidget : public Widget {
    StaticArray<Firework, MAX_FIREWORKS> fireworks;
    StaticArray<Particle, MAX_PARTICLES> particles;

    // 生成随机颜色
    uint32_t randomColor() {
        return 0xff000000 | (rand() % 256) << 16 | (rand() % 256) << 8 | (rand() % 256);
    }

    // 发射新烟花
    void launchFirework() {
        int x = rand() % area.w;
        int vy = -(rand() % 10 + 10);
        uint32_t color = randomColor();
        fireworks.push_back(Firework(x, area.h, vy, color));
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 清空背景
        graph_fill(g, r.x, r.y, r.w, r.h, 0xff000000);
    
        // 更新并绘制烟花
        for (int i = 0; i < fireworks.getSize();) {
            fireworks[i].y += fireworks[i].vy;
            if (fireworks[i].y < (area.h / 3) || rand() % 100 == 0) {
                // 烟花爆炸
                fireworks[i].exploded = true;
                for (int j = 0; j < 100; ++j) {
                    int angle = rand() % 360;
                    int speed = rand() % 5 + 1;
                    int vx = speed * cos(angle * 3.1415926 / 180);
                    int vy = speed * sin(angle * 3.1415926 / 180);
                    // 爆炸粒子使用随机颜色
                    particles.push_back(Particle(fireworks[i].x, fireworks[i].y, vx, vy, randomColor(), rand() % 40 + 10));
                }
                fireworks.erase(i);
            } else {
                // 增加上升粒子大小
                uint32_t brighterColor = adjustBrightness(fireworks[i].color, 1.5f);
                int radius = 2; // 增大粒子半径
                graph_fill_circle(g, fireworks[i].x, fireworks[i].y, radius, brighterColor);
                
                // 添加尾巴轨迹
                for (int t = 1; t <= 3; t++) {
                    float alpha = 1.0f - t * 0.3f;
                    uint32_t tailColor = adjustAlpha(brighterColor, alpha);
                    graph_fill_circle(g, 
                        fireworks[i].x, 
                        fireworks[i].y + t*radius*3, 
                        radius - t, tailColor);
                }
                ++i;
            }
        }
    
        // 更新并绘制粒子
        for (int i = 0; i < particles.getSize();) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].lifetime--;
            if (particles[i].lifetime <= 0) {
                particles.erase(i);
            } else {
                // 保留原有大小和渐变效果
                float alpha = (float)particles[i].lifetime / 30.0f;
                uint32_t fadedColor = adjustAlpha(particles[i].color, alpha);
                // 绘制圆形粒子
                int radius = 3;
                graph_fill_circle(g, particles[i].x, particles[i].y, radius, fadedColor);
                
                // 添加尾巴轨迹
                for (int t = 1; t <= 2; t++) {
                    float tailAlpha = alpha * (1.0f - t * 0.3f);
                    uint32_t tailColor = adjustAlpha(particles[i].color, tailAlpha);
                    graph_fill_circle(g, 
                        particles[i].x - particles[i].vx * t,
                        particles[i].y - particles[i].vy * t,
                        radius - t, tailColor);
                }
                ++i;
            }
        }
    }

    // 辅助函数：调整颜色亮度
    uint32_t adjustBrightness(uint32_t color, float factor) {
        uint8_t r = (color >> 16) & 0xff;
        uint8_t g = (color >> 8) & 0xff;
        uint8_t b = color & 0xff;

        r = static_cast<uint8_t>(myMin(255, static_cast<int>(r * factor)));
        g = static_cast<uint8_t>(myMin(255, static_cast<int>(g * factor)));
        b = static_cast<uint8_t>(myMin(255, static_cast<int>(b * factor)));

        return 0xff000000 | (r << 16) | (g << 8) | b;
    }

    // 辅助函数：调整颜色透明度
    uint32_t adjustAlpha(uint32_t color, float alpha) {
        uint8_t r = (color >> 16) & 0xff;
        uint8_t g = (color >> 8) & 0xff;
        uint8_t b = color & 0xff;
        uint8_t a = static_cast<uint8_t>(alpha * 255);

        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        if (timerStep % (timerFPS/1) == 0) {
            launchFirework();
        }
        update();
    }

public:
    FireworksWidget() {}
    ~FireworksWidget() {}
};

int main(int argc, char** argv) {
    X x;
    WidgetWin win;
    RootWidget *root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);
    root->setAlpha(false);

    FireworksWidget* fireworks = new FireworksWidget();
    root->add(fireworks);

    win.open(&x, 0, -1, -1, 0, 0, "Fireworks", XWIN_STYLE_NORMAL);
    win.setTimer(30);

    widgetXRun(&x, &win);
    return 0;
}