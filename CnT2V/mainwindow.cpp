#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //  初始化UI
    QImage img_out(ui->label_outputvideo->width(), ui->label_outputvideo->height(), QImage::Format_RGB888);
    img_out.fill(QColor(0, 0, 255));
    ui->label_outputvideo->setPixmap(QPixmap::fromImage(img_out));

    //  设置当前忙状态
    is_cur_busy = true;

    //  提示
    ui->progressBar->setFormat("Initializing environment, please wait...");

    //  关闭控件
    ui->pushButton_play   ->setEnabled(false);
    ui->pushButton_pause  ->setEnabled(false);
    ui->pushButton_stop   ->setEnabled(false);
    ui->pushButton_bycn   ->setEnabled(false);
    ui->pushButton_byen   ->setEnabled(false);
    ui->pushButton_tslonly->setEnabled(false);
    ui->spinBox_vd        ->setEnabled(false);

    //  设置视频时长默认值
    ui->spinBox_vd->setValue(2);

    //  视频播放
    m_mediaplayer = nullptr;

    //  视频播放状态
    video_st = EVideoSt_Stop;

    //  视频探针
    m_videoprobe = nullptr;

    //  初始化AI环境
    connect(&qtai, &CQtAI::send_translate_cn2en_finish, this, &MainWindow::slot_OnTranslateCn2EnFinish);
    connect(&qtai, &CQtAI::send_text_to_video_finish,   this, &MainWindow::slot_OnTextToVideoFinish);
    connect(&qtai, &CQtAI::send_environment_ready,      this, &MainWindow::slot_OnAIEnvReady);
    //qtai.setStackSize(1024*1024*1024);
    qtai.Init();
    qtai.start();

    //  定时器初始化
    timer = new QTimer(this);
    timer->setInterval(30);
    connect(timer, SIGNAL(timeout()), this, SLOT(slot_timeout()));
    timer->start();

}

MainWindow::~MainWindow()
{
    //  释放视频资源
    if(m_mediaplayer != nullptr)
    {
        m_mediaplayer->stop();
        delete m_mediaplayer;
        m_mediaplayer = nullptr;
    }

    //  释放视频探针
    if(m_videoprobe != nullptr)
    {
        delete m_videoprobe;
        m_videoprobe = nullptr;
    }

    //  释放定时器
    if(timer != nullptr)
    {
        timer->stop();
        delete timer;
        timer = nullptr;
    }

    //  释放AI资源
    qtai.Release();

    //  释放UI资源
    delete ui;
}


//  当AI环境就绪
void MainWindow::slot_OnAIEnvReady(void)
{
    //  开启UI
    ui->pushButton_play   ->setEnabled(true);
    ui->pushButton_pause  ->setEnabled(true);
    ui->pushButton_stop   ->setEnabled(true);
    ui->pushButton_bycn   ->setEnabled(true);
    ui->pushButton_byen   ->setEnabled(true);
    ui->pushButton_tslonly->setEnabled(true);
    ui->spinBox_vd        ->setEnabled(true);

    //  提示
    is_cur_busy = false;
    ui->progressBar->setFormat("Ready!!");

    //  代码测试用
    //this->LoadVideo("/home/rainhenry/CnT2V/tmp13.mp4");
    //qDebug("stack size = %d", qtai.stackSize());
}

