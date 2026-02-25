#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "detect.h"




// libjpeg错误处理回调
void jpegErrorExit(j_common_ptr cinfo) {
    JPEGErrorManager* err = (JPEGErrorManager*)cinfo->err;
    (*cinfo->err->output_message)(cinfo);  // 打印libjpeg原生错误信息
    longjmp(err->setjmp_buffer, 1);        // 跳转到之前设置的错误处理点
}

// MJPEG解码为灰度图
int decodeMJPEGToGray(const unsigned char* mjpeg_data, int data_len,
    unsigned char* gray_buf, int width, int height) {
    // 1. 初始化JPEG解码结构体和错误处理
    struct jpeg_decompress_struct cinfo;
    JPEGErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);  // 绑定默认错误管理器
    jerr.pub.error_exit = jpegErrorExit;    // 替换为自定义错误回调
    // 设置错误跳转点：如果解码出错，会跳回这里，返回false
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);    // 释放资源，避免内存泄漏
        printf("MJPEG解码失败\n");
        return -1;
    }

    // 2. 初始化解码流程
    jpeg_create_decompress(&cinfo);        // 创建解码实例
    jpeg_mem_src(&cinfo, mjpeg_data, data_len);  // 读取MJPEG数据
    jpeg_read_header(&cinfo, TRUE);        // 读取JPEG头信息（宽、高、格式）

    // 3. 强制解码为灰度格式
    cinfo.out_color_space = JCS_GRAYSCALE; // 输出灰度图，而非默认的RGB
    jpeg_start_decompress(&cinfo);         // 开始解码

    // 4. 获取解码后图像尺寸
    width = cinfo.output_width;
    height = cinfo.output_height;
	if (width > MAX_WIDTH || height > MAX_HEIGHT) {// 安全检查，避免解码过大图像导致内存问题
        printf("图像尺寸超出限制\n");
        jpeg_abort_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }

    // 5. 逐行读取解码后的灰度数据，写入缓存
    int row_stride = cinfo.output_width * cinfo.output_components; // 每行字节数（灰度图=宽度×1）
    // 申请临时行缓冲区（libjpeg的内存池管理，避免手动malloc）
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    int row = 0;
    // 循环读取每一行像素数据
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);  // 读取一行
        memcpy(gray_buf + row * width, buffer[0], width); // 拷贝到灰度缓存
        row++;
    }

    // 6. 释放解码资源
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 0;
}

// 运动检测函数：比较当前帧和前一帧的灰度图，计算差异像素占比
int detectMotion(const uint8_t* curr_frame, const uint8_t* prev_frame, int width, int height) {
    if (curr_frame == NULL || prev_frame == NULL) {
        return -1;
    }

    int total_pixels = width * height;
    int diff_count = 0;

    // 逐像素计算差异
    for (int i = 0; i < total_pixels; i++) {
        int diff = abs((int)curr_frame[i] - (int)prev_frame[i]);
        if (diff > PIXEL_THRESHOLD) {
            diff_count++;
        }
    }

    // 计算差异占比
    float diff_ratio = (float)diff_count / total_pixels;
    return (diff_ratio > DIFF_RATIO_THRESHOLD) ? 1 : 0;
}









    




