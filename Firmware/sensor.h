#ifndef SENSOR_H
#define SENSOR_H

//----------------------------------------------------------------------------------------------------
//����������
//----------------------------------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//----------------------------------------------------------------------------------------------------
//��������� �������
//----------------------------------------------------------------------------------------------------
void SENSOR_Init(void);//�������������
bool SENSOR_GetValue(int8_t *temp,int8_t *humidity);//�������� ���������

#endif