//  当完成一次翻译
void MainWindow::slot_OnTranslateCn2EnFinish(QString out_text, qint64 run_time_ns)
{
    //  检查文字内容的最后1个字符
    if(!out_text.isEmpty())
    {
        //  当为英文句号的时候，删除该字符
        if(*(out_text.end()-1) == '.')
        {
            out_text.remove(out_text.length() - 1, 1);
        }
    }

    //  更新输出文字内容
    ui->textEdit_english->setText(out_text);
    std::string tmp_str = out_text.toStdString();
    qDebug("out_text = [%s]", tmp_str.c_str());

    //  当为只进行翻译操作
    if(only_tsl_flag)
    {
        //  忙完
        is_cur_busy = false;

        //  开启UI
        ui->pushButton_play   ->setEnabled(true);
        ui->pushButton_pause  ->setEnabled(true);
        ui->pushButton_stop   ->setEnabled(true);
        ui->pushButton_bycn   ->setEnabled(true);
        ui->pushButton_byen   ->setEnabled(true);
        ui->pushButton_tslonly->setEnabled(true);
        ui->spinBox_vd        ->setEnabled(true);

        //  设置状态
        //  当为正常的文字内容
        if(!out_text.isEmpty())
        {
            long long ms = run_time_ns / 1000000LL;
            int sec = static_cast<int>(ms / 1000);
            int min = sec / 60;
            int hour = min / 60;
            QString str = QString::asprintf("Translate Done(%d:%02d:%02d.%03lld)",
                                            hour, min % 60, sec % 60, ms % 1000LL
                                           );
            ui->progressBar->setFormat(str);
        }
        //  当输出文字内容为空
        else
        {
            //  提示错误
            ui->progressBar->setFormat("Translation result is empty!!");
        }
    }
    //  否则直接执行文字生成视频
    else
    {
        //  当为正常的文字内容
        if(!out_text.isEmpty())
        {
            //  获取总视频时长，并计算总帧数
            int total_frames = ui->spinBox_vd->value() * 8;

            //  检查总帧数
            if(total_frames < TOTAL_FRAME_NUM_MIN)
            {
                total_frames = TOTAL_FRAME_NUM_MIN;
            }

            //  执行
            is_cur_busy = true;
        #if BEGIN_DELETE_VIDEO_TMP_FILE
            std::string cmd;
            cmd = "rm -rf ";
            cmd += TEXT_TO_VIDEO_TMP_FILE;
            system(cmd.c_str());
        #endif  //  BEGIN_DELETE_VIDEO_TMP_FILE
            qtai.ExTextToVideo(out_text, total_frames, TEXT_TO_VIDEO_TMP_FILE);

            //  设置状态
            ui->progressBar->setFormat("Generateing Video...");

            //  关闭控件
            ui->pushButton_play   ->setEnabled(false);
            ui->pushButton_pause  ->setEnabled(false);
            ui->pushButton_stop   ->setEnabled(false);
            ui->pushButton_bycn   ->setEnabled(false);
            ui->pushButton_byen   ->setEnabled(false);
            ui->pushButton_tslonly->setEnabled(false);
            ui->spinBox_vd        ->setEnabled(false);
        }
        //  当输出文字内容为空
        else
        {
            //  忙完
            is_cur_busy = false;

            //  开启UI
            ui->pushButton_play   ->setEnabled(true);
            ui->pushButton_pause  ->setEnabled(true);
            ui->pushButton_stop   ->setEnabled(true);
            ui->pushButton_bycn   ->setEnabled(true);
            ui->pushButton_byen   ->setEnabled(true);
            ui->pushButton_tslonly->setEnabled(true);
            ui->spinBox_vd        ->setEnabled(true);

            //  提示错误
            ui->progressBar->setFormat("Translation result is empty!!");
        }
    }
}

//  当完成一次文字生成视频
void MainWindow::slot_OnTextToVideoFinish(qint64 run_time_ns)
{
    //  忙完
    is_cur_busy = false;

    //  开启UI
    ui->pushButton_play   ->setEnabled(true);
    ui->pushButton_pause  ->setEnabled(true);
    ui->pushButton_stop   ->setEnabled(true);
    ui->pushButton_bycn   ->setEnabled(true);
    ui->pushButton_byen   ->setEnabled(true);
    ui->pushButton_tslonly->setEnabled(true);
    ui->spinBox_vd        ->setEnabled(true);

    //  设置状态
    long long ms = run_time_ns / 1000000LL;
    int sec = static_cast<int>(ms / 1000);
    int min = sec / 60;
    int hour = min / 60;
    QString str = QString::asprintf("Generate Video Done(%d:%02d:%02d.%03lld)",
                                    hour, min % 60, sec % 60, ms % 1000LL
                                   );
    ui->progressBar->setFormat(str);

    //  载入视频
    QString currentPath = QCoreApplication::applicationDirPath();
    this->LoadVideo(currentPath + "/" + TEXT_TO_VIDEO_TMP_FILE);

    //  自动播放视频
    this->on_pushButton_play_clicked();
}

