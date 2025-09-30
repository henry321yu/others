#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <dascardlib.h>

int run_flag = 1;
int save_flag = 0;

// control+C exit
void sigint_handler(int signum) {
    printf("Received SIGINT signal. Exiting...\n");
    run_flag = 0;
}

int getLength(const short *amp_data, int len){
    int length = 0, pos;
    double sd = 0.0, th, mean = 0.0;
    int noiseNum = 100; //用于评估阈值的噪声点数

    //计算末端noiseNum个噪声点的均值
    double sum = 0.0;
    for (int i = len-noiseNum; i < len; i++) {
        sum += amp_data[i];
    }
    mean = sum / noiseNum;

    //计算末端noiseNum个噪声点的标准差
    double variance = 0.0;
    for (int i = len-noiseNum; i < len; i++) {
        variance += pow(amp_data[i] - mean, 2);
    }
    variance /= noiseNum;
    sd = sqrt(variance);

    //计算判决阈值,取10倍标准差，可以调节倍数调节阈值
    th = mean + 10*sd;

    //从末端noiseNum个点往前搜寻
    for(pos=len-noiseNum; pos>0; pos--){
        if(amp_data[pos]>th){
            length = pos;
            break;
        }
    }

    return length;
}


int main()
{
    int ret = 0;
    int pulseNum = 100;
    int pulseWidth = 100;
    int pulseFrq = 10000;
    int samples = 4096;
    int dataSel = 2;

   if (signal(SIGINT, sigint_handler) == SIG_ERR) {
       perror("signal");
       return 1;
   }

    ret = DasCardOpen();
    if(ret){
        printf("open device failed!\n");
    }
    else
    {
        printf("open device successfull!\n");
    }

    //set thread number
    DasCardSetDemodThreadNum(8);

    //set demodulation gauge
    DasCardSetDemodGauge(10);

    ret = DasCardSetResolution(5);
    printf("ret:%d, DasCardSetResolution\n",ret);

    ret = DasCardSetDataSel(dataSel);
    printf("ret:%d, DasCardSetDataSel\n",ret);

    ret = DasCardSetPulseWidth(pulseWidth);
    printf("ret:%d, DasCardSetPulseWidth\n",ret);

    ret = DasCardSetPulseFrq(pulseFrq);
    printf("ret:%d, DasCardSetPulseFrq\n",ret);

    ret = DasCardSetPulseNum(pulseNum);
    printf("ret:%d, DasCardSetPulseNum\n",ret);

    ret = DasCardSetSampleNum(samples);
    printf("ret:%d, DasCardSetSampleNum\n",ret);



    ret = DasCardStart();
    if(ret){
        printf("start DAQ failed!\n");
    }
    else
    {
        printf("start DAQ successfull!\n");
    }

    int readSize = samples*pulseNum*sizeof (short)*2;
    short *buf = (short *)malloc(readSize);
    short *ampData = (short *)malloc(readSize/2);
    short *phaseData = (short *)malloc(readSize/2);
    int fibLenth = 0;

    int size;
    int timecount = 0;
    int lasttime = 0;

    FILE *file = fopen("data.bin", "wb");
    if (file == NULL) {
        perror("open file failed");
        return 1;
    }

    while(run_flag){
        size = DasCardQueryFifo();
        if(size>=readSize){
            ret = DasCardReadFifo(buf, readSize);

            if(ret){
                //process data
                for(int i=0;i<samples*pulseNum;i++){
                    ampData[i] = buf[2*i];
                    phaseData[i] = buf[2*i+1];
                }

                //计算光缆长度，实际距离需要乘以分辨率，例如分辨率2m，fibLength=500，光纤长度为500*2=1000m
                fibLenth = getLength(ampData, samples);

                //save data
                if(save_flag){
                    fwrite(buf, 1, readSize, file);
                }

                //statistics pulse frequency
                timecount++;
                if(timecount>=400){
                    int currenttime = GetTickCount();
                    printf( "pusle frequency read:%f\n" , 400.0*pulseNum/((currenttime -lasttime)/1000.0));
                    printf("fiber length: %d\n", fibLenth);
                    timecount = 0;
                    lasttime = currenttime;
                }
            }
        }
        else{
            usleep(10);
        }


    }

    fclose(file);


    ret = DasCardStop();
    if(ret){
        printf("stop DAQ failed!");
    }
    else
    {
        printf("stop DAQ successfull!");
    }

    DasCardClose();


    free(buf);
    free(phaseData);
    free(ampData);

    printf("exit!\n");

    return 0;
}
