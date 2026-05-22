#include <cstdio>
#include <iostream>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
}

static AVFormatContext* openInput(const std::string& path, AVCodecContext** vCtx, int* vIdx,
                                   AVCodecContext** aCtx, int* aIdx)
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
    *vIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    *aIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (*vIdx >= 0)
    {
        const AVCodec* dec = avcodec_find_decoder(fmtCtx->streams[*vIdx]->codecpar->codec_id);
        *vCtx = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(*vCtx, fmtCtx->streams[*vIdx]->codecpar);
        avcodec_open2(*vCtx, dec, nullptr);
    }
    if (*aIdx >= 0)
    {
        const AVCodec* dec = avcodec_find_decoder(fmtCtx->streams[*aIdx]->codecpar->codec_id);
        *aCtx = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(*aCtx, fmtCtx->streams[*aIdx]->codecpar);
        avcodec_open2(*aCtx, dec, nullptr);
    }
    return fmtCtx;
}

int main()
{
    std::string inputPath = "/home/lsm/cppdemos/static/test2.mp4";
    std::string outputPath = "/home/lsm/cppdemos/static/test2_副本.mp4";
    std::string watermarkText = "测试水印";

    // 1. 打开输入
    AVCodecContext *vDecCtx = nullptr, *aDecCtx = nullptr;
    int vInIdx = -1, aInIdx = -1;
    AVFormatContext* inFmt = openInput(inputPath, &vDecCtx, &vInIdx, &aDecCtx, &aInIdx);
    if (!inFmt) return 1;
    if (vInIdx < 0) { std::cerr << "没有视频流\n"; return 1; }

    // 2. 创建输出
    AVFormatContext* outFmt = nullptr;
    avformat_alloc_output_context2(&outFmt, nullptr, nullptr, outputPath.c_str());
    if (!outFmt) return 1;

    // 3. 视频编码器 (libx264)
    const AVCodec* vEnc = avcodec_find_encoder_by_name("libx264");
    AVCodecContext* vEncCtx = avcodec_alloc_context3(vEnc);
    vEncCtx->width = vDecCtx->width;
    vEncCtx->height = vDecCtx->height;
    vEncCtx->time_base = inFmt->streams[vInIdx]->time_base;
    vEncCtx->framerate = av_guess_frame_rate(inFmt, inFmt->streams[vInIdx], nullptr);
    vEncCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    av_opt_set(vEncCtx->priv_data, "preset", "fast", 0);
    av_opt_set(vEncCtx->priv_data, "crf", "23", 0);
    avcodec_open2(vEncCtx, vEnc, nullptr);
    AVStream* vOutStream = avformat_new_stream(outFmt, nullptr);
    avcodec_parameters_from_context(vOutStream->codecpar, vEncCtx);
    vOutStream->time_base = vEncCtx->time_base;

    // 4. 音频编码器 + FIFO (HE-AAC 解码输出 2048 samples，AAC-LC 编码需 1024)
    AVCodecContext* aEncCtx = nullptr;
    AVStream* aOutStream = nullptr;
    AVAudioFifo* aFifo = nullptr;
    int aEncFrameSize = 1024;
    int64_t aPts = 0;

    if (aInIdx >= 0)
    {
        const AVCodec* aEnc = avcodec_find_encoder_by_name("aac");
        aEncCtx = avcodec_alloc_context3(aEnc);
        aEncCtx->sample_rate = aDecCtx->sample_rate;
        aEncCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        av_channel_layout_copy(&aEncCtx->ch_layout, &aDecCtx->ch_layout);
        aEncCtx->time_base = {1, aDecCtx->sample_rate};
        avcodec_open2(aEncCtx, aEnc, nullptr);
        aOutStream = avformat_new_stream(outFmt, nullptr);
        avcodec_parameters_from_context(aOutStream->codecpar, aEncCtx);
        aOutStream->time_base = aEncCtx->time_base;

        aFifo = av_audio_fifo_alloc(aDecCtx->sample_fmt,
                                     aDecCtx->ch_layout.nb_channels, 1);
    }

    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&outFmt->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            std::cerr << "无法打开输出文件\n";
            return 1;
        }
    }
    if (avformat_write_header(outFmt, nullptr) < 0)
    {
        std::cerr << "无法写文件头\n";
        return 1;
    }

    // 5. 构建视频 filter graph: buffer -> drawtext -> buffersink
    char args[256];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=1/1:"
             "colorspace=%d:range=%d",
             vDecCtx->width, vDecCtx->height, vDecCtx->pix_fmt,
             inFmt->streams[vInIdx]->time_base.num,
             inFmt->streams[vInIdx]->time_base.den,
             vDecCtx->colorspace, vDecCtx->color_range);

    AVFilterGraph* filterGraph = avfilter_graph_alloc();
    AVFilterContext *srcCtx = nullptr, *sinkCtx = nullptr;
    avfilter_graph_create_filter(&srcCtx, avfilter_get_by_name("buffer"), "in", args, nullptr, filterGraph);
    avfilter_graph_create_filter(&sinkCtx, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, filterGraph);

    char drawtextArgs[512];
    snprintf(drawtextArgs, sizeof(drawtextArgs),
             "font=WenQuanYi Zen Hei:"
             "fontsize=36:fontcolor=white@0.7:"
             "box=1:boxcolor=black@0.4:boxborderw=8:"
             "text='%s':x=w-tw-20:y=20",
             watermarkText.c_str());

    AVFilterContext* drawtextCtx = nullptr;
    avfilter_graph_create_filter(&drawtextCtx, avfilter_get_by_name("drawtext"), "drawtext",
                                  drawtextArgs, nullptr, filterGraph);

    enum AVPixelFormat sinkFmts[] = {vEncCtx->pix_fmt, AV_PIX_FMT_NONE};
    av_opt_set_int_list(sinkCtx, "pix_fmts", sinkFmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    avfilter_link(srcCtx, 0, drawtextCtx, 0);
    avfilter_link(drawtextCtx, 0, sinkCtx, 0);
    if (avfilter_graph_config(filterGraph, nullptr) < 0)
    {
        std::cerr << "滤镜图配置失败\n";
        return 1;
    }

    // 6. 主循环：读取 → 解码 → 滤镜/编码 → 写入
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* filtFrame = av_frame_alloc();

    while (av_read_frame(inFmt, pkt) >= 0)
    {
        if (pkt->stream_index == vInIdx)
        {
            avcodec_send_packet(vDecCtx, pkt);
            while (avcodec_receive_frame(vDecCtx, frame) >= 0)
            {
                if (av_buffersrc_add_frame_flags(srcCtx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                    std::cerr << "滤镜输入失败\n";
                while (av_buffersink_get_frame(sinkCtx, filtFrame) >= 0)
                {
                    // 保留解码器的 PTS（滤镜图不改变 PTS）
                    avcodec_send_frame(vEncCtx, filtFrame);
                    AVPacket* outPkt = av_packet_alloc();
                    while (avcodec_receive_packet(vEncCtx, outPkt) >= 0)
                    {
                        av_packet_rescale_ts(outPkt, vEncCtx->time_base, vOutStream->time_base);
                        outPkt->stream_index = vOutStream->index;
                        av_interleaved_write_frame(outFmt, outPkt);
                    }
                    av_packet_free(&outPkt);
                    av_frame_unref(filtFrame);
                }
                av_frame_unref(frame);
            }
        }
        else if (pkt->stream_index == aInIdx && aEncCtx)
        {
            avcodec_send_packet(aDecCtx, pkt);
            while (avcodec_receive_frame(aDecCtx, frame) >= 0)
            {
                // 写入 FIFO 缓冲
                av_audio_fifo_write(aFifo, (void**)frame->data, frame->nb_samples);

                // 从 FIFO 取出 encoder frame_size 大小的数据送给编码器
                while (av_audio_fifo_size(aFifo) >= aEncFrameSize)
                {
                    AVFrame* aEncFrame = av_frame_alloc();
                    aEncFrame->nb_samples = aEncFrameSize;
                    aEncFrame->format = aEncCtx->sample_fmt;
                    aEncFrame->sample_rate = aEncCtx->sample_rate;
                    av_channel_layout_copy(&aEncFrame->ch_layout, &aEncCtx->ch_layout);
                    av_frame_get_buffer(aEncFrame, 0);
                    av_audio_fifo_read(aFifo, (void**)aEncFrame->data, aEncFrameSize);
                    aEncFrame->pts = aPts;
                    aPts += aEncFrameSize;

                    avcodec_send_frame(aEncCtx, aEncFrame);
                    AVPacket* outPkt = av_packet_alloc();
                    while (avcodec_receive_packet(aEncCtx, outPkt) >= 0)
                    {
                        av_packet_rescale_ts(outPkt, aEncCtx->time_base, aOutStream->time_base);
                        outPkt->stream_index = aOutStream->index;
                        av_interleaved_write_frame(outFmt, outPkt);
                    }
                    av_packet_free(&outPkt);
                    av_frame_free(&aEncFrame);
                }
            }
        }
        av_packet_unref(pkt);
    }

    // 7. flush video filter & encoder
    if (av_buffersrc_add_frame_flags(srcCtx, nullptr, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
        std::cerr << "滤镜 flush 失败\n";
    while (av_buffersink_get_frame(sinkCtx, filtFrame) >= 0)
    {
        avcodec_send_frame(vEncCtx, filtFrame);
        AVPacket* outPkt = av_packet_alloc();
        while (avcodec_receive_packet(vEncCtx, outPkt) >= 0)
        {
            av_packet_rescale_ts(outPkt, vEncCtx->time_base, vOutStream->time_base);
            outPkt->stream_index = vOutStream->index;
            av_interleaved_write_frame(outFmt, outPkt);
        }
        av_packet_free(&outPkt);
        av_frame_unref(filtFrame);
    }
    avcodec_send_frame(vEncCtx, nullptr);
    AVPacket* outPkt = av_packet_alloc();
    while (avcodec_receive_packet(vEncCtx, outPkt) >= 0)
    {
        av_packet_rescale_ts(outPkt, vEncCtx->time_base, vOutStream->time_base);
        outPkt->stream_index = vOutStream->index;
        av_interleaved_write_frame(outFmt, outPkt);
    }
    av_packet_free(&outPkt);

    // flush audio encoder
    if (aEncCtx)
    {
        // 冲洗 FIFO 中剩余样本
        int remaining = av_audio_fifo_size(aFifo);
        if (remaining > 0)
        {
            AVFrame* aEncFrame = av_frame_alloc();
            aEncFrame->nb_samples = remaining;
            aEncFrame->format = aEncCtx->sample_fmt;
            aEncFrame->sample_rate = aEncCtx->sample_rate;
            av_channel_layout_copy(&aEncFrame->ch_layout, &aEncCtx->ch_layout);
            av_frame_get_buffer(aEncFrame, 0);
            av_audio_fifo_read(aFifo, (void**)aEncFrame->data, remaining);
            aEncFrame->pts = aPts;

            avcodec_send_frame(aEncCtx, aEncFrame);
            outPkt = av_packet_alloc();
            while (avcodec_receive_packet(aEncCtx, outPkt) >= 0)
            {
                av_packet_rescale_ts(outPkt, aEncCtx->time_base, aOutStream->time_base);
                outPkt->stream_index = aOutStream->index;
                av_interleaved_write_frame(outFmt, outPkt);
            }
            av_packet_free(&outPkt);
            av_frame_free(&aEncFrame);
        }

        avcodec_send_frame(aEncCtx, nullptr);
        outPkt = av_packet_alloc();
        while (avcodec_receive_packet(aEncCtx, outPkt) >= 0)
        {
            av_packet_rescale_ts(outPkt, aEncCtx->time_base, aOutStream->time_base);
            outPkt->stream_index = aOutStream->index;
            av_interleaved_write_frame(outFmt, outPkt);
        }
        av_packet_free(&outPkt);
    }

    av_write_trailer(outFmt);

    // 清理
    avfilter_graph_free(&filterGraph);
    av_frame_free(&frame);
    av_frame_free(&filtFrame);
    av_packet_free(&pkt);
    avcodec_free_context(&vDecCtx);
    avcodec_free_context(&vEncCtx);
    if (aDecCtx) avcodec_free_context(&aDecCtx);
    if (aEncCtx) avcodec_free_context(&aEncCtx);
    if (aFifo) av_audio_fifo_free(aFifo);
    avformat_close_input(&inFmt);
    if (!(outFmt->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outFmt->pb);
    avformat_free_context(outFmt);

    std::cout << "视频水印添加完成: " << outputPath << "\n";
    return 0;
}
