//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include "keyboard.h"
#include "based.h"

//----------------------------------------------------------------------------------------------------
//����������������
//----------------------------------------------------------------------------------------------------

//����� ������ 1
#define KEYBOARD_OUTPUT_LINE_1_DDR    DDRD
#define KEYBOARD_OUTPUT_LINE_1_PORT   PORTD
#define KEYBOARD_OUTPUT_LINE_1        4

//����� ������ 2
#define KEYBOARD_OUTPUT_LINE_2_DDR    DDRD
#define KEYBOARD_OUTPUT_LINE_2_PORT   PORTD
#define KEYBOARD_OUTPUT_LINE_2        3

//����� ������ 3
#define KEYBOARD_OUTPUT_LINE_3_DDR    DDRD
#define KEYBOARD_OUTPUT_LINE_3_PORT   PORTD
#define KEYBOARD_OUTPUT_LINE_3        2

//����� ������ 4
#define KEYBOARD_OUTPUT_LINE_4_DDR    DDRD
#define KEYBOARD_OUTPUT_LINE_4_PORT   PORTD
#define KEYBOARD_OUTPUT_LINE_4        1

//����� ����� 1
#define KEYBOARD_INPUT_LINE_1_DDR    DDRD
#define KEYBOARD_INPUT_LINE_1_PORT   PORTD
#define KEYBOARD_INPUT_LINE_1_PIN    PIND
#define KEYBOARD_INPUT_LINE_1        5

//����� ����� 2
#define KEYBOARD_INPUT_LINE_2_DDR    DDRB
#define KEYBOARD_INPUT_LINE_2_PORT   PORTB
#define KEYBOARD_INPUT_LINE_2_PIN    PINB
#define KEYBOARD_INPUT_LINE_2        7

//����� ����� 3
#define KEYBOARD_INPUT_LINE_3_DDR    DDRB
#define KEYBOARD_INPUT_LINE_3_PORT   PORTB
#define KEYBOARD_INPUT_LINE_3_PIN    PINB
#define KEYBOARD_INPUT_LINE_3        6

//----------------------------------------------------------------------------------------------------
//������������
//----------------------------------------------------------------------------------------------------

typedef enum
{
 KEY_STATE_UP,//������� �� ������
 KEY_STATE_DOWN,//������� ������
 KEY_STATE_PRESSED//������� ������ � ��������
} KEY_STATE;

//----------------------------------------------------------------------------------------------------
//���������� ����������
//----------------------------------------------------------------------------------------------------

volatile static uint8_t KeyCounter[KEY_AMOUNT];//������ ��������� ������ (� ������������)
volatile static KEY_STATE KeyState[KEY_AMOUNT];//������ ��������� �������

static const uint8_t MAX_KEY_COUNTER_VALUE=64;//������������ �������� �������� �������
static const uint8_t MIN_KEY_COUNTER_VALUE=0;//����������� �������� �������� �������
static const uint8_t KEY_PRESSED_MIN_COUNTER_VALUE=32;//����������� �������� ��������, ��� ������� ������� ��������� �������

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------

static void KEYBOARD_KeyProcessing(KEY key);//��������� �������
static void KEYBOARD_KeyDown(KEY key);//������� ������
static void KEYBOARD_KeyUp(KEY key);//������� ��������

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//�������
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//----------------------------------------------------------------------------------------------------
//�������������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_Init(void)
{
 KEYBOARD_OUTPUT_LINE_1_DDR|=(1<<KEYBOARD_OUTPUT_LINE_1);
 KEYBOARD_OUTPUT_LINE_2_DDR|=(1<<KEYBOARD_OUTPUT_LINE_2);
 KEYBOARD_OUTPUT_LINE_3_DDR|=(1<<KEYBOARD_OUTPUT_LINE_3);
 KEYBOARD_OUTPUT_LINE_4_DDR|=(1<<KEYBOARD_OUTPUT_LINE_4);
 
 KEYBOARD_INPUT_LINE_1_DDR&=0xff^(1<<KEYBOARD_INPUT_LINE_1);
 KEYBOARD_INPUT_LINE_2_DDR&=0xff^(1<<KEYBOARD_INPUT_LINE_2);
 KEYBOARD_INPUT_LINE_3_DDR&=0xff^(1<<KEYBOARD_INPUT_LINE_3);
 
 KEYBOARD_OUTPUT_LINE_1_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_1);
 KEYBOARD_OUTPUT_LINE_2_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_2);
 KEYBOARD_OUTPUT_LINE_3_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_3);
 KEYBOARD_OUTPUT_LINE_4_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_4);
 
 KEYBOARD_INPUT_LINE_1_PORT&=0xff^(1<<KEYBOARD_INPUT_LINE_1);
 KEYBOARD_INPUT_LINE_2_PORT&=0xff^(1<<KEYBOARD_INPUT_LINE_2);
 KEYBOARD_INPUT_LINE_3_PORT&=0xff^(1<<KEYBOARD_INPUT_LINE_3);
 
 for(uint8_t n=0;n<KEY_AMOUNT;n++) 
 {
  KeyCounter[n]=0;
  KeyState[n]=KEY_STATE_UP;
 }
}

