#include<string.h>
#include"al.h"
#include"alc.h"
#include<iostream>
#include<conio.h>
#include<windows.h>
#include<list>

using std::cout;
using std::endl;

void PrintALInfo()
{
	cout << "Vendor:" << AL_VENDOR << endl;
	cout << "Version:" << AL_VERSION << endl;
	cout << "Renderer:" << AL_RENDERER << endl;
	cout << "Extension:" << AL_EXTENSIONS << endl;
}

class PlayBack {
public:
	enum { MAX_CACHE = 32 };
	bool initSuccess = true;
	PlayBack() {
		m_source = 0;
		m_device = alcOpenDevice(nullptr);
		if (m_device == nullptr) {
			initSuccess = false;
			cout << "create device fail" << endl;
			return;
		}
		m_context = alcCreateContext(m_device, nullptr);
		if (m_context == nullptr) {
			initSuccess = false;
			cout << "create context fail" << endl;
			return;
		}
		alcMakeContextCurrent(m_context);

		if (checkALError()) {
			initSuccess = false;
			return;
		}
		alGenSources(1, &m_source);
		alGenBuffers(MAX_CACHE, m_buffer);
		if (checkALError()) {
			initSuccess = false;
			return;
		}
		for (auto buf : m_buffer) {
			m_bufferQueue.push_back(buf);
		}
	}

	void clear() {
		if (m_context != nullptr) {
			alDeleteSources(1, &m_source);
			m_source = 0;
			alDeleteBuffers(MAX_CACHE, m_buffer);
			memset(m_buffer, 0, sizeof(m_buffer));
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(m_context);
			m_context = nullptr;
			alcCloseDevice(m_device);
			m_device = nullptr;
			m_bufferQueue.clear();
		}
	}

	void play(const void *data, int datasize, int freq, ALenum format) {
		if (data != nullptr && !m_bufferQueue.empty()) {
			ALuint buffer = m_bufferQueue.front();
			m_bufferQueue.pop_front();
			alBufferData(buffer, format, data, datasize, freq);
			alSourceQueueBuffers(m_source, 1, &buffer);
		}
		resume();
	}

	bool isPlaying() {
		return playState() == AL_PLAYING;
	}

	ALint playState() {
		ALint playState = 0;
		alGetSourcei(m_source, AL_SOURCE_STATE, &playState);
		return playState;
	}

	void resume() {
		if (!isPlaying()) {
			ALint bufferQueued;
			alGetSourcei(m_source, AL_BUFFERS_QUEUED, &bufferQueued);
			if(bufferQueued != 0){
				alSourcePlay(m_source);
			}
		}
	}

	void pause() {
		if (isPlaying()) {
			alSourcePause(m_source);
		}
	}

	void stop() {
		if (isPlaying()) {
			alSourceStop(m_source);
		}
	}

	void recycle() {
		cout << (this == nullptr) << endl;
		ALint proceBufNum;
		alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &proceBufNum);
		if (proceBufNum > 0) {
			ALuint buffers[MAX_CACHE];
			alSourceUnqueueBuffers(m_source, proceBufNum, buffers);
			for (int i = 0; i < proceBufNum; ++i) {
				m_bufferQueue.push_back(buffers[i]);
			}
		}
	}

	void makeALCurrent(){
		alcMakeContextCurrent(m_context);
	}

	inline ALCdevice* getDevice() { return m_device; }
	inline ALCcontext* getContext() { return m_context; }
	inline std::list<ALuint>& getBufferQueue() { return m_bufferQueue; }

	static bool checkALError() {
		int loopCnt = 0;
		for (ALenum error = alGetError(); loopCnt < 32 && error; error = alGetError(), ++loopCnt) {
			const char* pMsg;
			switch (error)
			{
			case AL_INVALID_NAME:
				pMsg = "invalid name"; 
				break;
			case AL_INVALID_ENUM:
				pMsg = "invalid enum";
				break;
			case AL_INVALID_VALUE:
				pMsg = "invalid value";
				break;
			case AL_INVALID_OPERATION: 
				pMsg = "invalid operation";
				break;
			case AL_OUT_OF_MEMORY: 
				pMsg = "out of memory";
				break;
			default:
				pMsg = "unknown error";
			}
			cout << "alGetError:" << pMsg << endl;
		}
		return loopCnt != 0;
	}
private:
	
	ALCcontext *m_context;
	ALCdevice *m_device;
	//typedef unsigned int ALuint;
	ALuint m_source;
	ALuint m_buffer[MAX_CACHE];

	std::list<ALuint> m_bufferQueue;
};

int main() {
	PlayBack* playBack = new PlayBack();
	if (!playBack->initSuccess) {
		cout << "create playback fail" << endl;
	}
	playBack->makeALCurrent();
	PrintALInfo();
	static const int FREQ = 44100;
	static const int CAPTURE_SIZE = 512;
	ALCdevice* micDevice = alcCaptureOpenDevice(nullptr, FREQ, AL_FORMAT_MONO16, FREQ / 10);
	if (micDevice == nullptr || alcGetError(micDevice)) {
		cout << "start microphone failed" << endl;
		return -1;
	}
	alcCaptureStart(micDevice);

	ALint samples;
	short audioBuffer[CAPTURE_SIZE];

	auto clearFunc = [&] {
		if (micDevice != nullptr) {
			alcCaptureStop(micDevice);
			alcCaptureCloseDevice(micDevice);
			micDevice = nullptr;
		}
		if (playBack != nullptr) {
			delete playBack;
			playBack = nullptr;
		}

	};

	while (true)
	{
		Sleep(20);
		while (true)
		{
			alcGetIntegerv(micDevice, ALC_CAPTURE_SAMPLES, 1, &samples);
			
			if (samples > CAPTURE_SIZE)
			{
				alcCaptureSamples(micDevice, audioBuffer, CAPTURE_SIZE);
				playBack->recycle();
				playBack->play(audioBuffer, sizeof(audioBuffer), FREQ, AL_FORMAT_MONO16);
				//shouldRedraw = true;
			}
			else
			{
				printf("%d\n", samples);
				break;
			}
		}
	}
	clearFunc();

	return 0;
}