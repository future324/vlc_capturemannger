#pragma once
#include<opencv2\opencv.hpp>	
#include<vlc\vlc.h>
#include<windows.h>

using namespace std;
using namespace cv;

#define CaptureManager_debug_message_output 0

class CaptureManager
{
public:
	CaptureManager() {
		newInstance();
	}
	~CaptureManager() {
		libvlc_media_release(media);
		libvlc_media_player_release(media_player);
		libvlc_release(instance);
	}
	void CaptureDirectShow (string dshow_vdev, Size target_size, int target_fps = 30, int caching_ms = 0) {
		if (target_size == Size())  target_size = Size(640, 480);
		init_media("dshow://", caching_ms);
		set_dshow(dshow_vdev, target_size, target_fps);
		parse_media();
		grab_frame_method = grab_stream;
	}
	void CaptureScreen(int target_fps = 30) {
		init_media("screen://", 0);
		libvlc_media_add_option(media, (string(":screen-fps=") + to_string(target_fps)).c_str());
		parse_media();
		grab_frame_method = grab_stream;
	}
	void CaptureFile(string mrl, bool byframe = true, int loop_mode = 0) {
		init_network_media("screen://", 0);
		// 还没做，暂时没意义，opencv自己的先用着
	}
	void CaptureStream(string mrl, int caching_ms = 500,int buffer_ms = 30) {
		init_network_media(mrl, caching_ms, buffer_ms);
		parse_media();
		grab_frame_method = grab_stream;
	}
	void Start(Mat &init_frame) {
		libvlc_media_player_play(media_player);
		while (init_frame.empty()) {
			while (new_frame_cnt == 0) {
				Sleep(1);
				title = string("[") + states(State()) + string("(init)]");
				system((string("title ") + title).c_str());
				if (_state > Paused) { 
					libvlc_media_player_stop(media_player); 
					break;
				}
				if (_state < Playing) {
					libvlc_media_player_play(media_player);
					break;
				}
			}
			new_frame_cnt = 0;
			init_frame = _frame;
		}
		cout << "streaming：" << endl;
	}
	Mat Start() {
		Mat iframe;
		Start(iframe);
		return iframe;
	}
	void Pause() {
		libvlc_media_player_pause(media_player);
	}
	void Stop() {
		libvlc_media_player_stop(media_player);
	}
	typedef enum state
	{
		Idle = 0,
		Opening,
		Buffering,
		Playing,
		Paused,
		Stopped,
		Ended,
		Error
	} state;
	state State() {
		_state = libvlc_media_player_get_state(media_player);
		return (state)_state;
	}
	void Reset() {
		libvlc_media_release(media);
		libvlc_media_player_release(media_player);
		libvlc_release(instance);
		newInstance();
	}
	float prop_fps() {
		return video_prop.fps;
	}
	// 用于根据不同媒体类型动态绑定不同的帧更新方法：逐帧、最新帧等
	// 现在只写了最新帧的
	int (*grab_frame_method)(Mat &i, void* pinstance);

	/* size_t operator >>(Mat&)  ret<0 帧尚未更新 | ret=0 正常 | ret>0 n帧被跳过*/
	int operator >> (Mat &i){
		LARGE_INTEGER t,freq;
		QueryPerformanceCounter(&t);
		// 解码速率更新
		libvlc_media_get_stats(media, &stats);
		// 解码状态更新
		State();
		// 抓帧函数由不同方法来绑定
		int ret = (grab_frame_method)(i, this);
		if (ret >= 0) {
			QueryPerformanceFrequency(&freq);
			auto k_byte_rate = stats.f_demux_bitrate * 10000 / 8;
			auto mbyte = k_byte_rate > 1000;
			title = string("[") + states((state)_state) + string("] ") +
				string("FPS:") + to_string((float)freq.QuadPart / (float)(t.QuadPart - t_last.QuadPart)) +
				//string(" @ ") + to_string(stats.f_demux_bitrate * 10000) + string("Kbps")+
				string(" @ ") + to_string(mbyte ? k_byte_rate / 1000 : k_byte_rate) + string(mbyte ? " MByte/s" : " KByte/s");
			t_last = t;
		}
		system((string("title ") + title).c_str());
		return ret;
	}
	
private:
	libvlc_instance_t *instance;
	libvlc_media_t  *media;
	libvlc_media_player_t *media_player;
	
