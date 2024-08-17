#include "adc.h"



static uint8_t FLAG_ADC1InitOK = 0;                     // ADC1初始化标记，0：未初始化，1：已初始化



static void delay_us(uint32_t us)                       // 定义一个us延时函数，减少移植时对外部文件依赖;
{
    while (us--)                                        // 注意，本函数是简易延时，非精准延时
        for (uint32_t i = 0; i < 7; i++);               // 72MHz系统时钟下，大约多少个空循环耗时1us
}



/******************************************************************************
 * 函  数： configADC1
 * 功  能： 初始化ADC
 * 参  数：
 * 返回值：
 *
 * 备  注： 1-ADC温度通道的采样时间，芯片手册上推荐为17.1us, 远小于此值时数据出错；
 *
 ******************************************************************************/
static void  configADC1(void)
{
    RCC->APB2ENR  |= 1 << 8;           // ADC1时钟使能
    RCC->APB2RSTR |= 1 << 8;           // ADC1复位
    RCC->APB2RSTR &= ~(1 << 8);        // 复位结束

    ADC->CCR &= ~(3 << 16);            // 分频因子清零
    ADC->CCR |= 1 << 16;               // ADC1时钟设置=ADCCLK=APB2/4=84/4=21Mhz,ADC最大时钟不能超过30M!

    ADC1->CR1  = 0 ;                   // 清0 , CR1设置工作模式
    ADC1->CR1 |= 0 << 8;               // 扫描模式(多个通道)  0:关闭   1：使用
    ADC1->CR1 |= 0 << 24;              // 12位分辨率，15ADCCLK周期

    ADC1->CR2  = 0;                    // 清0, CR2设置工作参数
    ADC1->CR2 |= 0 << 1;               // 连续转换   0：单次转换   1：连续转换
    ADC1->CR2 |= 0 << 11;              // 右对齐
    ADC1->CR2 |= 0 << 28;              // 软件控制，禁止电平触发

    ADC1->SQR1 &= ~(0x0F<<20);         // 清0转换通道总数
    ADC1->SQR1 |= 0 << 20;             // 需要转换的通道数量 易错：数量=N+1

    ADC1->SMPR1 = 0x07FF6DB6;          // 通道18~16为480个采样周期(约23us)，通道15~10为144个采样周期(约7us)
    ADC1->SMPR2 = 0x36DB6DB6;          // 通道 9~ 0为144个采样周期(约7us)

    ADC1->CR2  |= 1 << 0;              // 开启AD转换器

    FLAG_ADC1InitOK = 1;               // 初始化后，标志置1
}



// 初始化所需要的引脚
static void adc_GPIOInit(GPIO_TypeDef *GPIOx, uint32_t PINx)
{   
    // 使能所用引脚端口时钟；使用端口判断的方法使能时钟, 以使代码移植更方便
    if (GPIOx == GPIOA)  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    if (GPIOx == GPIOB)  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    if (GPIOx == GPIOC)  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    if (GPIOx == GPIOD)  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    if (GPIOx == GPIOE)  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;

    // 配置相关引脚为模拟输入模式; ADC1的15个通道，分布在PA、PB、PC的0~7引脚
    if (PINx == GPIO_Pin_0)   GPIOx->MODER |= (0x03<<0*0);  // GPIO_Pin_0，模拟输入
    if (PINx == GPIO_Pin_1)   GPIOx->MODER |= (0x03<<1*2);  // GPIO_Pin_1，模拟输入
    if (PINx == GPIO_Pin_2)   GPIOx->MODER |= (0x03<<2*2);  // GPIO_Pin_2，模拟输入
    if (PINx == GPIO_Pin_3)   GPIOx->MODER |= (0x03<<3*2);  // GPIO_Pin_3，模拟输入
    if (PINx == GPIO_Pin_4)   GPIOx->MODER |= (0x03<<4*2);  // GPIO_Pin_4，模拟输入
    if (PINx == GPIO_Pin_5)   GPIOx->MODER |= (0x03<<5*2);  // GPIO_Pin_5，模拟输入
    if (PINx == GPIO_Pin_6)   GPIOx->MODER |= (0x03<<6*2);  // GPIO_Pin_6，模拟输入
    if (PINx == GPIO_Pin_7)   GPIOx->MODER |= (0x03<<7*2);  // GPIO_Pin_7，模拟输入
}



