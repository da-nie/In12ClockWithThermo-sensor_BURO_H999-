//----------------------------------------------------------------------------------------------------
//библиотеки
//----------------------------------------------------------------------------------------------------
#include "based.h"
#include "output.h"
#include "sensor.h"
#include "rtc.h"
#include "keyboard.h"
#include "watchdog.h"

//----------------------------------------------------------------------------------------------------
//перечисления
//----------------------------------------------------------------------------------------------------

//режимы работы главного цикла
typedef enum
{
 MODE_CLOCK_HOUR_MIN=0,//режим отображения часов и минут
 MODE_CLOCK_MIN_SEC,//режим отображения минут и секунд
 MODE_SET_TIME_HOUR_TENS,//режим задания времени
 MODE_SET_TIME_HOUR_UNITS,//режим задания времени
 MODE_SET_TIME_MIN_TENS,//режим задания времени
 MODE_SET_TIME_MIN_UNITS,//режим задания времени
 MODE_CATHODE_CLEANING,//режим чистки катодов
 MODE_TEMPERATURE_HUMIDITY//режим вывода температуры и влажности 
} MODE;

//----------------------------------------------------------------------------------------------------
//прототипы функций
//----------------------------------------------------------------------------------------------------
void InitAVR(void);//инициализация контроллера
MODE ModeSetTime(MODE mode);//режим установки времени
void ModeClockHourMin(void);//режим отображения часов и минут
void ModeClockMinSec(void);//режим отображения минут и секунд
void ModeTemperatureHumidity(void);//режим отображения влажности и температуры
void CathodeCleaning(void);//режим очистки катодов индикаторов

//----------------------------------------------------------------------------------------------------
//глобальные переменные
//----------------------------------------------------------------------------------------------------

static const uint8_t CATHODE_CLEANING_COUNTER_MAX_VALUE_SEC=180;//количество секунд до очистки катодов индикаторов

volatile uint8_t Digit[4]={0,0,0,0};//текущие отображаемые цифры
volatile uint8_t NewDigit[4]={0,0,0,0};//новые отображаемые цифры
volatile uint8_t ChangeDigitCounter[4]={0,0,0,0};//счётчик перехода от старых к новым цифрам
volatile uint8_t CathodeCleaningCounter=0;//счётчик времени до включения режима очистки катодов


//----------------------------------------------------------------------------------------------------
//основная функция программы
//----------------------------------------------------------------------------------------------------
int main(void)
{
 InitAVR();
 
 WDT_Reset();
 _delay_ms(500);
 WDT_WatchDogOff();
 WDT_WatchDogOn();
 
 CathodeCleaningCounter=CATHODE_CLEANING_COUNTER_MAX_VALUE_SEC;
 
 sei();
 
 MODE mode=MODE_CLOCK_HOUR_MIN;
 MODE last_mode=mode; 
 
 while(1)
 {
  WDT_Reset(); 
  if (mode==MODE_CLOCK_HOUR_MIN) ModeClockHourMin();
  if (mode==MODE_CLOCK_MIN_SEC) ModeClockMinSec();
  if (mode==MODE_SET_TIME_HOUR_TENS || mode==MODE_SET_TIME_HOUR_UNITS || mode==MODE_SET_TIME_MIN_TENS || mode==MODE_SET_TIME_MIN_UNITS ) mode=ModeSetTime(mode);
  if (mode==MODE_CATHODE_CLEANING)
  {
   CathodeCleaning();
   mode=last_mode;
   cli();
   CathodeCleaningCounter=CATHODE_CLEANING_COUNTER_MAX_VALUE_SEC;
   sei();
   continue;
  }
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
  
  if (KEYBOARD_GetKeyState(KEY_STAR)==true && KEYBOARD_GetKeyState(KEY_SHARP)==true)
  {  
   mode=MODE_SET_TIME_HOUR_TENS;
   KEYBOARD_GetKeyPressedAndResetIt(KEY_STAR);
   KEYBOARD_GetKeyPressedAndResetIt(KEY_SHARP);   
  } 
  if (CathodeCleaningCounter==0 && mode!=MODE_CATHODE_CLEANING)
  {
   last_mode=mode;
   mode=MODE_CATHODE_CLEANING;   
  } 
  sei();
  
  _delay_ms(100);
 }
 return(0);
}

