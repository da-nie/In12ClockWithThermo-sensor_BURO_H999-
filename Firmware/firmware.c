//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------
#include "based.h"
#include "output.h"
#include "sensor.h"
#include "rtc.h"
#include "keyboard.h"

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void InitAVR(void);//������������� �����������
void ModeClockHourMin(void);//����� ����������� ����� � �����
void ModeClockMinSec(void);//����� ����������� ����� � ������
void ModeTemperatureHumidity(void);//����� ����������� ��������� � �����������

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------
volatile uint8_t Digit[4]={0,0,0,0};//������� ������������ �����
volatile uint8_t NewDigit[4]={0,0,0,0};//����� ������������ �����
volatile uint8_t ChangeDigitCounter[4]={0,0,0,0};//������� �������� �� ������ � ����� ������

//----------------------------------------------------------------------------------------------------
//�������� ������� ���������
//----------------------------------------------------------------------------------------------------
int main(void)
{
 InitAVR(); 
 sei();
 
 typedef enum
 {
  MODE_CLOCK_HOUR_MIN=0,//����� ����������� ����� � �����
  MODE_CLOCK_MIN_SEC,//����� ����������� ����� � ������
  MODE_SET_TIME,//����� ������� �������
  MODE_CATHODE_CLEANING,//����� ������ �������
  MODE_TEMPERATURE_HUMIDITY//����� ������ ����������� � ��������� 
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
 TCNT0=0xff- mask;
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
//���������� ������� ���������� ������� T02 (8-�� ��������� ������) �� ������������
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
