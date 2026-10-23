#include "icm20608.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int init_error, read_error, write_failure_at=-1;
static uint8_t identity=ICM20608_WHO_AM_I_VALUE;
static uint8_t registers_written[16], values_written[16], sensor_bytes[14];
static unsigned int writes, reads;

int32_t BSP_I2C2_Init(void) { return init_error; }
int32_t BSP_I2C2_WriteReg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length)
{
    assert(address==ICM20608_I2C_ADDRESS && data && length==1 && writes<16);
    registers_written[writes]=(uint8_t)reg; values_written[writes]=*data;
    if ((int)writes++==write_failure_at) return -1;
    return 0;
}
int32_t BSP_I2C2_ReadReg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length)
{
    assert(address==ICM20608_I2C_ADDRESS && data); ++reads;
    if (read_error) return read_error;
    if (reg==0x75) { assert(length==1); *data=identity; }
    else { assert(reg==0x3b && length==14); memcpy(data,sensor_bytes,length); }
    return 0;
}
void HAL_Delay(uint32_t milliseconds) { assert(milliseconds<=100); }

static void reset(void)
{
    init_error=read_error=0; write_failure_at=-1;
    identity=ICM20608_WHO_AM_I_VALUE; writes=reads=0;
    memset(sensor_bytes,0,sizeof(sensor_bytes));
}

static void put_raw(unsigned int offset, int16_t value)
{ sensor_bytes[offset]=(uint8_t)((uint16_t)value>>8); sensor_bytes[offset+1]=(uint8_t)value; }

int main(void)
{
    static const uint8_t expected_registers[]={0x6b,0x6b,0x6c,0x19,0x1a,0x1b,0x1c,0x1d};
    static const uint8_t expected_values[]={0x80,0x01,0x00,0x00,0x03,0x00,0x00,0x03};
    ICM20608_Data_t data, before;
    reset(); assert(ICM20608_Init()==0 && writes==8);
    assert(memcmp(registers_written,expected_registers,8)==0);
    assert(memcmp(values_written,expected_values,8)==0); /* +/-2 g, +/-250 dps. */
    reset(); identity=0x69; assert(ICM20608_Init()==-1 && writes==0);
    reset(); init_error=-1; assert(ICM20608_Init()==-1 && reads==0 && writes==0);
    reset(); read_error=-1; assert(ICM20608_Init()==-1 && writes==0);
    for (int i=0;i<8;++i) {
        reset(); write_failure_at=i;
        assert(ICM20608_Init()==-1 && writes==(unsigned int)i+1);
    }
    reset();
    put_raw(0,INT16_MIN); put_raw(2,INT16_MAX); put_raw(4,-16384);
    put_raw(6,12345); /* Temperature bytes must not leak into gyro axes. */
    put_raw(8,INT16_MIN); put_raw(10,INT16_MAX); put_raw(12,-131);
    assert(ICM20608_ReadData(&data)==0);
    assert(data.accel_x_raw==INT16_MIN && data.accel_y_raw==INT16_MAX && data.accel_z_raw==-16384);
    assert(data.gyro_x_raw==INT16_MIN && data.gyro_y_raw==INT16_MAX && data.gyro_z_raw==-131);
    assert(data.accel_x_g==-2.0f && fabsf(data.accel_y_g-1.999938965f)<0.000001f && data.accel_z_g==-1.0f);
    assert(fabsf(data.gyro_x_dps+250.137405f)<0.00003f);
    assert(fabsf(data.gyro_y_dps-250.129776f)<0.00003f && data.gyro_z_dps==-1.0f);
    reset(); put_raw(4,16384); assert(ICM20608_ReadData(&data)==0);
    assert(data.accel_x_g==0 && data.accel_y_g==0 && data.accel_z_g==1 && data.gyro_x_dps==0);
    memset(&data,0xa5,sizeof(data)); memcpy(&before,&data,sizeof(data));
    read_error=-1; assert(ICM20608_ReadData(&data)==-1);
    assert(memcmp(&data,&before,sizeof(data))==0);
    assert(ICM20608_ReadData(NULL)==-1 && ICM20608_ReadID(NULL)==-1);
    puts("PASS: actual ICM20608 identity/range setup, I2C errors, signed axes and physical-unit scales");
    return 0;
}