//  定时器
void MainWindow::slot_timeout()
{
    //  进度变量
    static int cur_proc = 100;

    //  当忙
    if(is_cur_busy)
    {
        if(cur_proc >= 100) cur_proc = 0;
        else                cur_proc = cur_proc + 1;
    }
    //  当忙完
    else
    {
        cur_proc = 100;
    }

    //  更新到控件
    ui->progressBar->setValue(cur_proc);
}

//  载入视频
void MainWindow::LoadVideo(QString filename)
{
    //  释放之前的资源
    if(m_mediaplayer != nullptr)
    {
        m_mediaplayer->stop();
        delete m_mediaplayer;
        m_mediaplayer = nullptr;
    }

    //  创建视频播放
    m_mediaplayer = new QMediaPlayer(this);

    //  检查视频探针
    if(m_videoprobe == nullptr)
    {
        //  创建视频帧探针
        m_videoprobe = new QVideoProbe;

        //  链接视频帧探针的信号
        connect(m_videoprobe, SIGNAL(videoFrameProbed(QVideoFrame)), this, SLOT(slot_OnVideoProbeFrame(QVideoFrame)));
    }

    //  配置探针的源数据
    m_videoprobe ->setSource(m_mediaplayer);

    //  载入视频文件
    m_mediaplayer->setMedia(QUrl::fromLocalFile(filename));

    //  载入后，先暂停
    m_mediaplayer->pause();
    video_st = EVideoSt_Pause;
}

//  单击视频播放按钮
void MainWindow::on_pushButton_play_clicked()
{
    //  控制视频
    m_mediaplayer->play();
    video_st = EVideoSt_Play;
}

//  单击视频暂停按钮
void MainWindow::on_pushButton_pause_clicked()
{
    m_mediaplayer->pause();
    video_st = EVideoSt_Pause;
}

//  单击视频停止按钮
void MainWindow::on_pushButton_stop_clicked()
{
    //  控制视频
    m_mediaplayer->stop();
    m_mediaplayer->pause();
    video_st = EVideoSt_Stop;
}

//  单击通过中文生成视频按钮
void MainWindow::on_pushButton_bycn_clicked()
{
    //  检查输入文字
    QString in_text = ui->textEdit_chinese->toPlainText();
    if(in_text.isEmpty())
    {
        //  提示
        ui->progressBar->setFormat("Input Chinese Text is NULL!");
        return;
    }

    //  执行
    is_cur_busy = true;
    qtai.ExTranslateCn2En(in_text);

    //  设置状态
    ui->progressBar->setFormat("Translate Processing...");

    //  关闭控件
    ui->pushButton_play   ->setEnabled(false);
    ui->pushButton_pause  ->setEnabled(false);
    ui->pushButton_stop   ->setEnabled(false);
    ui->pushButton_bycn   ->setEnabled(false);
    ui->pushButton_byen   ->setEnabled(false);
    ui->pushButton_tslonly->setEnabled(false);
    ui->spinBox_vd        ->setEnabled(false);

    //  设置标志
    only_tsl_flag = false;
}

//  单击通过英文生成视频按钮
void MainWindow::on_pushButton_byen_clicked()
{
    //  检查输入文字
    QString in_text = ui->textEdit_english->toPlainText();
    if(in_text.isEmpty())
    {
        //  提示
        ui->progressBar->setFormat("Input English Text is NULL!");
        return;
    }

    //  获取总视频时长，并计算总帧数
    int total_frames = ui->spinBox_vd->value() * 8;

    //  检查总帧数
    if(total_frames < TOTAL_FRAME_NUM_MIN)
    {
        total_frames = TOTAL_FRAME_NUM_MIN;
    }

    //  执行
    is_cur_busy = true;
#if BEGIN_DELETE_VIDEO_TMP_FILE
    std::string cmd;
    cmd = "rm -rf ";
    cmd += TEXT_TO_VIDEO_TMP_FILE;
    system(cmd.c_str());
#endif  //  BEGIN_DELETE_VIDEO_TMP_FILE
    qtai.ExTextToVideo(in_text, total_frames, TEXT_TO_VIDEO_TMP_FILE);

    //  设置状态
    ui->progressBar->setFormat("Generateing Video...");

    //  关闭控件
    ui->pushButton_play   ->setEnabled(false);
    ui->pushButton_pause  ->setEnabled(false);
    ui->pushButton_stop   ->setEnabled(false);
    ui->pushButton_bycn   ->setEnabled(false);
    ui->pushButton_byen   ->setEnabled(false);
    ui->pushButton_tslonly->setEnabled(false);
    ui->spinBox_vd        ->setEnabled(false);
}

