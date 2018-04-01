#include<opencv2\opencv.hpp>	
#include"CaptureManager.hpp"
#include<vlc\vlc.h>
using namespace std;
using namespace cv;

int main() {
	CaptureManager video;

	// 空字符串代表打开任意摄像头，要筛选的话直接把设备管理器的摄像头名字打进去，设置好正确的分辨率即可，帧率默认30，有的摄像头可能是25
	video.CaptureDirectShow("", Size(640, 480), 30, 0);	
	
	// 屏幕捕捉，类似OBS那种，可以用这个功能做自动玩游戏
	//video.CaptureScreen();
	
	// 打开rtsp流，缓冲时间需要调试，一般720p 300ms 1080p 500ms 更低的直接100ms即可，时间过小会导致帧不完整无法解码
	// 一般情况下高分辨率20ms以内容易翻车，任何分辨率0ms必翻
	// 缓冲时间是一个时间窗口，在这个时间内必须要有足够多的数据描述一个帧才行，超时的就会丢掉
	//video.CaptureStream("rtsp://:8554/1",500);
	video.CaptureStream("http://itv.hdpfans.com/live?vid=43", 20);
	
	// 注：
	// CaptureStream可以打开文件(用的是VLC的MRL，能打开的多了去了，甚至DVD啥的)，但还不是逐帧的，是抓取最新的
	// CaptureFile还没做，opencv自己的就能用，只不过解码器垃圾（没硬件加速、格式支持少）罢了
	
	// 启动解码并初始化第一帧的格式和内容，可以有效避免OpenCV崩掉	|	两种方法，适用不同情况;
	Mat frame = video.Start();										// 	Mat frame; video.Start(frame);	

	for (int frame_dropped = 0; waitKey(1) != 27; frame_dropped = video >> frame) {
		if (frame_dropped > 0)	cout << "[Performace] " << frame_dropped << " frame(s) dropped" << endl;
		//pyrDown(frame, frame);
		imshow("frame", frame);
	}
	
	return 0;
}

// 我感觉这样比VideoCapture用起来简洁一些
// 特点：
// 可依赖，无论媒体多不靠谱都能恢复过来
// 自动启动硬件加速，查了半天又试了半天终于可以用了
// 默认摄像头0缓冲，缺点是如果【场景过暗】导致摄像头帧率低于标称的话会卡，因为帧数就不同步，让摄像头回到标称帧率那么一秒就同步了，之后低于标称帧率就不卡
// 