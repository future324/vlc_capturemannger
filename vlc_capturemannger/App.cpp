#include<opencv2\opencv.hpp>	
#include"CaptureManager.hpp"
#include<vlc\vlc.h>
using namespace std;
using namespace cv;

int main() {
	CaptureManager video;

	// ���ַ����������������ͷ��Ҫɸѡ�Ļ�ֱ�Ӱ��豸������������ͷ���ִ��ȥ�����ú���ȷ�ķֱ��ʼ��ɣ�֡��Ĭ��30���е�����ͷ������25
	video.CaptureDirectShow("", Size(640, 480), 30, 0);	
	
	// ��Ļ��׽������OBS���֣�����������������Զ�����Ϸ
	//video.CaptureScreen();
	
	// ��rtsp��������ʱ����Ҫ���ԣ�һ��720p 300ms 1080p 500ms ���͵�ֱ��100ms���ɣ�ʱ���С�ᵼ��֡�������޷�����
	// һ������¸߷ֱ���20ms�������׷������κηֱ���0ms�ط�
	// ����ʱ����һ��ʱ�䴰�ڣ������ʱ���ڱ���Ҫ���㹻�����������һ��֡���У���ʱ�ľͻᶪ��
	//video.CaptureStream("rtsp://:8554/1",500);
	video.CaptureStream("http://itv.hdpfans.com/live?vid=43", 20);
	
	// ע��
	// CaptureStream���Դ��ļ�(�õ���VLC��MRL���ܴ򿪵Ķ���ȥ�ˣ�����DVDɶ��)������������֡�ģ���ץȡ���µ�
	// CaptureFile��û����opencv�Լ��ľ����ã�ֻ����������������ûӲ�����١���ʽ֧���٣�����
	
	// �������벢��ʼ����һ֡�ĸ�ʽ�����ݣ�������Ч����OpenCV����	|	���ַ��������ò�ͬ���;
	Mat frame = video.Start();										// 	Mat frame; video.Start(frame);	

	for (int frame_dropped = 0; waitKey(1) != 27; frame_dropped = video >> frame) {
		if (frame_dropped > 0)	cout << "[Performace] " << frame_dropped << " frame(s) dropped" << endl;
		//pyrDown(frame, frame);
		imshow("frame", frame);
	}
	
	return 0;
}

// �Ҹо�������VideoCapture���������һЩ
// �ص㣺
// ������������ý��಻���׶��ָܻ�����
// �Զ�����Ӳ�����٣����˰��������˰������ڿ�������
// Ĭ������ͷ0���壬ȱ���������������������������ͷ֡�ʵ��ڱ�ƵĻ��Ῠ����Ϊ֡���Ͳ�ͬ����������ͷ�ص����֡����ôһ���ͬ���ˣ�֮����ڱ��֡�ʾͲ���
// 