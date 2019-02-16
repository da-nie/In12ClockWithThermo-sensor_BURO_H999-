//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include "sensor.h"
#include "based.h"

//----------------------------------------------------------------------------------------------------
//������������
//----------------------------------------------------------------------------------------------------

//��� �����
typedef enum 
{
 BLOCK_TYPE_UNKNOW,//����������� ����
 BLOCK_TYPE_DIVIDER,//����������� (������ �������� ������)
 BLOCK_TYPE_SYNCHRO,//������������
 BLOCK_TYPE_ONE,//�������
 BLOCK_TYPE_ZERO//����
} BLOCK_TYPE;

//����� �������������
typedef enum 
{
 MODE_WAIT_SYNCHRO,//�������� �������������
 MODE_WAIT_ZERO_FIRST,//�������� ������� ����
 MODE_WAIT_ZERO_SECOND,//�������� ������� ����
 MODE_RECEIVE_DATA//���� ������
} MODE;
 
//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------
static const uint16_t MAX_TIMER_INTERVAL_VALUE=0xFFFF;//������������ �������� ��������� �������
static const uint16_t MAX_TIMER_ENABLED_COUNTER_INTERVAL_VALUE=0xFF;//������������ �������� ��������� ������� ������� ������
static const uint32_t TIMER_FREQUENCY_HZ=31250UL;//������� �������
static const uint32_t TIMER_ENABLED_COUNTER_FREQUENCY_HZ=31250UL;//������� ������� ������� ������
static volatile bool TimerOverflow=false;//���� �� ������������ �������

static uint8_t Buffer[20];//����� ������ ���������
static uint8_t BitSize=0;//���������� �������� ���
static uint8_t Byte=0;//���������� ����
static volatile int8_t CurrentTemp=0;//�����������
static volatile int8_t CurrentHumidity=0;//���������
static volatile uint32_t EnabledDataCounter=0;//������� ������� �������

static volatile uint8_t LastBuffer[3][10];//��������� �������� ������

//----------------------------------------------------------------------------------------------------
//����������������
//----------------------------------------------------------------------------------------------------

//������������ �������� �������� ������� ������� �������
#define MAX_ENABLED_DATA_COUNTER ((TIMER_ENABLED_COUNTER_FREQUENCY_HZ*60*10)/MAX_TIMER_ENABLED_COUNTER_INTERVAL_VALUE)

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------

static void SENSOR_SetValue(int8_t temp,int8_t humidity);//������ ���������
static void SENSOR_SetTimerOverflow(void);//���������� ���� ������������ �������
static void SENSOR_ResetTimerOverflow(void);//�������� ���� ������������ �������
static bool SENSOR_IsTimerOverflow(void);//��������, ���� �� ������������ �������
static uint16_t SENSOR_GetTimerValue(void);//�������� �������� �������
static void SENSOR_ResetTimerValue(void);//�������� �������� ������� 
static BLOCK_TYPE SENSOR_GetBlockType(uint32_t counter,bool value);//�������� ��� �����
static void SENSOR_AddBit(bool state);//�������� ��� ������
static void SENSOR_ResetData(void);//������ ������ ������ ������
static void SENSOR_AnalizeCounter(uint32_t counter,bool value,MODE *mode);//������ �����

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//----------------------------------------------------------------------------------------------------
//�������������
//----------------------------------------------------------------------------------------------------
void SENSOR_Init(void)
{
 //����������� ���������� ����������
 ACSR=(0<<ACD)|(1<<ACBG)|(0<<ACO)|(0<<ACI)|(1<<ACIE)|(0<<ACIC)|(0<<ACIS1)|(0<<ACIS0);
 //ACD - ��������� ����������� (0 - ����ר�!)
 //ACBG - ����������� � ����������������� ����� ����������� ������������ ���'�
 //ACO - ��������� ��������� (����� �����������)
 //ACI - ���� ���������� �� �����������
 //ACIE - ���������� ���������� �� �����������
 //ACIC - ����������� ����������� � ����� ������� ������� T1
 //ACIS1,ACID0 - ������� ��������� ���������� �� �����������
  
 //����������� ������ T1 �� ������� 31250 ��
 TCCR1A=(0<<WGM11)|(0<<WGM10)|(0<<COM1A1)|(0<<COM1A0)|(0<<COM1B1)|(0<<COM1B0);
 //COM1A1-COM1A0 - ��������� ������ OC1A
 //COM1B1-COM1B0 - ��������� ������ OC1B 
 //WGM11-WGM10 - ����� ������ �������
 TCCR1B=(0<<WGM13)|(0<<WGM12)|(1<<CS12)|(0<<CS11)|(0<<CS10)|(0<<ICES1)|(0<<ICNC1); 
 //WGM13-WGM12 - ����� ������ �������
 //CS12-CS10 - ���������� �������� �������� (������ ����� ������� �������� ��������� �� 256 (������� ������� 31250 ��))
 //ICNC1 - ���������� ������ ���������� ����� ����� �������
 //ICES1 - ����� ��������� ������ ������� �������
 TCNT1=0;//��������� �������� �������
 TIMSK|=(1<<TOIE1);//���������� �� ������������ ������� (������ T1 �����������������) 
 
 EnabledDataCounter=0;
 
 for(uint8_t n=0;n<3;n++)
 {
  for(uint8_t m=0;m<10;m++) LastBuffer[n][m]=0; 
 }
}

//----------------------------------------------------------------------------------------------------
//�������� ���������
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
//������ ���������
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
//��������� ������� ������� ������
//----------------------------------------------------------------------------------------------------
void SENSOR_DecrementEnabledDataCounter(void)
{
 cli();
 if (EnabledDataCounter>0) EnabledDataCounter--;
 sei();
}

