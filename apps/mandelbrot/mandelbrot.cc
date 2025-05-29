#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph_png.h>
#include <ewoksys/ewokdef.h>
#include <openlibm.h>

using namespace Ewok;

class Mandelbrot : public Widget {
public:
    inline Mandelbrot() {
    }
    
    inline ~Mandelbrot() {
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 定义复平面范围
        const double xMin = -2.0;
        const double xMax = 1.0;
        const double yMin = -1.5;
        const double yMax = 1.5;
        const int maxIterations = 30;

        for (int y = r.y; y < r.y + r.h; y++) {
            for (int x = r.x; x < r.x + r.w; x++) {
                // 将屏幕坐标转换为复平面坐标
                double cRe = xMin + (x - r.x) * (xMax - xMin) / r.w;
                double cIm = yMin + (y - r.y) * (yMax - yMin) / r.h;

                double zRe = 0.0;
                double zIm = 0.0;
                int iteration = 0;

                // 迭代计算
                while (zRe * zRe + zIm * zIm <= 4.0 && iteration < maxIterations) {
                    double temp = zRe * zRe - zIm * zIm + cRe;
                    zIm = 2.0 * zRe * zIm + cIm;
                    zRe = temp;
                    iteration++;
                }

                // 根据迭代次数设置颜色
                uint8_t colorValue = (iteration == maxIterations) ? 0 : (255 * iteration / maxIterations);
                //uint32_t color = 0xff000000|(colorValue << 16) | (colorValue << 8) | colorValue;
                uint32_t color = 0xff000000|(colorValue << 16) | (0 << 8) | colorValue;
                graph_pixel(g, x, y, color);
            }
        }
    }
};


int main(int argc, char** argv) {
    X x;
    WidgetWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);
    root->setAlpha(false);

    Mandelbrot* mandelbrot = new Mandelbrot();
    root->add(mandelbrot);

    win.open(&x, 0, -1, -1, 300, 200, "Mandelbrot Set", XWIN_STYLE_NORMAL);
    widgetXRun(&x, &win);    
    return 0;
}