	unsigned int n_tracks = 0;
	libvlc_media_track_t **tracks;
	libvlc_media_stats_t stats;
	size_t new_frame_cnt = 0;

	struct{
		Size size;
		float fps = 0;
	}video_prop;
	Mat _frame;
	uchar* buffer;

	string title;
	int _state = 0;
	LARGE_INTEGER t_last;
private:
	string states(state statex) {
		switch (statex)
		{
		case CaptureManager::Idle:
			return "Idle";
		case CaptureManager::Opening:
			return "Opening";
		case CaptureManager::Buffering:
			return "Buffering";
		case CaptureManager::Playing:
			return "Playing";
		case CaptureManager::Paused:
			return "Paused";
		case CaptureManager::Stopped:
			return "Stopped";
		case CaptureManager::Ended:
			return "Ended";
		case CaptureManager::Error:
			return "Error";
		default:
			return "Unknown";
		}
	}
	inline static int grab_stream(Mat &i,void* pinstance) {
		auto instance = (CaptureManager*)pinstance; // 静态函数访问动态实例成员内容
		LARGE_INTEGER t0,t1, freq;
		QueryPerformanceCounter(&t0);
		while (instance->new_frame_cnt == 0) {
			Sleep(1);
			QueryPerformanceCounter(&t1);
			QueryPerformanceFrequency(&freq);
			auto timeout = (t1.QuadPart - t0.QuadPart) / freq.QuadPart;
			if (timeout) break;
			if (instance->_state < Playing) {
				instance->title = "[Buffering...]";
				//libvlc_media_player_play(instance->media_player);
				break;
			}
			if (instance->_state > Paused) {
				//cout << "player has reset.";
				libvlc_media_player_stop(instance->media_player);
				libvlc_media_player_release(instance->media_player);
				instance->media_player = libvlc_media_player_new_from_media(instance->media);
				instance->init_player();
				libvlc_media_player_play(instance->media_player);
				break;
			}
		}
		int dropped = instance->new_frame_cnt - 1;
		instance->new_frame_cnt = 0;
		i.data = instance->buffer;
		return dropped;
	}
	inline static size_t grab_next_frame(Mat &i, void* pinstance) {
		// 没测试，可以看完代码试一下，不行了查文档
		auto instance = (CaptureManager*)pinstance;
		libvlc_media_player_pause(instance->media_player);
		libvlc_media_player_next_frame(instance->media_player);
	}
	inline static void *lock(void *opaque, void**p_pixels) {
		//CaptureManager* instance = (CaptureManager*)opaque;
		//instance->locked = true;
		*p_pixels = ((CaptureManager*)opaque)->buffer;
		return NULL;
	}
	inline static void unlock(void *opaque, void *id, void *const *p_pixels) {
		//CaptureManager* instance = (CaptureManager*)opaque;
		//instance->locked = false;
		((CaptureManager*)opaque)->new_frame_cnt += 1;
	}
	inline static void ready(void *opaque, void *picture) {
		//CaptureManager* instance = (CaptureManager*)opaque;

	}
	void newInstance() {
		char const* vlc_args[] = {
			//"--intf=dummy",
			"--ignore-config",
			"--sout-display-delay=0",
			"--rtsp-frame-buffer-size=327680",
			"--h264-fps=60",

#if CaptureManager_debug_message_output
			"-vvv",// debug message
#else
			"--quiet",
#endif // CaptureManager_debug_message_output
		};
		instance = libvlc_new(sizeof(vlc_args) / sizeof(*vlc_args), vlc_args);
		media_player = libvlc_media_player_new(instance);
		buffer = NULL;
	}
	void set_dshow(string vdev, Size size, int target_fps = 30) {
		video_prop.size = size;
		libvlc_media_add_option(media, (string(":dshow-vdev=") + vdev).c_str());
		libvlc_media_add_option(media, ":dshow-adev=None");
		libvlc_media_add_option(media, (string(":dshow-size=") + to_string(video_prop.size.width) + string("x") + to_string(video_prop.size.height)).c_str());
		libvlc_media_add_option(media, (string(":dshow-fps=") + to_string(target_fps)).c_str());
		cout << "[DirectShow] " << (vdev.empty() ? "Default" : vdev) <<" ";
	}
	void init_media_options_common() {
		libvlc_media_add_option(media, (string(":avcodec-hw=dxva2").c_str()));
		//libvlc_media_add_option(media, (string(":avcodec-fast").c_str()));
		//libvlc_media_add_option(media, (string(":avcodec-hurry-up").c_str()));
		libvlc_media_add_option(media, (string(":avcodec-dr").c_str()));
		libvlc_media_add_option(media, (string(":sout-display-delay=0").c_str()));
	}
	void init_media(string mrl,int caching_ms = 0) {
		media = libvlc_media_new_location(instance, mrl.c_str());
		libvlc_media_add_option(media, (string(":live-caching=") + to_string(caching_ms)).c_str());	
		init_media_options_common();
	}
	void init_network_media(string mrl, int caching_ms = 0, int buffer_ms = 0) {
		media = libvlc_media_new_location(instance, mrl.c_str());
		libvlc_media_add_option(media, (string(":live")).c_str());
		libvlc_media_add_option(media, (string(":smooth")).c_str());
		libvlc_media_add_option(media, (string(":network-caching=") + to_string(caching_ms)).c_str());
		libvlc_media_add_option(media, (string(":live-caching=") + to_string(buffer_ms)).c_str());
		init_media_options_common();
	}
	void parse_media() {
		cout << "find media... ";		
		// create media player instance
		libvlc_media_player_set_media(media_player, media);
		libvlc_video_set_callbacks(media_player, NULL, NULL, NULL, NULL);
		// parse media 
		libvlc_media_parse(media);
		libvlc_media_player_play(media_player);

		do{
			string title = (string("[Parsing: ") + libvlc_media_get_mrl(media) + "]");
			n_tracks = libvlc_media_tracks_get(media, &tracks);
			for (size_t i = 0; i < n_tracks; i++) {
				if (tracks[i] && tracks[i]->i_type == libvlc_track_video){
					video_prop.size = Size(tracks[i]->video->i_width, tracks[i]->video->i_height);
					video_prop.fps = (float)tracks[i]->video->i_frame_rate_num / (float)tracks[i]->video->i_frame_rate_den;
				}
			}
			Sleep(1);
			if (State() > Paused) {
				libvlc_media_player_stop(media_player);
				libvlc_media_player_play(media_player);
			}
			system((string("title ") + title).c_str());
		} while (video_prop.size == Size(0,0));
		libvlc_media_player_stop(media_player);

		cout << video_prop.size << "@" << video_prop.fps << "FPS" << endl;
		
		init_player();
	}
	void *aligned_malloc(size_t size, int aligned)
	{
		assert((aligned&(aligned - 1)) == 0);
		void *data = malloc(sizeof(void *) + aligned + size); 
		void **temp = (void **)data + 1;
		void **alignedData = (void **)(((size_t)temp + aligned - 1)&-aligned);
		alignedData[-1] = data;
		return alignedData;
	}
	void init_player() {
		libvlc_media_parse(media);
		libvlc_media_player_release(media_player);
		media_player = libvlc_media_player_new(instance);
		libvlc_media_player_set_media(media_player, media);
		// allocate buffer
		if(buffer) free(((void **)buffer)[-1]);
		buffer = (uchar*)aligned_malloc(video_prop.size.width * video_prop.size.height * 3, 64);
		_frame = Mat(video_prop.size, CV_8UC3, buffer);
		// init
		libvlc_video_set_format(media_player, "RV24", video_prop.size.width, video_prop.size.height, video_prop.size.width * 3);
		libvlc_video_set_callbacks(media_player, lock, unlock, ready, this);
	}
	
};
