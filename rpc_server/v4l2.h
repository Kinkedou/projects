#ifndef _V4L2_H
#define _V4L2_H

#define NB_BUFFER 4


int isSupportThisFormat(int iPixelFormat);
int v4l2_init();
int V4l2ExitDevice();
int V4l2PutFrameForStreaming();
int V4l2StartDevice();
int V4l2StopDevice();
int v4l2_read(char* buffer, int* buf_len);
int base64_encode(const unsigned char* in, int in_len, char* out, int out_len);

#endif // !_V4L2_H
