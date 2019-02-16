//----------------------------------------------------------------------------------------------------
//библиотеки
//----------------------------------------------------------------------------------------------------

#include "sensor.h"
#include "based.h"

//----------------------------------------------------------------------------------------------------
//перечисления
//----------------------------------------------------------------------------------------------------

//тип блока
typedef enum 
{
 BLOCK_TYPE_UNKNOW,//неизвестный блок
 BLOCK_TYPE_DIVIDER,//разделитель (всегда высокого уровня)
 BLOCK_TYPE_SYNCHRO,//синхросигнал
 BLOCK_TYPE_ONE,//единица
 BLOCK_TYPE_ZERO//ноль
} BLOCK_TYPE;

//режим декодирования
typedef enum 
{
 MODE_WAIT_SYNCHRO,//ожидание синхросигнала
 MODE_WAIT_ZERO_FIRST,//ожидание первого нуля
 MODE_WAIT_ZERO_SECOND,//ожидание второго нуля
 MODE_RECEIVE_DATA//приём данных
} MODE;
 
//----------------------------------------------------------------------------------------------------
//глобальные переменные
//----------------------------------------------------------------------------------------------------
static const uint16_t MAX_TIMER_INTERVAL_VALUE=0xFFFF;//максимальное значение интервала таймера
static const uint16_t MAX_TIMER_ENABLED_COUNTER_INTERVAL_VALUE=0xFF;//максимальное значение интервала таймера наличия данных
static const uint32_t TIMER_FREQUENCY_HZ=31250UL;//частота таймера
static const uint32_t TIMER_ENABLED_COUNTER_FREQUENCY_HZ=31250UL;//частота таймера наличия данных
static volatile bool TimerOverflow=false;//было ли переполнение таймера

static uint8_t Buffer[20];//буфер сборки полубайта
static uint8_t BitSize=0;//количество принятых бит
static uint8_t Byte=0;//собираемый байт
static volatile int8_t CurrentTemp=0;//температура
static volatile int8_t CurrentHumidity=0;//влажность
static volatile uint32_t EnabledDataCounter=0;//счётчик наличия сигнала

static volatile uint8_t LastBuffer[3][10];//последние принятые данные

//----------------------------------------------------------------------------------------------------
//макроопределения
//----------------------------------------------------------------------------------------------------

//максимальное значение счётчика наличия сигнала датчика
#define MAX_ENABLED_DATA_COUNTER ((TIMER_ENABLED_COUNTER_FREQUENCY_HZ*60*10)/MAX_TIMER_ENABLED_COUNTER_INTERVAL_VALUE)

//----------------------------------------------------------------------------------------------------
//прототипы функций
//----------------------------------------------------------------------------------------------------

static void SENSOR_SetValue(int8_t temp,int8_t humidity);//задать показания
static void SENSOR_SetTimerOverflow(void);//установить флаг переполнения таймера
static void SENSOR_ResetTimerOverflow(void);//сбросить флаг переполнения таймера
static bool SENSOR_IsTimerOverflow(void);//получить, есть ли переполнение таймера
static uint16_t SENSOR_GetTimerValue(void);//получить значение таймера
static void SENSOR_ResetTimerValue(void);//сбросить значение таймера 
static BLOCK_TYPE SENSOR_GetBlockType(uint32_t counter,bool value);//получить тип блока
static void SENSOR_AddBit(bool state);//добавить бит данных
static void SENSOR_ResetData(void);//начать сборку данных заново
static void SENSOR_AnalizeCounter(uint32_t counter,bool value,MODE *mode);//анализ блока

