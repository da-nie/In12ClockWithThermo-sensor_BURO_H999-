//----------------------------------------------------------------------------------------------------
//библиотеки
//----------------------------------------------------------------------------------------------------
#include "based.h"
#include "output.h"
#include "sensor.h"
#include "rtc.h"
#include "keyboard.h"

//----------------------------------------------------------------------------------------------------
//прототипы функций
//----------------------------------------------------------------------------------------------------
void InitAVR(void);//инициализация контроллера
void ModeClockHourMin(void);//режим отображения часов и минут
void ModeClockMinSec(void);//режим отображения минут и секунд
void ModeTemperatureHumidity(void);//режим отображения влажности и температуры

//----------------------------------------------------------------------------------------------------
//глобальные переменные
//----------------------------------------------------------------------------------------------------
volatile uint8_t Digit[4]={0,0,0,0};//текущие отображаемые цифры
volatile uint8_t NewDigit[4]={0,0,0,0};//новые отображаемые цифры
volatile uint8_t ChangeDigitCounter[4]={0,0,0,0};//счётчик перехода от старых к новым цифрам

//----------------------------------------------------------------------------------------------------
//основная функция программы
//----------------------------------------------------------------------------------------------------
int main(void)
{
 InitAVR(); 
 sei();
 
 typedef enum
 {
  MODE_CLOCK_HOUR_MIN=0,//режим отображения часов и минут
  MODE_CLOCK_MIN_SEC,//режим отображения минут и секунд
  MODE_SET_TIME,//режим задания времени
  MODE_CATHODE_CLEANING,//режим чистки катодов
  MODE_TEMPERATURE_HUMIDITY//режим вывода температуры и влажности 
 } MODE;
 
 MODE mode=MODE_CLOCK_HOUR_MIN;
 
 while(1)
 {
  if (mode==MODE_CLOCK_HOUR_MIN) ModeClockHourMin();
  if (mode==MODE_CLOCK_MIN_SEC) ModeClockMinSec();
  //if (mode==MODE_SET_TIME) ModeSetTime();
  //if (mode==MODE_CATHODE_CLEANING) ModeCathodeCleaning();
  if (mode==MODE_TEMPERATURE_HUMIDITY) ModeTemperatureHumidity(); 
  
  MODE current_mode=mode;  
  cli();
  if (KEYBOARD_GetKeyPressedAndResetIt(KEY_STAR)==true)
  {     
   if (current_mode==MODE_CLOCK_HOUR_MIN) mode=MODE_CLOCK_MIN_SEC;
   if (current_mode==MODE_CLOCK_MIN_SEC) mode=MODE_CLOCK_HOUR_MIN;
  }  
  
  if (KEYBOARD_GetKeyPressedAndResetIt(KEY_SHARP)==true)
  {
   if (current_mode==MODE_CLOCK_HOUR_MIN || current_mode==MODE_CLOCK_MIN_SEC) mode=MODE_TEMPERATURE_HUMIDITY;
   if (current_mode==MODE_TEMPERATURE_HUMIDITY) mode=MODE_CLOCK_HOUR_MIN;   
  }
  sei();
  
  _delay_ms(100);
 }
 return(0);
}

//----------------------------------------------------------------------------------------------------
//режим отображения часов и минут
//----------------------------------------------------------------------------------------------------
void ModeClockHourMin(void)
{
 uint8_t hour;
 uint8_t min;
 uint8_t sec;//получить время 
 RTC_GetTime(&hour,&min,&sec);
 
 uint8_t set_digit[4];
 set_digit[0]=hour/10;
 set_digit[1]=hour%10;
 set_digit[2]=min/10;
 set_digit[3]=min%10;

 cli();
 for(uint8_t n=0;n<4;n++)
 {
  if (NewDigit[n]!=set_digit[n]) ChangeDigitCounter[n]=0x1f;
  NewDigit[n]=set_digit[n];
 }
 sei();
 
 OUTPUT_OutputPercent(false);
 OUTPUT_OutputMinus(false);
}

