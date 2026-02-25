#ifndef _DETECT_H
#define _DETECT_H

#include <setjmp.h>
#include "jpeglib.h"
#include <stdint.h>


// 视频帧尺寸
#define MAX_WIDTH 800
#define MAX_HEIGHT 600
// 灰度差异阈值（可根据环境调整，值越大越不敏感）
#define PIXEL_THRESHOLD 25
// 差异像素占比阈值（超过此比例判定为移动）
#define DIFF_RATIO_THRESHOLD 0.05

// 错误处理结构体（libjpeg需要）
typedef struct JPEGErrorManager {
    struct jpeg_error_mgr pub;  // 继承libjpeg默认错误管理器
    jmp_buf setjmp_buffer;      // 用于错误跳转的缓冲区
}JPEGErrorManager;

// libjpeg错误处理回调
void jpegErrorExit(j_common_ptr cinfo);
int decodeMJPEGToGray(const unsigned char* mjpeg_data, int data_len,unsigned char* gray_buf, int width, int height);
int detectMotion(const uint8_t* curr_frame, const uint8_t* prev_frame, int width, int height);

#endif // !_DETECT_H