//----------------------------------------------------------------------------------------------------
//глобальные переменные
//----------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//функции
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//----------------------------------------------------------------------------------------------------
//инициализация
//----------------------------------------------------------------------------------------------------
void SENSOR_Init(void)
{
 //настраиваем аналоговый компаратор
 ACSR=(0<<ACD)|(1<<ACBG)|(0<<ACO)|(0<<ACI)|(1<<ACIE)|(0<<ACIC)|(0<<ACIS1)|(0<<ACIS0);
 //ACD - включение компаратора (0 - ВКЛЮЧЁН!)
 //ACBG - подключение к неинвертрирующему входу компаратора внутрреннего ИОН'а
 //ACO - результат сравнения (выход компаратора)
 //ACI - флаг прерывания от компаратора
 //ACIE - разрешение прерываний от компаратора
 //ACIC - подключение компаратора к схеме захвата таймера T1
 //ACIS1,ACID0 - условие генерации прерывания от компаратора
  
 //настраиваем таймер T1 на частоту 31250 Гц
 TCCR1A=(0<<WGM11)|(0<<WGM10)|(0<<COM1A1)|(0<<COM1A0)|(0<<COM1B1)|(0<<COM1B0);
 //COM1A1-COM1A0 - состояние вывода OC1A
 //COM1B1-COM1B0 - состояние вывода OC1B 
 //WGM11-WGM10 - режим работы таймера
 TCCR1B=(0<<WGM13)|(0<<WGM12)|(1<<CS12)|(0<<CS11)|(0<<CS10)|(0<<ICES1)|(0<<ICNC1); 
 //WGM13-WGM12 - режим работы таймера
 //CS12-CS10 - управление тактовым сигналом (выбран режим деления тактовых импульсов на 256 (частота таймера 31250 Гц))
 //ICNC1 - управление схемой подавления помех блока захвата
 //ICES1 - выбор активного фронта сигнала захвата
 TCNT1=0;//начальное значение таймера
 TIMSK|=(1<<TOIE1);//прерывание по переполнению таймера (таймер T1 шестнадцатибитный) 
 
 EnabledDataCounter=0;
 
 for(uint8_t n=0;n<3;n++)
 {
  for(uint8_t m=0;m<10;m++) LastBuffer[n][m]=0; 
 }
}

//----------------------------------------------------------------------------------------------------
//получить показания
//----------------------------------------------------------------------------------------------------
bool SENSOR_GetValue(int8_t *temp,int8_t *humidity)
{
 cli();
 *temp=CurrentTemp;
 *humidity=CurrentHumidity;
 bool ret=false;
 if (EnabledDataCounter>0) ret=true;
 sei();
 return(ret);
}

//----------------------------------------------------------------------------------------------------
//задать показания
//----------------------------------------------------------------------------------------------------
void SENSOR_SetValue(int8_t temp,int8_t humidity)
{
 cli(); 
 CurrentTemp=temp;
 CurrentHumidity=humidity;
 EnabledDataCounter=MAX_ENABLED_DATA_COUNTER;
 sei();
}

//----------------------------------------------------------------------------------------------------
//уменьшить счётчик наличия данных
//----------------------------------------------------------------------------------------------------
void SENSOR_DecrementEnabledDataCounter(void)
{
 cli();
 if (EnabledDataCounter>0) EnabledDataCounter--;
 sei();
}

//----------------------------------------------------------------------------------------------------
//установить флаг переполнения таймера
//----------------------------------------------------------------------------------------------------
void SENSOR_SetTimerOverflow(void)
{
 cli();
 TimerOverflow=true;
 sei();
}
//----------------------------------------------------------------------------------------------------
//сбросить флаг переполнения таймера
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetTimerOverflow(void)
{
 cli();
 TimerOverflow=false;
 sei();
}
//----------------------------------------------------------------------------------------------------
//получить, есть ли переполнение таймера
//----------------------------------------------------------------------------------------------------
bool SENSOR_IsTimerOverflow(void)
{
 cli();
 bool ret=TimerOverflow;
 sei();
 return(ret);
}

//----------------------------------------------------------------------------------------------------
//получить значение таймера 
//----------------------------------------------------------------------------------------------------
uint16_t SENSOR_GetTimerValue(void)
{
 cli();
 uint16_t ret=TCNT1;
 sei(); 
 return(ret);
} 


