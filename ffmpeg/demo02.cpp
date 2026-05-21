#include <cstdio>
#include <iostream>
#include <string_view>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

static AVFormatContext* openInput(std::string path, AVCodecContext** outCodecCtx, int* outStreamIdx)
{
    AVFormatContext* fmtCtx = nullptr;
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0)
    {
        std::cerr << "无法打开输入文件: " << path << "\n";
        return nullptr;
    }
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        std::cerr << "无法获取流信息\n";
        avformat_close_input(&fmtCtx);
        return nullptr;
    }
    int vIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (vIdx < 0)
    {
        std::cerr << "未找到视频流\n";
        avformat_close_input(&fmtCtx);
        return nullptr;
    }
    const AVCodec* dec = avcodec_find_decoder(fmtCtx->streams[vIdx]->codecpar->codec_id);
    if (!dec)
    {
        std::cerr << "找不到解码器\n";
        avformat_close_input(&fmtCtx);
        return nullptr;
    }
    AVCodecContext* codecCtx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(codecCtx, fmtCtx->streams[vIdx]->codecpar);
    if (avcodec_open2(codecCtx, dec, nullptr) < 0)
    {
        std::cerr << "无法打开解码器\n";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return nullptr;
    }
    *outCodecCtx = codecCtx;
    *outStreamIdx = vIdx;
    return fmtCtx;
}

static AVFormatContext* openOutput(std::string path, AVCodecContext* encCtx)
{
    AVFormatContext* fmtCtx = nullptr;
    avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr, path.c_str());
    if (!fmtCtx)
    {
        std::cerr << "无法创建输出上下文\n";
        return nullptr;
    }
    AVStream* stream = avformat_new_stream(fmtCtx, nullptr);
    avcodec_parameters_from_context(stream->codecpar, encCtx);
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&fmtCtx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            std::cerr << "无法打开输出文件: " << path << "\n";
            avformat_free_context(fmtCtx);
            return nullptr;
        }
    }
    return fmtCtx;
}

