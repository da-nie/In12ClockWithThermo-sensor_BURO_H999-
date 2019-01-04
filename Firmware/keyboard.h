#ifndef KEYBOARD_H
#define KEYBOARD_H

//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//----------------------------------------------------------------------------------------------------
//������������
//----------------------------------------------------------------------------------------------------

typedef enum
{
 KEY_0=0,
 KEY_1,
 KEY_2,
 KEY_3,
 KEY_4,
 KEY_5,
 KEY_6,
 KEY_7,
 KEY_8,
 KEY_9,
 KEY_STAR,
 KEY_SHARP,
 KEY_AMOUNT 
} KEY;

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void KEYBOARD_Init(void);//�������������
void KEYBOARD_Scan(void);//���������� ������������ ����������
bool KEYBOARD_GetKeyState(KEY key);//�������� ��������� �������
bool KEYBOARD_GetKeyPressedAndResetIt(KEY key);//��������, ���� �� ������� ������� � ����������� � �������� ���� ����� ���������

#endif
