//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include "output.h"
#include "based.h"

//----------------------------------------------------------------------------------------------------
//����������������
//----------------------------------------------------------------------------------------------------

//�������� �������� ����
#define DIGIT_VALUE_DDR  DDRB
#define DIGIT_VALUE_PORT PORTB
#define DIGIT_VALUE      3

//����� ���
#define DIGIT_SHIFT_DDR  DDRB
#define DIGIT_SHIFT_PORT PORTB
#define DIGIT_SHIFT      1

//������� ���������� �������� �� ����� ��������
#define DIGIT_OUT_DDR    DDRB
#define DIGIT_OUT_PORT   PORTB
#define DIGIT_OUT        2

//����������� ����� "�����"
#define DIGIT_MINUS_DDR    DDRB
#define DIGIT_MINUS_PORT   PORTB
#define DIGIT_MINUS        4

//����������� ����� "�������"
#define DIGIT_PERCENT_DDR    DDRB
#define DIGIT_PERCENT_PORT   PORTB
#define DIGIT_PERCENT        5


//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//----------------------------------------------------------------------------------------------------
//�������������
//----------------------------------------------------------------------------------------------------
void OUTPUT_Init(void)
{
 DIGIT_VALUE_DDR|=(1<<DIGIT_VALUE);
 DIGIT_SHIFT_DDR|=(1<<DIGIT_SHIFT);
 DIGIT_OUT_DDR|=(1<<DIGIT_OUT);
 DIGIT_MINUS_DDR|=(1<<DIGIT_MINUS);
 DIGIT_PERCENT_DDR|=(1<<DIGIT_PERCENT);
 
 DIGIT_VALUE_PORT&=0xff^(1<<DIGIT_VALUE);
 DIGIT_SHIFT_PORT&=0xff^(1<<DIGIT_SHIFT);
 DIGIT_OUT_PORT&=0xff^(1<<DIGIT_OUT); 
 DIGIT_MINUS_PORT&=0xff^(1<<DIGIT_MINUS); 
 DIGIT_PERCENT_PORT&=0xff^(1<<DIGIT_PERCENT); 
}

//----------------------------------------------------------------------------------------------------
//������� �����
//----------------------------------------------------------------------------------------------------
void OUTPUT_OutputDigit(const uint8_t digit[4])
{
 OUTPUT_OutputOneDigit(digit[3]);
 OUTPUT_OutputOneDigit(digit[2]);
 OUTPUT_OutputOneDigit(digit[1]);
 OUTPUT_OutputOneDigit(digit[0]);
 
 DIGIT_OUT_PORT|=(1<<DIGIT_OUT);
 DIGIT_OUT_PORT&=0xff^(1<<DIGIT_OUT); 
}
//----------------------------------------------------------------------------------------------------
//������� ���� �����
//----------------------------------------------------------------------------------------------------
void OUTPUT_OutputOneDigit(uint8_t digit)
{ 
 DIGIT_VALUE_PORT&=0xff^(1<<DIGIT_VALUE);
 DIGIT_SHIFT_PORT&=0xff^(1<<DIGIT_SHIFT);
 DIGIT_OUT_PORT&=0xff^(1<<DIGIT_OUT);

 uint8_t counter=0;
 uint8_t mask[4]={4,2,8,1};
 while(counter<4)
 {
  if (digit&mask[counter]) DIGIT_VALUE_PORT|=(1<<DIGIT_VALUE);
                       else DIGIT_VALUE_PORT&=0xff^(1<<DIGIT_VALUE);
			 
  DIGIT_SHIFT_PORT|=(1<<DIGIT_SHIFT);
  DIGIT_SHIFT_PORT&=0xff^(1<<DIGIT_SHIFT); 
  counter++;
 } 
}
//----------------------------------------------------------------------------------------------------
//���������� ���� "�����"
//----------------------------------------------------------------------------------------------------
void OUTPUT_OutputMinus(bool state)
{
 if (state==true) DIGIT_MINUS_PORT|=(1<<DIGIT_MINUS);
              else DIGIT_MINUS_PORT&=0xff^(1<<DIGIT_MINUS);
}
//----------------------------------------------------------------------------------------------------
//���������� ���� "�������"
//----------------------------------------------------------------------------------------------------
void OUTPUT_OutputPercent(bool state)
{
 if (state==true) DIGIT_PERCENT_PORT|=(1<<DIGIT_PERCENT);
              else DIGIT_PERCENT_PORT&=0xff^(1<<DIGIT_PERCENT);
}