//  单击只将中文翻译成英文的按钮
void MainWindow::on_pushButton_tslonly_clicked()
{
    //  检查输入文字
    QString in_text = ui->textEdit_chinese->toPlainText();
    if(in_text.isEmpty())
    {
        //  提示
        ui->progressBar->setFormat("Input Chinese Text is NULL!");
        return;
    }

    //  执行
    is_cur_busy = true;
    qtai.ExTranslateCn2En(in_text);

    //  设置状态
    ui->progressBar->setFormat("Translate Processing...");

    //  关闭控件
    ui->pushButton_play   ->setEnabled(false);
    ui->pushButton_pause  ->setEnabled(false);
    ui->pushButton_stop   ->setEnabled(false);
    ui->pushButton_bycn   ->setEnabled(false);
    ui->pushButton_byen   ->setEnabled(false);
    ui->pushButton_tslonly->setEnabled(false);
    ui->spinBox_vd        ->setEnabled(false);

    //  设置标志
    only_tsl_flag = true;
}

//  视频流探针槽
void MainWindow::slot_OnVideoProbeFrame(const QVideoFrame& frame)
{
    //  检查
    if(!frame.isValid()) return;

    //  复制一份当前帧的副本
    QVideoFrame cloneFrame(frame);
    cloneFrame.map(QAbstractVideoBuffer::ReadOnly);   //  采用只读方式映射

    //  构造输出帧图像
    QImage img(cloneFrame.width(), cloneFrame.height(), QImage::Format_RGB888);
    unsigned char* in_buf = cloneFrame.bits();        //  视频流原始数据帧缓冲区
    unsigned char* out_buf = img.bits();              //  视频流输出数据帧缓冲区

    //  根据源帧的格式进行转换
    //  这里只处理常见格式
    if(cloneFrame.pixelFormat()==QVideoFrame::Format_YV12)
    {
        int y=0;
        int x=0;
        for(y=0;y<cloneFrame.height();y++)
        {
            for(x=0;x<cloneFrame.width();x++)
            {
                unsigned char Y = static_cast<unsigned char>(in_buf[y*cloneFrame.width() + x]);
                unsigned char V = static_cast<unsigned char>(in_buf[cloneFrame.width() * cloneFrame.height() + (y/2)*(cloneFrame.width()/2) + x/2]);
                unsigned char U = static_cast<unsigned char>(in_buf[cloneFrame.width() * cloneFrame.height() + ((cloneFrame.width()/2) * (cloneFrame.height()/2)) + (y/2)*(cloneFrame.width()/2) + x/2]);

                double R = Y + 1.402 * (V-128);
                double G = Y - 0.344136 * (U-128) - 0.714136 * (V-128);
                double B = Y + 1.772 * (U-128);

                if(R >= 255) R = 255;
                if(R <= 0) R = 0;
                if(G >= 255) G = 255;
                if(G <= 0) G = 0;
                if(B >= 255) B = 255;
                if(B <= 0) B = 0;

                out_buf[y*img.bytesPerLine() + x*3 + 0] = static_cast<unsigned char>(R);
                out_buf[y*img.bytesPerLine() + x*3 + 1] = static_cast<unsigned char>(G);
                out_buf[y*img.bytesPerLine() + x*3 + 2] = static_cast<unsigned char>(B);
            }
        }
    }
    else if(cloneFrame.pixelFormat()==QVideoFrame::Format_YUV420P)
    {
        int y=0;
        int x=0;
        for(y=0;y<cloneFrame.height();y++)
        {
            for(x=0;x<cloneFrame.width();x++)
            {
                unsigned char Y = static_cast<unsigned char>(in_buf[y*cloneFrame.width() + x]);
                unsigned char U = static_cast<unsigned char>(in_buf[cloneFrame.width() * cloneFrame.height() + (y/2)*(cloneFrame.width()/2) + x/2]);
                unsigned char V = static_cast<unsigned char>(in_buf[cloneFrame.width() * cloneFrame.height() + ((cloneFrame.width()/2) * (cloneFrame.height()/2)) + (y/2)*(cloneFrame.width()/2) + x/2]);

                double R = Y + 1.402 * (V-128);
                double G = Y - 0.344136 * (U-128) - 0.714136 * (V-128);
                double B = Y + 1.772 * (U-128);

                if(R >= 255) R = 255;
                if(R <= 0) R = 0;
                if(G >= 255) G = 255;
                if(G <= 0) G = 0;
                if(B >= 255) B = 255;
                if(B <= 0) B = 0;

                out_buf[y*img.bytesPerLine() + x*3 + 0] = static_cast<unsigned char>(R);
                out_buf[y*img.bytesPerLine() + x*3 + 1] = static_cast<unsigned char>(G);
                out_buf[y*img.bytesPerLine() + x*3 + 2] = static_cast<unsigned char>(B);
            }
        }
    }
    else if(cloneFrame.pixelFormat()==QVideoFrame::Format_NV12)
    {
        int y=0;
        int x=0;
        for(y=0;y<cloneFrame.height();y++)
        {
            for(x=0;x<cloneFrame.width();x++)
            {
                unsigned char Y = static_cast<unsigned char>(in_buf[y*cloneFrame.bytesPerLine() + x]);
                unsigned char U = static_cast<unsigned char>(in_buf[cloneFrame.bytesPerLine() * cloneFrame.height() + (y/2)*(cloneFrame.bytesPerLine()) + (x&(~1)) + 0]);
                unsigned char V = static_cast<unsigned char>(in_buf[cloneFrame.bytesPerLine() * cloneFrame.height() + (y/2)*(cloneFrame.bytesPerLine()) + (x&(~1)) + 1]);

                double R = Y + 1.402 * (V-128);
                double G = Y - 0.344136 * (U-128) - 0.714136 * (V-128);
                double B = Y + 1.772 * (U-128);

                if(R >= 255) R = 255;
                if(R <= 0) R = 0;
                if(G >= 255) G = 255;
                if(G <= 0) G = 0;
                if(B >= 255) B = 255;
                if(B <= 0) B = 0;

                out_buf[y*img.bytesPerLine() + x*3 + 0] = static_cast<unsigned char>(R);
                out_buf[y*img.bytesPerLine() + x*3 + 1] = static_cast<unsigned char>(G);
                out_buf[y*img.bytesPerLine() + x*3 + 2] = static_cast<unsigned char>(B);
            }
        }
    }
    else if(cloneFrame.pixelFormat()==QVideoFrame::Format_RGB32)
    {
        int y=0;
        int x=0;
        for(y=0;y<cloneFrame.height();y++)
        {
            for(x=0;x<cloneFrame.width();x++)
            {
                unsigned char R = static_cast<unsigned char>(in_buf[y*cloneFrame.bytesPerLine() + x*4 + 2]);
                unsigned char G = static_cast<unsigned char>(in_buf[y*cloneFrame.bytesPerLine() + x*4 + 1]);
                unsigned char B = static_cast<unsigned char>(in_buf[y*cloneFrame.bytesPerLine() + x*4 + 0]);

                out_buf[y*img.bytesPerLine() + x*3 + 0] = static_cast<unsigned char>(R);
                out_buf[y*img.bytesPerLine() + x*3 + 1] = static_cast<unsigned char>(G);
                out_buf[y*img.bytesPerLine() + x*3 + 2] = static_cast<unsigned char>(B);
            }
        }
    }
    else
    {
        qDebug("Unsupport in_fmt = %d w=%d, h=%d, bl=%d", cloneFrame.pixelFormat(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine());
    }

    //  根据输出控件的尺寸进行缩放
    img = img.scaled(ui->label_outputvideo->width(), ui->label_outputvideo->height(), Qt::KeepAspectRatio);

    //  显示到控件上
    ui->label_outputvideo->setPixmap(QPixmap::fromImage(img));

    //  取消映射
    cloneFrame.unmap();
}
