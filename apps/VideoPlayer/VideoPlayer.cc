#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Container.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/Menubar.h>
#include <WidgetVideo/WidgetVideo.h>

#include <x++/X.h>

#include <stdio.h>
#include <string.h>

using namespace Ewok;

class ProgressBar : public Widget {
public:
	typedef void (*SeekCallback)(void* userData, float progress);

	ProgressBar() {
		progress = 0.0f;
		seekCb = NULL;
		seekUserData = NULL;
		dragging = false;
	}

	void setProgress(float p) {
		if(p < 0.0f)
			p = 0.0f;
		if(p > 1.0f)
			p = 1.0f;
		progress = p;
		update();
	}

	void setSeekCallback(SeekCallback cb, void* ud) {
		seekCb = cb;
		seekUserData = ud;
	}

	bool isDragging(void) const {
		return dragging;
	}

protected:
	float progress;
	SeekCallback seekCb;
	void* seekUserData;
	bool dragging;

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
		int fillWidth;
		(void)theme;

		graph_fill_rect(g, rect.x, rect.y, rect.w, rect.h, 0xFF333344);
		fillWidth = (int)(rect.w * progress);
		if(fillWidth > 0) {
			graph_fill_rect(g, rect.x, rect.y, fillWidth, rect.h, 0xFF00AAFF);
		}

		graph_line(g, rect.x, rect.y, rect.x + rect.w - 1, rect.y, 0xFF555566);
		graph_line(g, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1, 0xFF555566);
		graph_line(g, rect.x + rect.w - 1, rect.y + rect.h - 1, rect.x, rect.y + rect.h - 1, 0xFF555566);
		graph_line(g, rect.x, rect.y + rect.h - 1, rect.x, rect.y, 0xFF555566);
	}

	bool onMouse(xevent_t* ev) {
		grect_t r = area;
		int mx;

		if(ev->type != XEVT_MOUSE)
			return false;

		gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);

		mx = pos.x;
		if(mx < 0 || mx >= r.w)
			return false;

		if(ev->state == MOUSE_STATE_DOWN) {
			dragging = true;
			updateProgressFromMouse(mx, r);
			return true;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			if(dragging) {
				updateProgressFromMouse(mx, r);
				return true;
			}
		}
		else if(ev->state == MOUSE_STATE_UP) {
			if(dragging) {
				dragging = false;
				updateProgressFromMouse(mx, r);
				return true;
			}
		}
		return false;
	}

	void updateProgressFromMouse(int mx, const grect_t& r) {
		float newProgress = (float)(mx - r.x) / (float)r.w;

		if(newProgress < 0.0f)
			newProgress = 0.0f;
		if(newProgress > 1.0f)
			newProgress = 1.0f;
		progress = newProgress;
		update();
		if(seekCb != NULL)
			seekCb(seekUserData, progress);
	}
};

class VideoPlayerWin : public WidgetWin {
	FileDialog fdialog;
	WidgetVideo* video;
	LabelButton* playBtn;
	Label* timeLabel;
	ProgressBar* progressBar;
	LabelButton* muteBtn;
	LabelButton* loopBtn;
	string loadedFile;

protected:
	void onDialoged(XWin* from, int res, void* arg) {
		(void)arg;
		if(from != &fdialog || res != Dialog::RES_OK)
			return;

		string path = fdialog.getResult();
		if(!path.empty())
			loadVideo(path.c_str());
	}

public:
	VideoPlayerWin(void) {
		video = NULL;
		playBtn = NULL;
		timeLabel = NULL;
		progressBar = NULL;
		muteBtn = NULL;
		loopBtn = NULL;
	}

	void setVideo(WidgetVideo* v) {
		video = v;
	}

	void setPlayBtn(LabelButton* btn) {
		playBtn = btn;
	}

	void setTimeLabel(Label* label) {
		timeLabel = label;
	}

	void setProgressBar(ProgressBar* bar) {
		progressBar = bar;
	}

	void seekToProgress(float progress) {
		if(video == NULL)
			return;
		video->seekToProgress(progress);
		updateUi();
	}

	void setMuteBtn(LabelButton* btn) {
		muteBtn = btn;
	}

	void setLoopBtn(LabelButton* btn) {
		loopBtn = btn;
	}

	FileDialog* getFileDialog(void) {
		return &fdialog;
	}

	void loadVideo(const char* path) {
		if(video == NULL || path == NULL || path[0] == 0)
			return;

		loadedFile = path;
		video->loadVideo(path);
		updateUi();
	}

	void togglePlay(void) {
		if(video == NULL || loadedFile.empty())
			return;

		if(video->isPlaying())
			video->pause();
		else
			video->play();
		updateUi();
	}

	void stopVideo(void) {
		if(video == NULL)
			return;
		video->stop();
		updateUi();
	}

	void toggleMute(void) {
		if(video == NULL)
			return;
		video->setMute(!video->isMutedState());
		updateUi();
	}

