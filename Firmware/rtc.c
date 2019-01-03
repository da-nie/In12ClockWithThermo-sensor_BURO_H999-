//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include "rtc.h"
#include "based.h"

//----------------------------------------------------------------------------------------------------
//����������������
//----------------------------------------------------------------------------------------------------

//������� I2C
#define F_I2C 50000UL
//����� DS1307
#define DS1307_ADR 0x68

//������ ���������
#define RTC_SEC_ADR     0x00
#define RTC_MIN_ADR     0x01
#define RTC_HOUR_ADR    0x02
#define RTC_OUT_ADR     0x07

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------

static void RTC_SetValue(uint8_t addr,uint8_t data);//���������� �������� RTC
static uint8_t RTC_GetValue(uint8_t addr);//������� �������� RTC


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//----------------------------------------------------------------------------------------------------
//�������������
//----------------------------------------------------------------------------------------------------
void RTC_Init(void)
{
 //�������������� I2C (TWI)  
 TWBR=(((F_CPU)/(F_I2C)-16)/2);
 TWSR=0;
 
 //�������� ����
 uint8_t s=RTC_GetValue(RTC_SEC_ADR);
 RTC_SetValue(RTC_SEC_ADR,s&0x7F);//�������� ������������
 RTC_SetValue(RTC_OUT_ADR,(0<<7)|(1<<4)|(0<<1)|(0<<0));//�������� ������������ ������ 
}


//----------------------------------------------------------------------------------------------------
//���������� �������� RTC
//----------------------------------------------------------------------------------------------------
void RTC_SetValue(uint8_t addr,uint8_t data)
{
 //��������� ��������� �����
 TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //������ �� ���� ����� SLA-W
 TWDR=(DS1307_ADR<<1)|0;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //�������� ����� �������� ds1307
 TWDR=addr;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //�������� ������
 TWDR=data;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //��������� ��������� ����
 TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}
//----------------------------------------------------------------------------------------------------
//������� �������� RTC
//----------------------------------------------------------------------------------------------------
uint8_t RTC_GetValue(uint8_t addr)
{
 //��������� ��������� �����
 TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //������ �� ���� ����� SLA-W
 TWDR=(DS1307_ADR<<1)|0;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //�������� ����� �������� ds1307
 TWDR=addr;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //��������� ��������� ����
 TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
 
 //��������� ������
 uint8_t data;
 //��������� ��������� �����
 TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //������ �� ���� ����� SLA-R
 TWDR=(DS1307_ADR<<1)|1;
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 //��������� ������
 TWCR=(1<<TWINT)|(1<<TWEN);
 while(!(TWCR&(1<<TWINT)));
 data=TWDR;
 //��������� ��������� ����
 TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
 return(data);
}
//----------------------------------------------------------------------------------------------------
//���������� �����
//----------------------------------------------------------------------------------------------------
void RTC_SetTime(uint8_t hour,uint8_t min,uint8_t sec)
{
 uint8_t hour_h=hour/10;
 uint8_t hour_l=hour%10;
 uint8_t min_h=min/10;
 uint8_t min_l=min%10;
 uint8_t sec_h=sec/10;
 uint8_t sec_l=sec%10;
 RTC_SetValue(RTC_SEC_ADR,(1<<7));//��������� ������������
 RTC_SetValue(RTC_MIN_ADR,(min_h<<4)|(min_l));
 RTC_SetValue(RTC_HOUR_ADR,((hour_h<<4)|(hour_l))&0x3F);
 RTC_SetValue(RTC_SEC_ADR,((sec_h<<4)|(sec_l))&0x7F);//����� ������� � �������� ������������
}
//----------------------------------------------------------------------------------------------------
//�������� �����
//----------------------------------------------------------------------------------------------------
void RTC_GetTime(uint8_t *hour,uint8_t *min,uint8_t *sec)
{
 //��������� �����
 uint8_t sec_t=RTC_GetValue(RTC_SEC_ADR);
 uint8_t min_t=RTC_GetValue(RTC_MIN_ADR);
 uint8_t hour_t=RTC_GetValue(RTC_HOUR_ADR);
 
 *sec=(sec_t&0x0F)+((sec_t&0xF0)>>4)*10;
 *min=(min_t&0x0F)+((min_t&0xF0)>>4)*10;
 *hour=(hour_t&0x0F)+((hour_t&0xF0)>>4)*10;
}