//----------------------------------------------------------------------------------------------------
//���������� ������������ ����������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_Scan(void)
{
 static const uint8_t key[4][3]={{KEY_1,KEY_2,KEY_3},{KEY_4,KEY_5,KEY_6},{KEY_7,KEY_8,KEY_9},{KEY_STAR,KEY_0,KEY_SHARP}};

 for(uint8_t line=0;line<4;line++)
 {
  if (line==0) KEYBOARD_OUTPUT_LINE_1_PORT|=(1<<KEYBOARD_OUTPUT_LINE_1);
  if (line==1) KEYBOARD_OUTPUT_LINE_2_PORT|=(1<<KEYBOARD_OUTPUT_LINE_2);
  if (line==2) KEYBOARD_OUTPUT_LINE_3_PORT|=(1<<KEYBOARD_OUTPUT_LINE_3);
  if (line==3) KEYBOARD_OUTPUT_LINE_4_PORT|=(1<<KEYBOARD_OUTPUT_LINE_4);
  
  asm volatile ("nop"::);
  asm volatile ("nop"::);
  asm volatile ("nop"::);
  asm volatile ("nop"::);
  asm volatile ("nop"::);
  
  if (KEYBOARD_INPUT_LINE_1_PIN&(1<<KEYBOARD_INPUT_LINE_1)) KEYBOARD_KeyDown(key[line][0]);
                                                        else KEYBOARD_KeyUp(key[line][0]);
													  
  if (KEYBOARD_INPUT_LINE_2_PIN&(1<<KEYBOARD_INPUT_LINE_2)) KEYBOARD_KeyDown(key[line][1]);
                                                        else KEYBOARD_KeyUp(key[line][1]);
													  
  if (KEYBOARD_INPUT_LINE_3_PIN&(1<<KEYBOARD_INPUT_LINE_3)) KEYBOARD_KeyDown(key[line][2]);
                                                        else KEYBOARD_KeyUp(key[line][2]);
 
  KEYBOARD_OUTPUT_LINE_1_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_1);
  KEYBOARD_OUTPUT_LINE_2_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_2);
  KEYBOARD_OUTPUT_LINE_3_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_3);
  KEYBOARD_OUTPUT_LINE_4_PORT&=0xff^(1<<KEYBOARD_OUTPUT_LINE_4);  
 }
}
//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_KeyProcessing(KEY key)
{
 if (KEYBOARD_GetKeyState(key)==true)
 {
  if (KeyState[key]==KEY_STATE_UP) KeyState[key]=KEY_STATE_DOWN;
 }
 else
 {
  if (KeyState[key]==KEY_STATE_DOWN) KeyState[key]=KEY_STATE_PRESSED;
 }
}
//----------------------------------------------------------------------------------------------------
//������� ������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_KeyDown(KEY key)
{ 
 if (KeyCounter[key]<MAX_KEY_COUNTER_VALUE) KeyCounter[key]++;
 KEYBOARD_KeyProcessing(key);
}
//----------------------------------------------------------------------------------------------------
//������� ��������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_KeyUp(KEY key)
{
 if (KeyCounter[key]>MIN_KEY_COUNTER_VALUE) KeyCounter[key]--;
 KEYBOARD_KeyProcessing(key);
}
//----------------------------------------------------------------------------------------------------
//�������� ��������� �������
//----------------------------------------------------------------------------------------------------
bool KEYBOARD_GetKeyState(KEY key)
{ 
 if (KeyCounter[key]>=KEY_PRESSED_MIN_COUNTER_VALUE) return(true);  
 return(false);
}
//----------------------------------------------------------------------------------------------------
//��������, ���� �� ������� ������� � ����������� � �������� ���� ����� ���������
//----------------------------------------------------------------------------------------------------
bool KEYBOARD_GetKeyPressedAndResetIt(KEY key)
{
 if (KeyState[key]==KEY_STATE_PRESSED)
 {
  KeyState[key]=KEY_STATE_UP;
  return(true);
 }
 return(false);
}