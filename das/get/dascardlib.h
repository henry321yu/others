#ifndef DASCARDLIB_H
#define DASCARDLIB_H

#ifdef __cplusplus
    extern "C" {
#endif
    int __declspec(dllexport) DasCardOpen();            //打开采集卡
    void __declspec(dllexport) DasCardClose();          //关闭采集卡
    int __declspec(dllexport) DasCardStart();           //开始采集
    int __declspec(dllexport) DasCardStop();            //停止采集
    void __declspec(dllexport) DasCardSetDemodThreadNum(int num);     //设置参与计算的线程数目，根据CPU核心数及性能合理选择，对于12带i5以上cpu通常6个线程即可,DasCardStart前调用
    void __declspec(dllexport) DasCardSetDemodGauge(int width);       //设置解调标距，即空间分辨率，乘以采样分辨率得到实际米数，DasCardStart前调用
    int __declspec(dllexport) DasCardSetPulseNum(int num);           //设置一次读取数据的脉冲数目
    int __declspec(dllexport) DasCardSetPulseFrq(int frq);           //设置脉冲触发频率，内部触发有效
    int __declspec(dllexport) DasCardSetSampleNum(int num);          //设置每次触发采样长度，必须在DasCardSetResolution后调用，成对调用
    int __declspec(dllexport) DasCardSetPulseWidth(int width);       //设置脉冲宽度，ns
    int __declspec(dllexport) DasCardSetDataSel(int sel);            //设置上传数据类型，1：双通道原始数据，2：单通道幅度相位数据，3：双通道幅度相位数据
    int __declspec(dllexport) DasCardSetDemodChannelNum(int num);           //设置解调通道数
    int __declspec(dllexport) DasCardReadFifo(char *data, int len);     //读缓存数据,数据类型为int16，对于相位数据需要除以256转换成double即为最终的弧度数据
    int __declspec(dllexport) DasCardQueryFifo();                       //查询缓存空间
    void __declspec(dllexport) DasCardCacheClear();                     //清除缓存
    int __declspec(dllexport) DasCardSetUnwrap(int flag);               //设置是否解缠绕
    int __declspec(dllexport) DasCardSetOffsetVoltage(int offset);      //设置偏置电压 ADC值
    int __declspec(dllexport) DasCardSetTrigerSoure(int trig);          //设置触发源，0：内部触发，1：外部触发
    int __declspec(dllexport) DasCardSetDelay(int num);                 //设置延迟采样点
    int __declspec(dllexport) DasCardSetResolution(int id);             //设置采样分辨率，1：0.4m，2：0.8m，3：1.2m，4：1.6m，5：2m, 必须在DasCardSetSampleNum前调用
#ifdef __cplusplus
    }
#endif

#endif // DASCARDLIB_H