//----------------------------------------------------------------------------------------------------
//сбросить значение таймера 
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetTimerValue(void)
{
 cli();
 TCNT1=0;
 sei();
 SENSOR_ResetTimerOverflow();
} 
//----------------------------------------------------------------------------------------------------
//получить тип блока
//----------------------------------------------------------------------------------------------------
BLOCK_TYPE SENSOR_GetBlockType(uint32_t counter,bool value)
{ 
 static const uint32_t DIVIDER_MIN=(TIMER_FREQUENCY_HZ*(5))/44100UL;
 static const uint32_t DIVIDER_MAX=(TIMER_FREQUENCY_HZ*(35))/44100UL;
 static const uint32_t ZERO_MIN=(TIMER_FREQUENCY_HZ*(80-25))/44100UL;
 static const uint32_t ZERO_MAX=(TIMER_FREQUENCY_HZ*(100+25))/44100UL;
 static const uint32_t ONE_MIN=(TIMER_FREQUENCY_HZ*(160-25))/44100UL;
 static const uint32_t ONE_MAX=(TIMER_FREQUENCY_HZ*(200+25))/44100UL;
 static const uint32_t SYNCHRO_MIN=(TIMER_FREQUENCY_HZ*(320-25))/44100UL;
 static const uint32_t SYNCHRO_MAX=(TIMER_FREQUENCY_HZ*(400+25))/44100UL;
 

 if (counter>DIVIDER_MIN && counter<DIVIDER_MAX) return(BLOCK_TYPE_DIVIDER);//разделитель
 if (counter>ZERO_MIN && counter<ZERO_MAX) return(BLOCK_TYPE_ZERO);//ноль
 if (counter>ONE_MIN && counter<ONE_MAX) return(BLOCK_TYPE_ONE);//один
 if (counter>SYNCHRO_MIN && counter<SYNCHRO_MAX) return(BLOCK_TYPE_SYNCHRO);//синхросигнал
 return(BLOCK_TYPE_UNKNOW);//неизвестный блок
}
//----------------------------------------------------------------------------------------------------
//добавить бит данных
//----------------------------------------------------------------------------------------------------
void SENSOR_AddBit(bool state)
{
 if ((BitSize>>2)>=19) return;//буфер заполнен
 Byte<<=1;
 if (state==true) Byte|=1;
 BitSize++; 
 if ((BitSize&0x03)==0)
 {
  Buffer[(BitSize>>2)-1]=Byte;
  Byte=0;
 }
}
//----------------------------------------------------------------------------------------------------
//начать сборку данных заново
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetData(void)
{
 BitSize=0;
 Byte=0;
}