	void toggleLoop(void) {
		if(video == NULL)
			return;
		video->setLoop(!video->isLoopState());
		updateUi();
	}

	void updateUi(void) {
		uint32_t cur;
		uint32_t tot;
		char buf[64];

		if(video == NULL)
			return;

		if(playBtn != NULL)
			playBtn->setLabel(video->isPlaying() ? "||" : ">");

		if(muteBtn != NULL)
			muteBtn->setLabel(video->isMutedState() ? "Muted" : "Sound");

		if(loopBtn != NULL)
			loopBtn->setLabel(video->isLoopState() ? "Loop*" : "Loop");

		cur = video->getCurrentMs();
		tot = video->getTotalMs();

		if(progressBar != NULL) {
			if(!progressBar->isDragging() && tot > 0)
				progressBar->setProgress((float)cur / (float)tot);
			else if(!progressBar->isDragging())
				progressBar->setProgress(0.0f);
		}

		if(timeLabel != NULL) {
			snprintf(buf, sizeof(buf) - 1, "%02d:%02d / %02d:%02d",
					cur / 60000, (cur / 1000) % 60,
					tot / 60000, (tot / 1000) % 60);
			timeLabel->setLabel(buf);
		}
	}

	void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
		(void)timerFPS;
		(void)timerSteps;
		updateUi();
	}
};

static void onOpen(MenuItem* item, void* arg) {
	(void)item;
	VideoPlayerWin* win = (VideoPlayerWin*)arg;
	win->getFileDialog()->popup(win, 0, 0, "files", XWIN_STYLE_NORMAL);
}

static void onPlayClick(Widget* wd, xevent_t* evt, void* arg) {
	(void)wd;
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	((VideoPlayerWin*)arg)->togglePlay();
}

static void onStopClick(Widget* wd, xevent_t* evt, void* arg) {
	(void)wd;
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	((VideoPlayerWin*)arg)->stopVideo();
}

static void onMuteClick(Widget* wd, xevent_t* evt, void* arg) {
	(void)wd;
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	((VideoPlayerWin*)arg)->toggleMute();
}

static void onLoopClick(Widget* wd, xevent_t* evt, void* arg) {
	(void)wd;
	if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
	((VideoPlayerWin*)arg)->toggleLoop();
}

static void onSeekProgress(void* userData, float progress) {
	((VideoPlayerWin*)userData)->seekToProgress(progress);
}

int main(int argc, char** argv) {
	X x;
	VideoPlayerWin win;

	RootWidget* root = new RootWidget();
	root->setType(Container::VERTICAL);
	win.setRoot(root);

	Menubar* menubar = new Menubar();
	menubar->fix(0, 24);
	menubar->setItemSize(50);
	menubar->add(0, "Open", NULL, NULL, onOpen, &win);
	root->add(menubar);

	WidgetVideo* video = new WidgetVideo();
	video->setAutoPlay(true);
	root->add(video);
	win.setVideo(video);

	ProgressBar* progressBar = new ProgressBar();
	progressBar->fix(0, 16);
	progressBar->setSeekCallback(onSeekProgress, &win);
	root->add(progressBar);
	win.setProgressBar(progressBar);

	Container* controls = new Container();
	controls->setType(Container::HORIZONTAL);
	controls->fix(0, 24);
	root->add(controls);

	LabelButton* playBtn = new LabelButton(">");
	playBtn->fix(40, 0);
	playBtn->setEventFunc(onPlayClick, &win);
	controls->add(playBtn);
	win.setPlayBtn(playBtn);

	Label* timeLabel = new Label("00:00 / 00:00");
	timeLabel->fix(140, 0);
	controls->add(timeLabel);
	win.setTimeLabel(timeLabel);

	LabelButton* stopBtn = new LabelButton("[]");
	stopBtn->fix(40, 0);
	stopBtn->setEventFunc(onStopClick, &win);
	controls->add(stopBtn);

	LabelButton* muteBtn = new LabelButton("Sound");
	muteBtn->fix(64, 0);
	muteBtn->setEventFunc(onMuteClick, &win);
	controls->add(muteBtn);
	win.setMuteBtn(muteBtn);

	LabelButton* loopBtn = new LabelButton("Loop");
	loopBtn->fix(64, 0);
	loopBtn->setEventFunc(onLoopClick, &win);
	controls->add(loopBtn);
	win.setLoopBtn(loopBtn);

	win.open(&x, -1, -1, -1, 420, 300, "VideoPlayer",
			XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT, true);
	/*
	 * Widget repaint is timer-driven; use a higher timer so video present
	 * granularity is closer to mp4player's direct render loop.
	 */
	win.setTimer(120);

	if(argc >= 2)
		win.loadVideo(argv[1]);
	win.updateUi();

	widgetXRun(&x, &win);
	return 0;
}
