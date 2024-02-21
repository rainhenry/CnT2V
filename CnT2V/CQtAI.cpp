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

//  包含头文件
#include "CQtAI.h"

//  构造函数
CQtAI::CQtAI():
    sem_cmd(0)
{
    //  初始化私有数据
    cur_st = EAISt_Ready;
    cur_cmd = EAIcmd_Null;
    ttv_total_frames = 0;

    //  创建Python的Chat对象
    py_ai = new CPyAI();
}

//  析构函数
CQtAI::~CQtAI()
{
    Release();   //  通知线程退出
    this->msleep(1000);
    this->quit();
    this->wait(500);

    delete py_ai;
}

//  初始化
void CQtAI::Init(void)
{
    //  暂时没有
}

void CQtAI::run()
{
    //  初始化python环境
    py_ai->Init();

    //  通知运行环境就绪
    emit send_environment_ready();

    //  现成主循环
    while(1)
    {
        //  获取信号量
        sem_cmd.acquire();

        //  获取当前命令和数据
        cmd_mutex.lock();
        EAIcmd now_cmd = this->cur_cmd;
        std::string tsl_prompt_str = this->tsl_prompt.toStdString();
        std::string ttv_prompt_str = this->ttv_prompt.toStdString();
        int ttv_total_frames_val = this->ttv_total_frames;
        std::string ttv_output_file_str = this->ttv_output_file.toStdString();
        cmd_mutex.unlock();

        //  当为空命令
        if(now_cmd == EAIcmd_Null)
        {
            //  释放CPU
            this->sleep(1);
        }
        //  当为退出命令
        else if(now_cmd == EAIcmd_Release)
        {
            py_ai->Release();
            qDebug("Thread is exit!!");
            return;
        }
        //  当为中文翻译成英文命令
        else if(now_cmd == EAIcmd_ExTranslateCn2En)
        {
            //  设置忙
            cmd_mutex.lock();
            this->cur_st = EAISt_Busy;
            cmd_mutex.unlock();

            //  执行
            QString re_str;
            std::string std_str;
            QElapsedTimer run_time;
            run_time.start();
            std_str = py_ai->Translate_Cn2En_Ex(tsl_prompt_str.c_str());
            qint64 run_time_ns = run_time.nsecsElapsed();

            //  转换字符串格式
            re_str = std_str.c_str();

            //  发出操作完成
            emit send_translate_cn2en_finish(re_str, run_time_ns);

            //  完成处理
            cmd_mutex.lock();
            this->cur_st = EAISt_Ready;
            now_cmd = EAIcmd_Null;
            cmd_mutex.unlock();
        }
        //  当为文字生成视频命令
        else if(now_cmd == EAIcmd_ExTextToVideo)
        {
            //  设置忙
            cmd_mutex.lock();
            this->cur_st = EAISt_Busy;
            cmd_mutex.unlock();

            //  执行
            QElapsedTimer run_time;
            run_time.start();
            py_ai->Text_To_Video(ttv_prompt_str.c_str(),
                                 ttv_total_frames_val,
                                 ttv_output_file_str.c_str()
                                );
            qint64 run_time_ns = run_time.nsecsElapsed();

            //  发出操作完成
            emit send_text_to_video_finish(run_time_ns);

            //  完成处理
            cmd_mutex.lock();
            this->cur_st = EAISt_Ready;
            now_cmd = EAIcmd_Null;
            cmd_mutex.unlock();
        }
        //  非法命令
        else
        {
            //  释放CPU
            QThread::sleep(1);
            qDebug("Unknow cmd code!!");
        }
    }
}

CQtAI::EAISt CQtAI::GetStatus(void)
{
    EAISt re;
    cmd_mutex.lock();
    re = this->cur_st;
    cmd_mutex.unlock();
    return re;
}

//  执行一次翻译
void CQtAI::ExTranslateCn2En(QString prompt)
{
    if(GetStatus() == EAISt_Busy) return;

    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_ExTranslateCn2En;
    this->tsl_prompt = prompt;
    this->cur_st = EAISt_Busy;    //  设置忙
    cmd_mutex.unlock();

    sem_cmd.release();
}

//  执行一次文字生成视频
void CQtAI::ExTextToVideo(QString prompt, int total_frames, QString output_file)
{
    if(GetStatus() == EAISt_Busy) return;

    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_ExTextToVideo;
    this->ttv_prompt = prompt;
    this->ttv_total_frames = total_frames;
    this->ttv_output_file = output_file;
    this->cur_st = EAISt_Busy;    //  设置忙
    cmd_mutex.unlock();

    sem_cmd.release();
}

void CQtAI::Release(void)
{
    cmd_mutex.lock();
    this->cur_cmd = EAIcmd_Release;
    cmd_mutex.unlock();

    sem_cmd.release();
}