int main()
{
    std::string inputPath = "/home/lsm/cppdemos/static/test1.jpg";
    std::string outputPath = "/home/lsm/cppdemos/static/test1副本.jpg";
    std::string watermarkText = "测试水印";

    // 1. 打开输入
    AVCodecContext* decCtx = nullptr;
    int vStreamIdx = 0;
    AVFormatContext* inFmt = openInput(inputPath, &decCtx, &vStreamIdx);
    if (!inFmt)
        return 1;

    int width = decCtx->width, height = decCtx->height;

    // 2. 打开编码器 (输出 JPEG)
    const AVCodec* enc = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext* encCtx = avcodec_alloc_context3(enc);
    encCtx->width = width;
    encCtx->height = height;
    encCtx->time_base = {1, 25};
    encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    encCtx->color_range = AVCOL_RANGE_JPEG;
    if (avcodec_open2(encCtx, enc, nullptr) < 0)
    {
        std::cerr << "无法打开编码器\n";
        return 1;
    }

    AVFormatContext* outFmt = openOutput(outputPath, encCtx);
    if (!outFmt)
        return 1;

    // image2 封装器默认用于序列图片，单张图片需设置 update=1
    av_opt_set_int(outFmt->priv_data, "update", 1, 0);

    if (avformat_write_header(outFmt, nullptr) < 0)
    {
        std::cerr << "无法写文件头\n";
        return 1;
    }

    // 3. 构建 filter graph: buffer -> drawtext -> buffersink
    // 描述输入 buffer 的参数
    char args[256];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=1/25:pixel_aspect=1/1:"
             "colorspace=%d:range=%d",
             width, height, decCtx->pix_fmt, decCtx->colorspace, decCtx->color_range);

    AVFilterGraph* filterGraph = avfilter_graph_alloc();
    const AVFilter* bufSrc = avfilter_get_by_name("buffer");
    const AVFilter* bufSink = avfilter_get_by_name("buffersink");

    AVFilterContext* srcCtx = nullptr;
    AVFilterContext* sinkCtx = nullptr;
    avfilter_graph_create_filter(&srcCtx, bufSrc, "in", args, nullptr, filterGraph);
    avfilter_graph_create_filter(&sinkCtx, bufSink, "out", nullptr, nullptr, filterGraph);

    // drawtext 参数
    char drawtextArgs[512];
    snprintf(drawtextArgs, sizeof(drawtextArgs),
             "font=WenQuanYi Zen Hei:"
             "fontsize=36:fontcolor=white@0.7:"
             "box=1:boxcolor=black@0.4:boxborderw=8:"
             "text='%s':x=w-tw-20:y=20",
             watermarkText.c_str());

    AVFilterContext* drawtextCtx = nullptr;
    const AVFilter* drawtextFilter = avfilter_get_by_name("drawtext");
    if (avfilter_graph_create_filter(&drawtextCtx, drawtextFilter, "drawtext", drawtextArgs, nullptr, filterGraph) < 0)
    {
        std::cerr << "无法创建 drawtext 滤镜（需要 libfreetype）\n";
        avfilter_graph_free(&filterGraph);
        return 1;
    }

    // 指定 buffersink 接受的像素格式（让 FFmpeg 自动插入 scale 转换）
    enum AVPixelFormat sinkFmts[] = {encCtx->pix_fmt, AV_PIX_FMT_NONE};
    av_opt_set_int_list(sinkCtx, "pix_fmts", sinkFmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    avfilter_link(srcCtx, 0, drawtextCtx, 0);
    avfilter_link(drawtextCtx, 0, sinkCtx, 0);
    if (avfilter_graph_config(filterGraph, nullptr) < 0)
    {
        std::cerr << "滤镜图配置失败\n";
        return 1;
    }

    // 4. 读取帧 → 滤镜 → 编码 → 写入
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* filtFrame = av_frame_alloc();
    bool done = false;

    while (!done && av_read_frame(inFmt, pkt) >= 0)
    {
        if (pkt->stream_index == vStreamIdx)
        {
            if (avcodec_send_packet(decCtx, pkt) == 0)
            {
                int ret = avcodec_receive_frame(decCtx, frame);
                if (ret == 0)
                {
                    // 送入 filter graph
                    if (av_buffersrc_add_frame_flags(srcCtx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                        std::cerr << "滤镜输入失败\n";
                    if (av_buffersink_get_frame(sinkCtx, filtFrame) >= 0)
                    {
                        // 编码
                        avcodec_send_frame(encCtx, filtFrame);
                        AVPacket* outPkt = av_packet_alloc();
                        if (avcodec_receive_packet(encCtx, outPkt) == 0)
                        {
                            av_packet_rescale_ts(outPkt, encCtx->time_base, outFmt->streams[0]->time_base);
                            outPkt->stream_index = 0;
                            av_interleaved_write_frame(outFmt, outPkt);
                        }
                        av_packet_free(&outPkt);
                        av_frame_unref(filtFrame);
                    }
                    av_frame_unref(frame);
                }
                else if (ret == AVERROR_EOF)
                {
                    done = true;
                }
            }
        }
        av_packet_unref(pkt);
    }

    // flush filter
    if (av_buffersrc_add_frame_flags(srcCtx, nullptr, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
        std::cerr << "滤镜 flush 失败\n";
    while (!done)
    {
        int ret = av_buffersink_get_frame(sinkCtx, filtFrame);
        if (ret < 0)
            break;
        avcodec_send_frame(encCtx, filtFrame);
        AVPacket* outPkt = av_packet_alloc();
        if (avcodec_receive_packet(encCtx, outPkt) == 0)
        {
            av_packet_rescale_ts(outPkt, encCtx->time_base, outFmt->streams[0]->time_base);
            outPkt->stream_index = 0;
            av_interleaved_write_frame(outFmt, outPkt);
        }
        av_packet_free(&outPkt);
        av_frame_unref(filtFrame);
    }

    // flush encoder
    avcodec_send_frame(encCtx, nullptr);
    AVPacket* outPkt = av_packet_alloc();
    while (avcodec_receive_packet(encCtx, outPkt) == 0)
    {
        av_packet_rescale_ts(outPkt, encCtx->time_base, outFmt->streams[0]->time_base);
        outPkt->stream_index = 0;
        av_interleaved_write_frame(outFmt, outPkt);
    }
    av_packet_free(&outPkt);

    av_write_trailer(outFmt);

    // 清理
    avfilter_graph_free(&filterGraph);
    av_frame_free(&frame);
    av_frame_free(&filtFrame);
    av_packet_free(&pkt);
    avcodec_free_context(&decCtx);
    avcodec_free_context(&encCtx);
    avformat_close_input(&inFmt);
    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outFmt->pb);
    avformat_free_context(outFmt);

    std::cout << "水印添加完成: " << outputPath << "\n";
    return 0;
}
