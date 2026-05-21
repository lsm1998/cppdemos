#include <iostream>
extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
}

// 列出可用的协议（输入/输出）
static void listProtocols()
{
    std::cout << "\n========== 可用协议 ==========\n";
    void* opaque = nullptr;
    const char* name;

    std::cout << "【输入协议】\n";
    while ((name = avio_enum_protocols(&opaque, 0)))
    {
        std::cout << "  " << name << "\n";
    }

    std::cout << "【输出协议】\n";
    opaque = nullptr;
    while ((name = avio_enum_protocols(&opaque, 1)))
    {
        std::cout << "  " << name << "\n";
    }
}

// 列出可用的输入/输出格式（复用 av_demuxer_iterate / av_muxer_iterate）
static void listFormats()
{
    std::cout << "\n========== 可用格式 ==========\n";

    std::cout << "【解封装（输入）】\n";
    const AVInputFormat* ifmt = nullptr;
    void* iter = nullptr;
    while ((ifmt = av_demuxer_iterate(&iter)))
    {
        std::cout << "  " << ifmt->name;
        if (ifmt->long_name)
            std::cout << " — " << ifmt->long_name;
        std::cout << "\n";
    }

    std::cout << "【封装（输出）】\n";
    const AVOutputFormat* ofmt = nullptr;
    iter = nullptr;
    while ((ofmt = av_muxer_iterate(&iter)))
    {
        std::cout << "  " << ofmt->name;
        if (ofmt->long_name)
            std::cout << " — " << ofmt->long_name;
        std::cout << "\n";
    }
}

// 列出音视频输入/输出设备
static void listDevices()
{
    std::cout << "\n========== 可用设备 ==========\n";

    // 输入设备
    const AVInputFormat* ifmt = nullptr;
    void* iter = nullptr;
    bool hasInput = false, hasOutput = false;

    while ((ifmt = av_demuxer_iterate(&iter)))
    {
        if (ifmt->priv_class && ifmt->priv_class->category == AV_CLASS_CATEGORY_DEVICE_INPUT)
        {
            if (!hasInput)
            {
                std::cout << "【输入设备】\n";
                hasInput = true;
            }
            std::cout << "  " << ifmt->name;
            if (ifmt->long_name)
                std::cout << " — " << ifmt->long_name;
            std::cout << "\n";
        }
    }

    if (!hasInput)
        std::cout << "  (无)\n";

    // 输出设备
    const AVOutputFormat* ofmt = nullptr;
    iter = nullptr;
    while ((ofmt = av_muxer_iterate(&iter)))
    {
        if (ofmt->priv_class && ofmt->priv_class->category == AV_CLASS_CATEGORY_DEVICE_OUTPUT)
        {
            if (!hasOutput)
            {
                std::cout << "【输出设备】\n";
                hasOutput = true;
            }
            std::cout << "  " << ofmt->name;
            if (ofmt->long_name)
                std::cout << " — " << ofmt->long_name;
            std::cout << "\n";
        }
    }

    if (!hasOutput)
        std::cout << "  (无)\n";
}

int main()
{
    // 注册所有组件（FFmpeg 4.x 后非必须，但显式调用兼容旧版）
    avdevice_register_all();

    // 打印基本查询信息
    std::cout << "FFmpeg 版本: " << av_version_info() << "\n";
    listProtocols();
    listFormats();
    listDevices();

    return 0;
}
