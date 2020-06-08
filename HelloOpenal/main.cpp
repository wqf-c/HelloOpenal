#include <iostream>
#include <alut.h>
#include <al.h>

void main1()
{
	alutInit(NULL, NULL);
	ALuint source1;
	alGenSources(1, &source1);

	ALuint buffer1 = alutCreateBufferFromFile("D://code//myCode//c++//HelloOpenal//HelloOpenal//test.wav");
	alSourcei(source1, AL_BUFFER, buffer1);
	alSourcePlay(source1);

	ALint state;
	do {
		alGetSourcei(source1, AL_SOURCE_STATE, &state);
	} while (state == AL_PLAYING);

	alDeleteSources(1, &source1);
	alDeleteBuffers(1, &buffer1);
	alutExit();
}
