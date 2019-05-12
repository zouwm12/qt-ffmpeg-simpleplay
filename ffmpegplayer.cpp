#include "ffmpegplayer.h"
#include <QDebug>
FFmpegPlayer::FFmpegPlayer()
{
    this->videoindex =-1;
    this->thread_status = 0;
    snprintf(this->filepath,sizeof(this->filepath),"%s","my.mp4");
    this->out_buffer_rgb = nullptr;
    this->pFormatCtx = nullptr;
    this->pFrame = nullptr;
    this->pFrameRGB = nullptr;
    this->pCodec = nullptr;
    this->pCodecCtx = nullptr;
    this->packet = nullptr;
    this->img_convert_ctx = nullptr;

    this->initFfmpeg();
}

FFmpegPlayer::~FFmpegPlayer()
{
    uninit();
}

void FFmpegPlayer::initFfmpeg()
{
    /**
     * 解封装
     */

    //step 1:open file,get format info from file header
    if (avformat_open_input(&pFormatCtx, filepath, nullptr, nullptr) != 0){
        fprintf(stderr,"avformat_open_input");
        return;
    }
    //step 2:get stread info
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0){
        fprintf(stderr,"avformat_find_stream_info");
        return;
    }
    //打印相关信息
    av_dump_format(pFormatCtx, 0, filepath, 0);
    //step 3:查找video在AVFormatContext的哪个stream中
    for (int i = 0; i < static_cast<int>(pFormatCtx->nb_streams); i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1){
        fprintf(stderr,"find video stream error");
        return;
    }

    /**
     * 解码
     */

    //step 4:find  decoder
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);

    if (!pCodec){
        fprintf(stderr,"avcodec_find_decoder error");
        return;
    }
    //step 5:get one instance of AVCodecContext,decode need it.
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    //step 6: open codec
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0){
        fprintf(stderr,"avcodec_open2 error");
        return;
    }


    pFrame = av_frame_alloc();
    pFrameRGB=av_frame_alloc();

    //(7) set the final frame format

    switch (pCodecCtx->pix_fmt)
    {
    case AV_PIX_FMT_YUVJ420P :
        pixFormat = AV_PIX_FMT_YUV420P;
        break;
    case AV_PIX_FMT_YUVJ422P  :
        pixFormat = AV_PIX_FMT_YUV422P;
        break;
    case AV_PIX_FMT_YUVJ444P   :
        pixFormat = AV_PIX_FMT_YUV444P;
        break;
    case AV_PIX_FMT_YUVJ440P :
        pixFormat = AV_PIX_FMT_YUV440P;
        break;
    default:
        pixFormat = pCodecCtx->pix_fmt;
        break;
    }

    //图像色彩空间转换（初始化）
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
            pixFormat, pCodecCtx->width, pCodecCtx->height,
            AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);

    //(8) caculate and alloc the size stroage we need
    size_t numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height,1);
    out_buffer_rgb = static_cast<unsigned char*> (av_malloc(numBytes));

    //(9) connect pFrame and out_buffer_rgb
    av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize,out_buffer_rgb, AV_PIX_FMT_RGB24,
                         pCodecCtx->width, pCodecCtx->height,1);
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    this->startPlay();
}

void FFmpegPlayer::startPlay()
{
    thread_status = 1;
    this->start();
}

void FFmpegPlayer::uninit()
{
    thread_status = 0;
    wait();
    av_packet_unref(packet);
    av_free(out_buffer_rgb);
    av_free(pFrame);
    av_free(pFrameRGB);
    sws_freeContext(img_convert_ctx);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

}

void FFmpegPlayer::run()
{
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        if(thread_status != 1)
        {
            av_packet_unref(packet);
            break;
        }
        if (packet->stream_index == videoindex)			//处理视频
        {
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    break;
                }
                fflush(stdout);
                //转换
                //如果srcSliceY=0，srcSliceH=height，表示一次性处理完整个图像,也可多线程分别处理
                sws_scale(img_convert_ctx,
                        (uint8_t const * const *) pFrame->data,
                        pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data,
                        pFrameRGB->linesize);
                //(2) Use QImage to carry the Image
                QImage tmpImg((uchar *)out_buffer_rgb,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB888);
                finalImage = tmpImg.convertToFormat(QImage::Format_RGB888,Qt::NoAlpha);
                emit sigGetOneFrame(finalImage);
                usleep(40000);
            }
        }
        av_packet_unref(packet);
    }
}