// 适用于ADC1和2，通过GPIO端口、引脚号，返回通道号
// 返回：正常：0~15，失败：99
static uint8_t adc_PinConvertChannel(GPIO_TypeDef *GPIOx, uint32_t PINx)
{
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_0))    return  0; // PA0 = 通道 0
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_1))    return  1; // PA1 = 通道 1
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_2))    return  2; // PA2 = 通道 2
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_3))    return  3; // PA3 = 通道 3
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_4))    return  4; // PA4 = 通道 4
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_5))    return  5; // PA5 = 通道 5
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_6))    return  6; // PA6 = 通道 6
    if ((GPIOx == GPIOA) && (PINx == GPIO_Pin_7))    return  7; // PA7 = 通道 7
    if ((GPIOx == GPIOB) && (PINx == GPIO_Pin_0))    return  8; // PB0 = 通道 8
    if ((GPIOx == GPIOB) && (PINx == GPIO_Pin_1))    return  9; // PB1 = 通道 9
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_0))    return 10; // PC0 = 通道10
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_1))    return 11; // PC1 = 通道11
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_2))    return 12; // PC2 = 通道12
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_3))    return 13; // PC3 = 通道13
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_4))    return 14; // PC4 = 通道14
    if ((GPIOx == GPIOC) && (PINx == GPIO_Pin_5))    return 15; // PC5 = 通道15
    return 99; // 如果没找到 匹配的参数，就返回错误号
}


// 为移植时清晰代码结构，及调用关系，本函数不作为全局函数
// 对通道进行多次采值，并返回平均值
// 参数：  uint8_t channel: 通道值，可选范围0~17
//         uint8_t times:   采植次数
// 返回值：多次采值后的平均值，值范围：0~4095
static uint16_t getADC1Value(uint8_t channel, uint8_t times)
{
    uint32_t valueSUM = 0;                        // ADC的多次取值累加结果

    if (FLAG_ADC1InitOK == 0)  configADC1();      // 配置ADC1的工作模式

    if (channel == 16 || channel == 17)           // 如果是第16、17通道，则使能TSVREFE位
        ADC->CCR |= 1 << 23;                      // 使能 温度传感器通道、内部参考电压通道

    ADC1->SQR3 = channel;                         // 第1个要转换的通道,内部参考电压转换值通道

    for (u8 t = 0; t < times; t++)                // 多次求值，并累加成结果值
    {
        ADC1->CR2 |= 1 << 30;                     // 启动规则转换通道
        while (!(ADC1->SR & 1 << 1));             // 等待本次转换结束
        valueSUM = valueSUM + ADC1->DR;           // 叠加本次采值
        delay_us(5);                              // 稍为延时一下
    }

    return valueSUM / times;                      // 返回平均值
}



/******************************************************************************
 * 函  数： ADC1_GetInternalTemperature
 * 功  能： 获取芯片内部温度值，即ADC1通道16
 *          本函数，直接调用即可输出结果，无需提前做ADC初始化
 * 参  数：
 * 返回值： float 摄氏度值
 *
 * 备  注： 1-此函数，直接调用即可，无需在外部对ADC、GPIO初始化; 将返回结果温度值，
 *          2-程序中第1次调用此函数，函数内工作耗时约2ms，因为函数要对ADC作初始化;
 *          3-第2次起，调用此函数，每次耗时约: 19us*5次 = 100us左右;
 *          4-如系统实时性要求较高，请改用中断、DMA;；
 *          5-温度值范围：-40~125度，精度：± 1.5℃左右
 *
 ******************************************************************************/
float  ADC1_GetInternalTemperature(void)
{
    uint16_t adcValue;
    float TEMP = 0.0;

    adcValue = getADC1Value(16, 10);;        // 返回多次采值后的平均值
    TEMP = (float)adcValue * (3.3 / 4096);   // 求电压
    TEMP = (TEMP-0.76f) / 0.0025f + 25;      // 根据公式，将电压转换成摄氏底
    return TEMP;
}



/******************************************************************************
 * 函  数： ADC1_GetVoltage
 * 功  能： 获取通道引脚上的电压值
 *
 * 参  数： GPIO_TypeDef*  GPIOx   GPIO端口, 可选参数：GPIOA ~ GPIOG
 *          uint32_t       PINx    引脚，    可选参数：GPIO_Pin_0 ~ GPIO_Pin_15
 *
 * 返回值： flloat 电压值，范围：0~3.3V
 *
 * 备  注： 1-此函数，直接调用即可，无需在外部对ADC、GPIO初始化; 将返回结果电压值，
 *          2-程序中第1次调用此函数，函数内工作耗时约2ms，因为函数要对ADC作初始化;
 *          3-第2次起，调用此函数，每次耗时约: 25us;
 *          4-如系统实时性要求较高，请改用中断、DMA;；
 *
 ******************************************************************************/
float ADC1_GetVoltage(GPIO_TypeDef *GPIOx, uint32_t PINx)
{
    static uint8_t  channel  = 0;                 // 通道号
    static uint16_t adcValue = 0;                 // ADC采集后的平均值，用于计算电压值

    channel = adc_PinConvertChannel(GPIOx, PINx); // 根据引脚，判断所用ADC1的通道
    if (channel == 99)  return 0;                 // 所输入的引脚号有误，退出并返回0

    adc_GPIOInit(GPIOx, PINx);                    // 初始化引脚为模拟输入模式
    adcValue = getADC1Value(channel, 3);          // 多次采值后的, 返回的平均值

    return (float)adcValue / 1241.2f;             // 返回电压值; 即 adcValue*(3.3/4096)的简化值;
}




