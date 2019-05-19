#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <QThread>
#include <QString>
#include <QImage>
#include <stdio.h>
#include <QLabel>

extern "C"
{
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/common.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/samplefmt.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
	#include <libavutil/time.h>
}


class FFmpegPlayer : public QThread
{
    Q_OBJECT
public:
    FFmpegPlayer();
    ~FFmpegPlayer();

    void initFfmpeg();
    void run();
    void uninit();
    void startPlay();
private:
    char filepath[64];
    int videoindex;
    char thread_status;
    unsigned char* out_buffer_rgb;

    AVFormatContext *pFormatCtx;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame,*pFrameRGB;
    AVPacket        *packet;
    QImage           finalImage;
    AVPixelFormat    pixFormat;
    struct SwsContext *img_convert_ctx;

signals:
    void sigGetOneFrame(QImage);

public slots:
};

#endif // FFMPEGPLAYER_H
