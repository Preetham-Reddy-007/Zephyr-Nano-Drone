/**
 * i2cdev.c - Functions to write to I2C devices
 */
#define DEBUG_MODULE "I2CDEV"

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#include "i2cdev.h"
#include "i2c_drv.h"
#include "nvicconf.h"

int i2cdevInit(const struct device *dev)
{
  // i2cdrvInit(dev);

  if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}
  return 1;
}

bool i2cdevReadByte(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t *data)
{
  return i2cdevReadReg8(dev, devAddress, memAddress, 1, data);
}

bool i2cdevReadBit(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint8_t bitNum, uint8_t *data)
{
  uint8_t byte;
  bool status;

  status = i2cdevReadReg8(dev, devAddress, memAddress, 1, &byte);
  *data = byte & (1 << bitNum);

  return status;
}

bool i2cdevReadBits(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t bitStart, uint8_t length, uint8_t *data)
{
  bool status;
  uint8_t byte;

  if ((status = i2cdevReadByte(dev, devAddress, memAddress, &byte)) == true)
  {
      uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
      byte &= mask;
      byte >>= (bitStart - length + 1);
      *data = byte;
  }
  return status;
}

bool i2cdevRead(const struct device *dev, uint16_t devAddress, uint32_t len, uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessage(&message, devAddress, i2cRead, len, data);

  int ret_code = i2c_read(dev, data, len, devAddress);
  
  if(ret_code == 0){
    return true;
  }

  return false;
}

bool i2cdevReadReg8(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint32_t len, uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessageIntAddr(&message, devAddress, false, memAddress,
  //                           i2cRead, len, data);
  int ret_code = i2c_burst_read(dev, devAddress, memAddress, data, len);

  if(ret_code == 0){
    return true;
  }

  return false;
}

bool i2cdevReadReg16(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint32_t len, uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessageIntAddr(&message, devAddress, true, memAddress,
  //                         i2cRead, len, data);

  int ret_code = i2c_burst_read(dev, devAddress, memAddress, data, len);
  if(ret_code == 0){
    return true;
  }

  return false;
}

bool i2cdevWriteByte(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t data)
{
  return i2cdevWriteReg8(dev, devAddress, memAddress, 1, &data);
}

bool i2cdevWriteBit(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t bitNum, uint8_t data)
{
    uint8_t byte;
    i2cdevReadByte(dev, devAddress, memAddress, &byte);
    byte = (data != 0) ? (byte | (1 << bitNum)) : (byte & ~(1 << bitNum));
    return i2cdevWriteByte(dev, devAddress, memAddress, byte);
}

bool i2cdevWriteBits(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint8_t bitStart, uint8_t length, uint8_t data)
{
  bool status;
  uint8_t byte;

  if ((status = i2cdevReadByte(dev, devAddress, memAddress, &byte)) == true)
  {
      uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
      data <<= (bitStart - length + 1); // shift data into correct position
      data &= mask; // zero all non-important bits in data
      byte &= ~(mask); // zero all important bits in existing byte
      byte |= data; // combine data with existing byte
      status = i2cdevWriteByte(dev, devAddress, memAddress, byte);
  }

  return status;
}

bool i2cdevWrite(const struct device *dev, uint16_t devAddress, uint16_t len, const uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessage(&message, devAddress, i2cWrite, len, data);
  int ret_code = i2c_write(dev, data, len, devAddress);

  if(ret_code == 0){
    return true;
  }

  return false;
}

bool i2cdevWriteReg8(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint32_t len, const uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessageIntAddr(&message, devAddress, false, memAddress,
  //                            i2cWrite, len, data);

  int ret_code = i2c_burst_write(dev, devAddress, memAddress, data, len);

  if(ret_code == 0){
    return true;
  }

  return false;
}

bool i2cdevWriteReg16(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                      uint32_t len, const uint8_t *data)
{
  // I2cMessage message;

  // i2cdrvCreateMessageIntAddr(&message, devAddress, true, memAddress,
  //                            i2cWrite, len, data);

  int ret_code = i2c_burst_write(dev, devAddress, memAddress, data, len);

  if(ret_code == 0){
    return true;
  }

  return false; 
}
