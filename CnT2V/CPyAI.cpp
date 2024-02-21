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

//  包含头文件
#include <stdio.h>
#include <stdlib.h>

#include "CPyAI.h"

#include <Python.h>
#include <numpy/arrayobject.h>
#include <numpy/ndarrayobject.h>
#include <numpy/npy_3kcompat.h>

//  初始化全局变量
bool CPyAI::Py_Initialize_flag = false;

//  构造函数
CPyAI::CPyAI()
{
    //  当没有初始化过
    if(!Py_Initialize_flag)
    {
        if(!Py_IsInitialized())
        {
            Py_Initialize();
            import_array_init();
        }
        Py_Initialize_flag = true;   //  标记已经初始化
    }

    //  初始化翻译相关私有数据
    py_tsl_module           = nullptr;
    py_tsl_model_init       = nullptr;
    py_tsl_tokenizer_init   = nullptr;
    py_tsl_model_handle     = nullptr;
    py_tsl_tokenizer_handle = nullptr;
    py_tsl_ex               = nullptr;

    //  初始化文字生成视频相关私有数据
    py_ttv_module           = nullptr;  
    py_ttv_pipe_init        = nullptr;   
    py_ttv_pipe_handle      = nullptr;
    py_ttv_ex               = nullptr;
}

//  析构函数
CPyAI::~CPyAI()
{
    //  此处不可以调用Release，因为Python环境实际运行所在线程
    //  不一定和构造该类对象是同一个线程
}

//  释放资源
//  注意！该释放必须和执行本体在同一线程中！
void CPyAI::Release(void)
{
    if(py_ttv_ex != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_ex));
    }
    if(py_ttv_pipe_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_pipe_handle));
    }
    if(py_ttv_pipe_init != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_pipe_init));
    }
    if(py_ttv_module != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_ttv_module));
    }

    if(py_tsl_ex != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_ex));
    }
    if(py_tsl_tokenizer_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_tokenizer_handle));
    }
    if(py_tsl_model_handle != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_model_handle));
    }
    if(py_tsl_tokenizer_init != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_tokenizer_init));
    }
    if(py_tsl_model_init != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_model_init));
    }
    if(py_tsl_module != nullptr)
    {
        Py_DecRef(static_cast<PyObject*>(py_tsl_module));
    }

    if(Py_Initialize_flag)
    {
        Py_Finalize();
        Py_Initialize_flag = false;   //  标记未初始化
    }
}

//  为了兼容Python C的原生API，独立封装numpy的C初始化接口
int CPyAI::import_array_init(void)
{
    import_array()
    return 0;
}

//  初始化
//  注意！该初始化必须和执行本体在同一线程中！
void CPyAI::Init(void)
{
    //  开启Python线程支持
    PyEval_InitThreads();
    PyEval_SaveThread();

    //  检测当前线程是否拥有GIL
    int ck = PyGILState_Check() ;
    if (!ck)
    {
        PyGILState_Ensure(); //  如果没有GIL，则申请获取GIL
    }

    //  构造基本Python环境
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('../CnT2V')");

    //  载入tslcte.py文件
    py_tsl_module = static_cast<void*>(PyImport_ImportModule("tslcte"));

    //  检查是否成功
    if(py_tsl_module == nullptr)
    {
        printf("[Error] py_tsl_module == null!!");
        return;
    }

    //  初始化模型
    py_tsl_model_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_tsl_module), "model_init"));
    PyObject* py_tsl_args = PyTuple_New(1);
    PyTuple_SetItem(py_tsl_args, 0, Py_BuildValue("s", "../opus-mt-zh-en"));
    py_tsl_model_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_tsl_model_init), py_tsl_args));

    if(py_tsl_model_handle == nullptr)
    {
        printf("[Error] py_tsl_model_handle == null!!");
        return;
    }

    //  初始化分词器
    py_tsl_tokenizer_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_tsl_module), "tokenizer_init"));
    py_tsl_args = PyTuple_New(1);
    PyTuple_SetItem(py_tsl_args, 0, Py_BuildValue("s", "../opus-mt-zh-en"));
    py_tsl_tokenizer_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_tsl_tokenizer_init), py_tsl_args));

    if(py_tsl_tokenizer_handle == nullptr)
    {
        printf("[Error] py_tsl_tokenizer_handle == null!!");
        return;
    }

    //  载入执行翻译本体
    py_tsl_ex = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_tsl_module), "translate_c2e"));

    //  载入ttv.py文件
    py_ttv_module = static_cast<void*>(PyImport_ImportModule("ttv"));

    //  检查是否成功
    if(py_ttv_module == nullptr)
    {
        printf("[Error] py_ttv_module == null!!");
        return;
    }

    //  初始化管道
    py_ttv_pipe_init   = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_ttv_module), "pipe_init"));
    PyObject* py_ttv_args = PyTuple_New(1);
    PyTuple_SetItem(py_ttv_args, 0, Py_BuildValue("s", "../text-to-video-ms-1.7b"));
    py_ttv_pipe_handle = static_cast<void*>(PyObject_CallObject(static_cast<PyObject*>(py_ttv_pipe_init), py_ttv_args));

    if(py_ttv_pipe_handle == nullptr)
    {
        printf("[Error] py_ttv_pipe_handle == null!!");
        return;
    }

    //  载入执行文字生成视频本体
    py_ttv_ex = static_cast<void*>(PyObject_GetAttrString(static_cast<PyObject*>(py_ttv_module), "text_to_video"));
}