//----------------------------------------------------------------------------------------------------
//режим установки времени
//----------------------------------------------------------------------------------------------------
MODE ModeSetTime(MODE mode)
{
 uint8_t position=0xff;
 if (mode==MODE_SET_TIME_HOUR_TENS) position=0;
 if (mode==MODE_SET_TIME_HOUR_UNITS) position=1;
 if (mode==MODE_SET_TIME_MIN_TENS) position=2;
 if (mode==MODE_SET_TIME_MIN_UNITS) position=3;

 //переключаем разряды по кругу  
 cli();
 for(uint8_t n=0;n<4;n++)
 {
  if (NewDigit[n]==Digit[n])//разряд переключился
  {   
   if (n==position) 
   {
    NewDigit[n]++;
	NewDigit[n]%=10;
	ChangeDigitCounter[n]=0x1f;   
   }
  }
 } 
 KEY key=KEYBOARD_GetPressedKey();
 sei();
 if (key>=KEY_0 && key<=KEY_9)
 {
  if (position>=0 && position<4) 
  {
   cli();   
   NewDigit[position]=key-KEY_0;
   ChangeDigitCounter[position]=0x01;
   sei();
   if (position==0) mode=MODE_SET_TIME_HOUR_UNITS;
   if (position==1) mode=MODE_SET_TIME_MIN_TENS;
   if (position==2) mode=MODE_SET_TIME_MIN_UNITS;
   if (position==3)
   {
    //устанавливаем время
	cli();
	uint8_t hour=NewDigit[0]*10+NewDigit[1];
	uint8_t min=NewDigit[2]*10+NewDigit[3];
	sei();
	
	if (hour>=24 || min>=60) mode=MODE_SET_TIME_HOUR_TENS;//такое время задать нельзя	
	else
	{
     uint8_t sec=0;
	 RTC_SetTime(hour,min,sec);
     mode=MODE_CLOCK_HOUR_MIN;
     KEYBOARD_GetKeyPressedAndResetIt(KEY_STAR);
     KEYBOARD_GetKeyPressedAndResetIt(KEY_SHARP);	
	}
   }
  }
 }
  
 OUTPUT_OutputPercent(false);
 OUTPUT_OutputMinus(false);
 
 return(mode);
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
//----------------------------------------------------------------------------------------------------
//режим очистки катодов индикаторов
//----------------------------------------------------------------------------------------------------
void CathodeCleaning(void)
{
 for(uint8_t n=0;n<10;n++)
 {
  cli();
  for(uint8_t m=0;m<4;m++)
  {
   ChangeDigitCounter[m]=0x1f;
   NewDigit[m]=n;
  }
  sei();
  WDT_Reset();
  _delay_ms(200);
  WDT_Reset();
  _delay_ms(200);
  WDT_Reset();
  _delay_ms(200);
  WDT_Reset();
  _delay_ms(200);
  WDT_Reset();
  _delay_ms(200);
 }
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
 TCNT0=0xff-mask;
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
//обработчик вектора прерывания таймера T2 (8-ми разрядный таймер) по переполнению
//----------------------------------------------------------------------------------------------------
ISR(TIMER2_OVF_vect)
{
 //таймер вызывается 122.0703125 раза в секунду

 TCNT2=0;
 KEYBOARD_Scan();
 SENSOR_DecrementEnabledDataCounter();
 
 static uint8_t tick=0;
 
 tick++;
 if (tick==122)//прошла секунда (примерно)
 {
  if (CathodeCleaningCounter>0) CathodeCleaningCounter--; 
  tick=0;
 }
 
 if ((tick&0x01)!=0) return;
 
 for(uint8_t n=0;n<4;n++)
 {
  if (ChangeDigitCounter[n]>0) ChangeDigitCounter[n]--;
 } 
}
