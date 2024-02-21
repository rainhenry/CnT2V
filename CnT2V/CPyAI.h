/*************************************************************

    程序名称:基于Python3原生C接口的AI C++类(阻塞)
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
#ifndef __CPYAI_H__
#define __CPYAI_H__

//------------------------------------------------------------
//  包含头文件
#include <string>
#include <list>

//------------------------------------------------------------
//  类定义
class CPyAI
{
public:
    //  构造与析构函数
    CPyAI();
    ~CPyAI();

    //  释放资源
    //  注意！该释放必须和执行本体在同一线程中！
    void Release(void);

    //  初始化
    //  注意！该初始化必须和执行本体在同一线程中！
    void Init(void);

    //  执行中文到英文的翻译
    std::string Translate_Cn2En_Ex(const char* prompt);

    //  执行英文文本到视频文件的生成
    void Text_To_Video(const char* prompt, int total_frames, const char* output_file);

private:
    //  为了兼容Python C的原生API，独立封装numpy的C初始化接口
    int import_array_init(void);

    //  静态python环境初始化标志
    static bool Py_Initialize_flag;

    //  翻译相关私有数据
    void* py_tsl_module;                 //  模块
    void* py_tsl_model_init;             //  模型初始化
    void* py_tsl_tokenizer_init;         //  分词器初始化
    void* py_tsl_model_handle;           //  模型句柄
    void* py_tsl_tokenizer_handle;       //  分词器句柄
    void* py_tsl_ex;                     //  执行一次翻译

    //  文字生成视频相关私有数据
    void* py_ttv_module;                 //  模块
    void* py_ttv_pipe_init;              //  管道初始化
    void* py_ttv_pipe_handle;            //  管道句柄
    void* py_ttv_ex;                     //  执行一次文字生成视频

};


//------------------------------------------------------------
#endif  //  __CPYAI_H__