//  执行中文到英文的翻译
std::string CPyAI::Translate_Cn2En_Ex(const char* prompt)
{
    //  返回字符串
    std::string re_str;

    //  构造输入数据
    PyObject* py_args = PyTuple_New(3);

    //  第一个参数为关键字
    PyObject* py_prompt = Py_BuildValue("s", prompt);
    PyTuple_SetItem(py_args, 0, py_prompt);

    //  第二个参数为模型句柄
    PyTuple_SetItem(py_args, 1, static_cast<PyObject*>(py_tsl_model_handle));

    //  第三个参数为分词器句柄
    PyTuple_SetItem(py_args, 2, static_cast<PyObject*>(py_tsl_tokenizer_handle));

    //  执行
    PyObject* py_ret = PyObject_CallObject(static_cast<PyObject*>(py_tsl_ex), py_args);

    //  检查
    if(py_ret == nullptr)
    {
        printf("py_ret == nullptr\n");
        return re_str;
    }

    //  拿到返回字符串
    const char* tmp_str = PyUnicode_AsUTF8(py_ret);

    //  检查字符串
    if(tmp_str == nullptr)
    {
        printf("tmp_str == nullptr\n");
        return re_str;
    }

    //  赋值
    re_str = tmp_str;

    //  释放资源
    Py_DecRef(py_ret);
    Py_DecRef(py_prompt);
    //Py_DecRef(py_args);    //  由于其中包含了模型句柄,所以不能释放

    //  操作完成
    return re_str;
}

//  执行英文文本到视频文件的生成
void CPyAI::Text_To_Video(const char* prompt, int total_frames, const char* output_file)
{
    //  构造输入数据
    PyObject* py_args = PyTuple_New(4);

    //  第一个参数为关键字
    PyObject* py_prompt = Py_BuildValue("s", prompt);
    PyTuple_SetItem(py_args, 0, py_prompt);

    //  第二个参数为总帧数
    PyObject* py_totoal_frames = Py_BuildValue("i", total_frames);
    PyTuple_SetItem(py_args, 1, py_totoal_frames);


    //  第三个参数为输出文件
    PyObject* py_output_file = Py_BuildValue("s", output_file);
    PyTuple_SetItem(py_args, 2, py_output_file);

    //  第四个参数为管道句柄
    PyTuple_SetItem(py_args, 3, static_cast<PyObject*>(py_ttv_pipe_handle));

    //  执行
    PyObject* py_ret = PyObject_CallObject(static_cast<PyObject*>(py_ttv_ex), py_args);

    //  检查
    if(py_ret == nullptr)
    {
        printf("py_ret == nullptr\n");
        return;
    }

    //  释放资源
    Py_DecRef(py_ret);
    Py_DecRef(py_output_file);
    Py_DecRef(py_totoal_frames);
    Py_DecRef(py_prompt);
    //Py_DecRef(py_args);    //  由于其中包含了模型句柄,所以不能释放

    //  操作完成
    return;
}

