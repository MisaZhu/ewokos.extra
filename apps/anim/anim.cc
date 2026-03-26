#include <Widget/SpriteWin.h>
#include <Widget/SpriteAnim.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <graph/graph_png.h>
#include <openlibm.h>
#include <ewoksys/vdevice.h>
#include <time.h>

using namespace Ewok;

int main(int argc, char** argv) {
    X x;
    SpriteWin win;

    RootWidget* root = new RootWidget();
	root->setAlpha(true);
    root->setType(Container::HORIZONTAL);
    win.setRoot(root);

    SpriteAnim* anim = new SpriteAnim();
    root->add(anim);

	std::string script = X::getResName("data/fighter.spr").c_str();
	if(argc > 1)
		script = argv[1];

	if(!anim->setSpriteByScript(script))
		return 1;

    gsize_t size = anim->getSpriteSize();
    win.open(&x, -1, -1, -1, size.w, size.h, "Anim", XWIN_STYLE_SPRITE);	
    win.setTimer(anim->getFPS());
    win.setAlpha(true);
    widgetXRun(&x, &win);
    return 0;
}