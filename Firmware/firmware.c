//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------
#include "based.h"
#include "output.h"
#include "sensor.h"
#include "rtc.h"
#include "keyboard.h"
#include "watchdog.h"

//----------------------------------------------------------------------------------------------------
//������������
//----------------------------------------------------------------------------------------------------

//������ ������ �������� �����
typedef enum
{
 MODE_CLOCK_HOUR_MIN=0,//����� ����������� ����� � �����
 MODE_CLOCK_MIN_SEC,//����� ����������� ����� � ������
 MODE_SET_TIME_HOUR_TENS,//����� ������� �������
 MODE_SET_TIME_HOUR_UNITS,//����� ������� �������
 MODE_SET_TIME_MIN_TENS,//����� ������� �������
 MODE_SET_TIME_MIN_UNITS,//����� ������� �������
 MODE_CATHODE_CLEANING,//����� ������ �������
 MODE_TEMPERATURE_HUMIDITY//����� ������ ����������� � ��������� 
} MODE;

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void InitAVR(void);//������������� �����������
MODE ModeSetTime(MODE mode);//����� ��������� �������
void ModeClockHourMin(void);//����� ����������� ����� � �����
void ModeClockMinSec(void);//����� ����������� ����� � ������
void ModeTemperatureHumidity(void);//����� ����������� ��������� � �����������
void CathodeCleaning(void);//����� ������� ������� �����������

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

static const uint8_t CATHODE_CLEANING_COUNTER_MAX_VALUE_SEC=180;//���������� ������ �� ������� ������� �����������

volatile uint8_t Digit[4]={0,0,0,0};//������� ������������ �����
volatile uint8_t NewDigit[4]={0,0,0,0};//����� ������������ �����
volatile uint8_t ChangeDigitCounter[4]={0,0,0,0};//������� �������� �� ������ � ����� ������
volatile uint8_t CathodeCleaningCounter=0;//������� ������� �� ��������� ������ ������� �������


//----------------------------------------------------------------------------------------------------
//�������� ������� ���������
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
//����� ��������� �������
//----------------------------------------------------------------------------------------------------
MODE ModeSetTime(MODE mode)
{
 uint8_t position=0xff;
 if (mode==MODE_SET_TIME_HOUR_TENS) position=0;
 if (mode==MODE_SET_TIME_HOUR_UNITS) position=1;
 if (mode==MODE_SET_TIME_MIN_TENS) position=2;
 if (mode==MODE_SET_TIME_MIN_UNITS) position=3;

 //����������� ������� �� �����  
 cli();
 for(uint8_t n=0;n<4;n++)
 {
  if (NewDigit[n]==Digit[n])//������ ������������
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
    //������������� �����
	cli();
	uint8_t hour=NewDigit[0]*10+NewDigit[1];
	uint8_t min=NewDigit[2]*10+NewDigit[3];
	sei();
	
	if (hour>=24 || min>=60) mode=MODE_SET_TIME_HOUR_TENS;//����� ����� ������ ������	
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
//����� ����������� ����� � �����
//----------------------------------------------------------------------------------------------------
void ModeClockHourMin(void)
{
 uint8_t hour;
 uint8_t min;
 uint8_t sec;//�������� ����� 
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
//����� ����������� ����� � ������
//----------------------------------------------------------------------------------------------------
void ModeClockMinSec(void)
{
 uint8_t hour;
 uint8_t min;
 uint8_t sec;//�������� ����� 
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
//����� ����������� ��������� � �����������
//----------------------------------------------------------------------------------------------------
void ModeTemperatureHumidity(void)
{
 int8_t temp;//�����������
 int8_t humidity;//���������
 
 uint8_t set_digit[4];
 
 if (SENSOR_GetValue(&temp,&humidity)==true)//������ ���������
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
//����� ������� ������� �����������
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
//�������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//----------------------------------------------------------------------------------------------------
//������������� �����������
//----------------------------------------------------------------------------------------------------
void InitAVR(void)
{
 DDRB=0;//���� ���� ��������������� ��� ����
 DDRC=0;//���� ���� ��������������� ��� ����
 DDRD=0;//���� ���� ��������������� ��� ����
 PORTB=0;
 PORTC=0;
 PORTD=0;

 OUTPUT_Init(); 
 SENSOR_Init();
 RTC_Init();
 KEYBOARD_Init();
 
 
 //����������� ������ T0
 TCCR0=((1<<CS02)|(0<<CS01)|(1<<CS00));//������ ����� ������� �������� ��������� �� 1024
 TCNT0=0;//��������� �������� �������
 TIMSK=(1<<TOIE0);//���������� �� ������������ ������� (������ T0 ������������)

 //����������� ������ T2
 TCCR2=((0<<FOC2)|(0<<WGM21)|(0<<WGM20)|(1<<CS22)|(1<<CS21)|(0<<CS20)|(0<<COM21)|(0<<COM20));//������ ����� ������� �������� ��������� �� 256
 TCNT2=0;//��������� �������� �������
 TIMSK|=(1<<TOIE2);//���������� �� ������������ ������� (������ T2 ������������)
}

 

//----------------------------------------------------------------------------------------------------
//���������� ������� ���������� ������� T0 (8-�� ��������� ������) �� ������������
//----------------------------------------------------------------------------------------------------
ISR(TIMER0_OVF_vect)
{ 
 //��������� ������� ���������/��������� ���� � �������������� BAM (binary amplitude modulation)
 static uint8_t weight=0;//��� �������
 
 uint8_t mask=0; 
 
 mask=(1<<weight);
 TCNT0=0xff-mask;
 weight++;
 if (weight==5) weight=0; 
 
 uint8_t digit[4]={Digit[0],Digit[1],Digit[2],Digit[3]};
 static bool new_digit[4]={false,false,false,false};
 
 for(uint8_t n=0;n<4;n++)
 {
  if (ChangeDigitCounter[n]==0) Digit[n]=NewDigit[n];//����� ���������
  if (ChangeDigitCounter[n]&mask) new_digit[n]=false;
                             else new_digit[n]=true;
  if (new_digit[n]==true) digit[n]=NewDigit[n];  
 } 
 
 OUTPUT_OutputDigit(digit); 
}

//----------------------------------------------------------------------------------------------------
//���������� ������� ���������� ������� T2 (8-�� ��������� ������) �� ������������
//----------------------------------------------------------------------------------------------------
ISR(TIMER2_OVF_vect)
{
 //������ ���������� 122.0703125 ���� � �������

 TCNT2=0;
 KEYBOARD_Scan();
 SENSOR_DecrementEnabledDataCounter();
 
 static uint8_t tick=0;
 
 tick++;
 if (tick==122)//������ ������� (��������)
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