//----------------------------------------------------------------------------------------------------
//режим отображения минут и секунд
//----------------------------------------------------------------------------------------------------
void ModeClockMinSec(void)
{
 uint8_t hour;
 uint8_t min;
 uint8_t sec;//получить время 
 RTC_GetTime(&hour,&min,&sec);
 
 uint8_t set_digit[4];
 set_digit[0]=min/10;
 set_digit[1]=min%10;
 set_digit[2]=sec/10;
 set_digit[3]=sec%10;

 cli();
 for(uint8_t n=0;n<4;n++)
 {
  if (NewDigit[n]!=set_digit[n]) ChangeDigitCounter[n]=0x1f;
  NewDigit[n]=set_digit[n];
 }
 sei();
 
 OUTPUT_OutputPercent(false);
 OUTPUT_OutputMinus(false);
}
//----------------------------------------------------------------------------------------------------
//режим отображения влажности и температуры
//----------------------------------------------------------------------------------------------------
void ModeTemperatureHumidity(void)
{
 int8_t temp;//температура
 int8_t humidity;//влажность
 
 uint8_t set_digit[4];
 
 if (SENSOR_GetValue(&temp,&humidity)==true)//меняем показания
 {  
  if (temp<0) OUTPUT_OutputMinus(true);
         else OUTPUT_OutputMinus(false);
  if (temp<0) temp=-temp;
  set_digit[0]=temp/10;
  set_digit[1]=temp%10;
		  
  set_digit[2]=humidity/10;
  set_digit[3]=humidity%10;		  
 }
 else
 {
  set_digit[0]=0;
  set_digit[1]=0;
  set_digit[2]=0;
  set_digit[3]=0;  
 }  
  
 cli();
 for(uint8_t n=0;n<4;n++)
 {
  if (NewDigit[n]!=set_digit[n]) ChangeDigitCounter[n]=0x1f;
  NewDigit[n]=set_digit[n];
 }
 sei();
  
 OUTPUT_OutputPercent(true);    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//функции
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//----------------------------------------------------------------------------------------------------
//инициализация контроллера
//----------------------------------------------------------------------------------------------------
void InitAVR(void)
{
 DDRB=0;//весь порт сконфигурирован как вход
 DDRC=0;//весь порт сконфигурирован как вход
 DDRD=0;//весь порт сконфигурирован как вход
 PORTB=0;
 PORTC=0;
 PORTD=0;

 OUTPUT_Init(); 
 SENSOR_Init();
 RTC_Init();
 KEYBOARD_Init();
 
 
 //настраиваем таймер T0
 TCCR0=((1<<CS02)|(0<<CS01)|(1<<CS00));//выбран режим деления тактовых импульсов на 1024
 TCNT0=0;//начальное значение таймера
 TIMSK=(1<<TOIE0);//прерывание по переполнению таймера (таймер T0 восьмибитный)

 //настраиваем таймер T2
 TCCR2=((0<<FOC2)|(0<<WGM21)|(0<<WGM20)|(1<<CS22)|(1<<CS21)|(0<<CS20)|(0<<COM21)|(0<<COM20));//выбран режим деления тактовых импульсов на 256
 TCNT2=0;//начальное значение таймера
 TIMSK|=(1<<TOIE2);//прерывание по переполнению таймера (таймер T2 восьмибитный)
}

 

//----------------------------------------------------------------------------------------------------
//обработчик вектора прерывания таймера T0 (8-ми разрядный таймер) по переполнению
//----------------------------------------------------------------------------------------------------
ISR(TIMER0_OVF_vect)
{ 
 //выполняем плавное зажигание/погасание цифр с импользованием BAM (binary amplitude modulation)
 static uint8_t weight=0;//вес разряда
 
 uint8_t mask=0; 
 
 mask=(1<<weight);
 TCNT0=0xff- mask;
 weight++;
 if (weight==5) weight=0; 
 
 uint8_t digit[4]={Digit[0],Digit[1],Digit[2],Digit[3]};
 static bool new_digit[4]={false,false,false,false};
 
 for(uint8_t n=0;n<4;n++)
 {
  if (ChangeDigitCounter[n]==0) Digit[n]=NewDigit[n];//цифра сменилась
  if (ChangeDigitCounter[n]&mask) new_digit[n]=false;
                             else new_digit[n]=true;
  if (new_digit[n]==true) digit[n]=NewDigit[n];  
 } 
 
 OUTPUT_OutputDigit(digit); 
}

//----------------------------------------------------------------------------------------------------
//обработчик вектора прерывания таймера T02 (8-ми разрядный таймер) по переполнению
//----------------------------------------------------------------------------------------------------
ISR(TIMER2_OVF_vect)
{
 TCNT2=0;
 KEYBOARD_Scan();
 
 static uint8_t cnt=0;
 cnt++;
 cnt&=1;
 if (cnt!=0) return;
 
 for(uint8_t n=0;n<4;n++)
 {
  if (ChangeDigitCounter[n]>0) ChangeDigitCounter[n]--;
 } 
}
