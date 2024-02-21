/*************************************************************

    程序名称:基于Qt线程类的AI类
    程序版本:REV 0.1
    创建日期:20240220
    设计编写:rainhenry
    作者邮箱:rainhenry@savelife-tech.com
    开源协议:GPL

    版本修订
        REV 0.1   20240220      rainhenry    创建文档

*************************************************************/
//------------------------------------------------------------
//  重定义保护
#ifndef __CQTAI_H__
#define __CQTAI_H__

//------------------------------------------------------------
//  包含头文件
#include <QString>
#include <QVariant>
#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QElapsedTimer>

#include "CPyAI.h"

//------------------------------------------------------------
//  类定义
class CQtAI: public QThread
{
    Q_OBJECT

public:
    //  状态枚举
    typedef enum
    {
        EAISt_Ready = 0,
        EAISt_Busy,
        EAISt_Error
    }EAISt;

    //  命令枚举
    typedef enum
    {
        EAIcmd_Null = 0,
        EAIcmd_ExTranslateCn2En,
        EAIcmd_ExTextToVideo,
        EAIcmd_Release,
    }EAIcmd;

    //  构造和析构函数
    CQtAI();
    ~CQtAI() override;

    //  线程运行本体
    void run() override;

    //  初始化
    void Init(void);

    //  执行一次翻译
    void ExTranslateCn2En(QString prompt);

    //  执行一次文字生成视频
    void ExTextToVideo(QString prompt, int total_frames, QString output_file);

    //  得到当前状态
    EAISt GetStatus(void);

    //  释放资源
    void Release(void);

signals:
    //  环境就绪
    void send_environment_ready(void);

    //  当完成翻译
    void send_translate_cn2en_finish(QString out_text, qint64 run_time_ns);

    //  当完成文字生成视频
    void send_text_to_video_finish(qint64 run_time_ns);

private:
    //  内部私有变量
    QSemaphore sem_cmd;           //  命令信号量
    QMutex cmd_mutex;             //  命令互斥量
    EAISt cur_st;                 //  当前状态
    EAIcmd cur_cmd;               //  当前命令
    CPyAI* py_ai;                 //  Python的AI对象
    QString tsl_prompt;           //  翻译的输入
    QString ttv_prompt;           //  生成视频的输入提示词
    int ttv_total_frames;         //  生成视频的总帧数
    QString ttv_output_file;      //  生成视频的输出视频文件
};

//------------------------------------------------------------
#endif  //  __CQTAI_H__



