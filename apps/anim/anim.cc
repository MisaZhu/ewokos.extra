#include <Widget/Sprite.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <graph/graph_png.h>
#include <openlibm.h>
#include <ewoksys/vdevice.h>
#include <time.h>

using namespace Ewok;

class AnimWidget : public Widget {
	int fighter_step;
	graph_t* img_fighter;

	void drawFitgher(graph_t* g) {
		graph_t* img = img_fighter;
		if(img == NULL)
			return;
		graph_blt(img, fighter_step*(img->w/7), 0, img->w/7, img->h,
				g, g->w-img->w/7, g->h-img->h, img->w, img->h);
	}
public:
	inline AnimWidget() {
		fighter_step = 0;
		img_fighter = png_image_new(X::getResName("data/fighter.png").c_str());
	}
	
	inline ~AnimWidget() {
		if(img_fighter != NULL)
			graph_free(img_fighter);
	}

	bool getSize(int32_t& w, int32_t& h) {
		if(img_fighter == NULL)
			return false;
		w = img_fighter->w / 7;
		h = img_fighter->h;
		return true;
	}

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_clear(g, 0x0);
		drawFitgher(g);

		fighter_step++;
		if (fighter_step >= 7)
			fighter_step = 0;
	}

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        update();
    }
};

int main(int argc, char** argv) {
    X x;
    Sprite win;

    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);

    AnimWidget* anim = new AnimWidget();
    root->add(anim);

    int32_t w=120, h=120;
    anim->getSize(w, h);
    win.open(&x, -1, -1, -1, w, h, "Anim", XWIN_STYLE_NO_FRAME);
    win.setTimer(7);
    win.setAlpha(true);
    widgetXRun(&x, &win);
    return 0;
}