//----------------------------------------------------------------------------------------------------
//���������� ���� ������������ �������
//----------------------------------------------------------------------------------------------------
void SENSOR_SetTimerOverflow(void)
{
 cli();
 TimerOverflow=true;
 sei();
}
//----------------------------------------------------------------------------------------------------
//�������� ���� ������������ �������
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetTimerOverflow(void)
{
 cli();
 TimerOverflow=false;
 sei();
}
//----------------------------------------------------------------------------------------------------
//��������, ���� �� ������������ �������
//----------------------------------------------------------------------------------------------------
bool SENSOR_IsTimerOverflow(void)
{
 cli();
 bool ret=TimerOverflow;
 sei();
 return(ret);
}

//----------------------------------------------------------------------------------------------------
//�������� �������� ������� 
//----------------------------------------------------------------------------------------------------
uint16_t SENSOR_GetTimerValue(void)
{
 cli();
 uint16_t ret=TCNT1;
 sei(); 
 return(ret);
} 


//----------------------------------------------------------------------------------------------------
//�������� �������� ������� 
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetTimerValue(void)
{
 cli();
 TCNT1=0;
 sei();
 SENSOR_ResetTimerOverflow();
} 
//----------------------------------------------------------------------------------------------------
//�������� ��� �����
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
 

 if (counter>DIVIDER_MIN && counter<DIVIDER_MAX) return(BLOCK_TYPE_DIVIDER);//�����������
 if (counter>ZERO_MIN && counter<ZERO_MAX) return(BLOCK_TYPE_ZERO);//����
 if (counter>ONE_MIN && counter<ONE_MAX) return(BLOCK_TYPE_ONE);//����
 if (counter>SYNCHRO_MIN && counter<SYNCHRO_MAX) return(BLOCK_TYPE_SYNCHRO);//������������
 return(BLOCK_TYPE_UNKNOW);//����������� ����
}
//----------------------------------------------------------------------------------------------------
//�������� ��� ������
//----------------------------------------------------------------------------------------------------
void SENSOR_AddBit(bool state)
{
 if ((BitSize>>2)>=19) return;//����� ��������
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
//������ ������ ������ ������
//----------------------------------------------------------------------------------------------------
void SENSOR_ResetData(void)
{
 BitSize=0;
 Byte=0;
}

//----------------------------------------------------------------------------------------------------
//������ �����
//----------------------------------------------------------------------------------------------------
void SENSOR_AnalizeCounter(uint32_t counter,bool value,MODE *mode)
{
 //������ ��� �����
 BLOCK_TYPE type=SENSOR_GetBlockType(counter,value);

 if (type==BLOCK_TYPE_UNKNOW)
 {
  *mode=MODE_WAIT_SYNCHRO;
  SENSOR_ResetData();
  return;
 } 
 if (type==BLOCK_TYPE_DIVIDER) return;//����������� ���������� ��� ������� 
 //������� ������ ���������� � ����������� ��������������
 if ((*mode)==MODE_WAIT_SYNCHRO)//��� �������������
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
 if ((*mode)==MODE_WAIT_ZERO_FIRST || (*mode)==MODE_WAIT_ZERO_SECOND)//��� ��� ����
 {
  if (type==BLOCK_TYPE_SYNCHRO && (*mode)==MODE_WAIT_ZERO_FIRST) return;//������������ ������������
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
 //��������� ������
 if (type==BLOCK_TYPE_SYNCHRO)//�������� ���������
 {
  uint8_t size=(BitSize>>2);
  if  (size!=10)
  {
   mode=MODE_WAIT_SYNCHRO;
   SENSOR_ResetData();
   return; 
  }
  //�������� ������ � ������ ��������� �������� ������
  for(uint8_t n=1;n<3;n++)
  {
   for(uint8_t m=0;m<10;m++) LastBuffer[n-1][m]=LastBuffer[n][m];
  }
  for(uint8_t m=0;m<10;m++) LastBuffer[2][m]=Buffer[m];
  //��������� ��� ���� ������ ���� ������� ���������� ����������  
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
  uint8_t humidity=(Buffer[7]<<4)|(Buffer[6]);//���������
  int16_t temp=(Buffer[5]<<8)|(Buffer[4]<<4)|(Buffer[3]);//����������� � ������� ������� ���������� �� ��������� �� 50 �����������
  //��������� � ������� ���������� �� 10.
  temp=(temp-320)*5/9-500;
  //���������
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
 //���� ������
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
//����������� �������� ����������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
//----------------------------------------------------------------------------------------------------
//���������� ������� ���������� ������� T1 (16-�� ��������� ������) �� ������������
//----------------------------------------------------------------------------------------------------
ISR(TIMER1_OVF_vect)
{   
 SENSOR_SetTimerOverflow(); 
} 
 
//----------------------------------------------------------------------------------------------------
//���������� ������� ���������� �� �����������
//----------------------------------------------------------------------------------------------------
ISR(ANA_COMP_vect)
{
 ACSR&=0xFF^(1<<ACIE);//��������� ����������
 ACSR|=(1<<ACI);//���������� ���� ���������� �����������
 
 static MODE mode=MODE_WAIT_SYNCHRO;
 
 //����� ������������ ���������
 uint16_t length=SENSOR_GetTimerValue();
 if (SENSOR_IsTimerOverflow()==true) length=MAX_TIMER_INTERVAL_VALUE;//���� ������������, ������� �������� ������������
 SENSOR_ResetTimerValue();
 //���������� �� ������
 bool value=true;
 if (ACSR&(1<<ACO)) value=false;
 SENSOR_AnalizeCounter(length,value,&mode);
 ACSR|=(1<<ACIE);//��������� ����������
}
