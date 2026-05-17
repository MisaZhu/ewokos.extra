#ifndef WIDGET_VIDEO_HH
#define WIDGET_VIDEO_HH

#include <Widget/Widget.h>
#include <pthread.h>

namespace Ewok {

class WidgetVideo : public Widget {
	string sourceFile;
	string statusText;

	pthread_t decodeThread;
	pthread_mutex_t stateMutex;
	pthread_mutex_t renderMutex;

	graph_t* frameGraph;

	bool autoPlay;
	bool loopPlay;
	bool muteAudio;
	bool playing;
	bool paused;
	bool stopRequested;
	bool threadRunning;
	bool eof;
	uint32_t currentMs;
	uint32_t totalMs;

protected:
	void onAdd();
	bool onMouse(xevent_t* ev);
	bool onIM(xevent_t* ev);
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);
	void setAttr(const string& attr, json_var_t* value);

public:
	WidgetVideo(const string& file = "");
	virtual ~WidgetVideo(void);

	bool loadVideo(const string& file);
	void play();
	void pause();
	void stop();

	void setAutoPlay(bool autoplay);
	void setLoop(bool loop);
	void setMute(bool mute);

	bool isPlaying();
	bool isPaused();
	bool isReady();
	bool isEOF();
	uint32_t getCurrentMs();
	uint32_t getTotalMs();

	gsize_t getMinSize(void);

	bool isStopRequested(void);
	bool isPausedState(void);
	bool isMutedState(void);
	bool isLoopState(void);
	void setPlaybackState(bool playing, bool paused, bool eof);

private:
	static void* decodeThreadEntry(void* p);

	void decodeLoop();
	void stopDecodeThread();
	void updateStatus(const string& text);
	string resolveSourceFile(void);
};

}

#endif