//----------------------------------------------------------------------------------------------------
//анализ блока
//----------------------------------------------------------------------------------------------------
void SENSOR_AnalizeCounter(uint32_t counter,bool value,MODE *mode)
{
 //узнаем тип блока
 BLOCK_TYPE type=SENSOR_GetBlockType(counter,value);

 if (type==BLOCK_TYPE_UNKNOW)
 {
  *mode=MODE_WAIT_SYNCHRO;
  SENSOR_ResetData();
  return;
 } 
 if (type==BLOCK_TYPE_DIVIDER) return;//разделитель бесполезен для анализа 
 //посылка должна начинаться и завершаться синхросигналом
 if ((*mode)==MODE_WAIT_SYNCHRO)//ждём синхросигнала
 {
  if (type==BLOCK_TYPE_SYNCHRO)
  {
   *mode=MODE_WAIT_ZERO_FIRST;
   return;
  }
  *mode=MODE_WAIT_SYNCHRO;
  SENSOR_ResetData();
  return;
 }
 if ((*mode)==MODE_WAIT_ZERO_FIRST || (*mode)==MODE_WAIT_ZERO_SECOND)//ждём два нуля
 {
  if (type==BLOCK_TYPE_SYNCHRO && (*mode)==MODE_WAIT_ZERO_FIRST) return;//продолжается синхросигнал
  if (type==BLOCK_TYPE_ZERO && (*mode)==MODE_WAIT_ZERO_FIRST)
  {
   *mode=MODE_WAIT_ZERO_SECOND;
   return;
  }
  if (type==BLOCK_TYPE_ZERO && (*mode)==MODE_WAIT_ZERO_SECOND)
  {
   *mode=MODE_RECEIVE_DATA;
   return;
  }
  *mode=MODE_WAIT_SYNCHRO;
  SENSOR_ResetData();
  return;
 }
 //принимаем данные
 if (type==BLOCK_TYPE_SYNCHRO)//передача завершена
 {
  uint8_t size=(BitSize>>2);
  if  (size!=10)
  {
   mode=MODE_WAIT_SYNCHRO;
   SENSOR_ResetData();
   return; 
  }
  //изменяем данные в буфере последних принятых данных
  for(uint8_t n=1;n<3;n++)
  {
   for(uint8_t m=0;m<10;m++) LastBuffer[n-1][m]=LastBuffer[n][m];
  }
  for(uint8_t m=0;m<10;m++) LastBuffer[2][m]=Buffer[m];
  //последние три раза должна быть принята одинаковая информация  
  for(uint8_t m=0;m<10;m++) 
  {
   if (LastBuffer[0][m]!=LastBuffer[1][m] || LastBuffer[0][m]!=LastBuffer[2][m])
   {
    *mode=MODE_WAIT_SYNCHRO;
    SENSOR_ResetData();
    return;   
   }
  }  
  
  uint8_t channel=Buffer[2]&0x03;
  uint8_t key=(Buffer[8]>>3)&0x01;
  uint8_t humidity=(Buffer[7]<<4)|(Buffer[6]);//влажность
  int16_t temp=(Buffer[5]<<8)|(Buffer[4]<<4)|(Buffer[3]);//температура в десятых градуса Фаренгейта со смещением на 50 Фаренгейтов
  //переводим в Цельсии умноженные на 10.
  temp=(temp-320)*5/9-500;
  //округляем
  int16_t r=temp;
  if (r<0) r=-r;
  r%=10;
  if (r>=5)
  {
   if (temp>0) temp+=10;
          else temp-=10;    
  }    
  SENSOR_SetValue((int8_t)(temp/10),humidity);
  
  *mode=MODE_WAIT_SYNCHRO;
  SENSOR_ResetData();
  return;
 }
 //приём данных
 if (type==BLOCK_TYPE_ONE)
 {
  SENSOR_AddBit(true);
  return;
 }
 if (type==BLOCK_TYPE_ZERO)
 {
  SENSOR_AddBit(false);
  return;
 }
 *mode=MODE_WAIT_SYNCHRO;
 SENSOR_ResetData();
}


 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//обработчики векторов прерываний
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
//----------------------------------------------------------------------------------------------------
//обработчик вектора прерывания таймера T1 (16-ми разрядный таймер) по переполнению
//----------------------------------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{   
 SENSOR_SetTimerOverflow(); 
} 
 
//----------------------------------------------------------------------------------------------------
//обработчик вектора прерывания от компаратора
//----------------------------------------------------------------------------------------------------
ISR(ANA_COMP_vect)
{
 ACSR&=0xFF^(1<<ACIE);//запрещаем прерывания
 ACSR|=(1<<ACI);//сбрасываем флаг прерывания компаратора
 
 static MODE mode=MODE_WAIT_SYNCHRO;
 
 //узнаём длительность интервала
 uint16_t length=SENSOR_GetTimerValue();
 if (SENSOR_IsTimerOverflow()==true) length=MAX_TIMER_INTERVAL_VALUE;//было переполнение, считаем интервал максимальным
 SENSOR_ResetTimerValue();
 //отправляем на анализ
 bool value=true;
 if (ACSR&(1<<ACO)) value=false;
 SENSOR_AnalizeCounter(length,value,&mode);
 ACSR|=(1<<ACIE);//разрешаем прерывания
}
