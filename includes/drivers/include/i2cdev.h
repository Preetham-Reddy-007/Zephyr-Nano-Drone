/**
 *
 * i2cdev.h - Functions to write to I2C devices
 */

#ifndef __I2CDEV_H__
#define __I2CDEV_H__

#include <stdint.h>
#include <stdbool.h>

#include "drivers/include/i2c_drv.h"

#define I2CDEV_I2C1_PIN_SDA GPIO_Pin_7
#define I2CDEV_I2C1_PIN_SCL GPIO_Pin_6

#define I2CDEV_NO_MEM_ADDR  0xFF

#define I2C1_DEV  &deckBus
#define I2C3_DEV  &sensorsBus

// For compatibility
#define i2cdevWrite16 i2cdevWriteReg16
#define i2cdevRead16  i2cdevReadReg16

/**
 * Read bytes from an I2C peripheral
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevRead(const struct device *dev, uint16_t devAddress, uint32_t len, uint8_t *data);

/**
 * Read bytes from an I2C peripheral
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param memAddress  The internal address to read from, I2CDEV_NO_MEM_ADDR if none.
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevReadReg8(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint32_t len, uint8_t *data);

/**
 * Read bytes from an I2C peripheral with a 16bit internal reg/mem address
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param memAddress  The internal address to read from, I2CDEV_NO_MEM_ADDR if none.
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevReadReg16(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint32_t len, uint8_t *data);

/**
 * I2C device init function.
 * @param dev  Pointer to I2C peripheral to initialize.
 *
 * @return TRUE if initialization went OK otherwise FALSE.
 */
int i2cdevInit(const struct device *dev);

/**
 * Read a byte from an I2C peripheral
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param memAddress  The internal address to read from, I2CDEV_NO_MEM_ADDR if none.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevReadByte(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t *data);

/**
 * Read a bit from an I2C peripheral
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param memAddress  The internal address to read from, I2CDEV_NO_MEM_ADDR if none.
 * @param bitNum  The bit number 0 - 7 to read.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevReadBit(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint8_t bitNum, uint8_t *data);
/**
 * Read up to 8 bits from an I2C peripheral
 * @param dev  Pointer to I2C peripheral to read from
 * @param devAddress  The device address to read from
 * @param memAddress  The internal address to read from, I2CDEV_NO_MEM_ADDR if none.
 * @param bitStart The bit to start from, 0 - 7, MSB at 0
 * @param length  The number of bits to read, 1 - 8.
 * @param data  Pointer to a buffer to read the data to.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevReadBits(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                    uint8_t bitStart, uint8_t length, uint8_t *data);

/**
 * Write bytes to an I2C peripheral
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data from that will be written.
 *
 * @return TRUE if write was successful, otherwise FALSE.
 */
bool i2cdevWrite(const struct device *dev, uint16_t devAddress, uint16_t len, const uint8_t *data);

/**
 * Write bytes to an I2C peripheral
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param memAddress  The internal address to write to, I2CDEV_NO_MEM_ADDR if none.
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data from that will be written.
 *
 * @return TRUE if write was successful, otherwise FALSE.
 */
bool i2cdevWriteReg8(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                     uint32_t len, const uint8_t *data);

/**
 * Write bytes to an I2C peripheral with 16bit internal reg/mem address.
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param memAddress  The internal address to write to, I2CDEV_NO_MEM_ADDR if none.
 * @param len  Number of bytes to read.
 * @param data  Pointer to a buffer to read the data from that will be written.
 *
 * @return TRUE if write was successful, otherwise FALSE.
 */
bool i2cdevWriteReg16(const struct device *dev, uint16_t devAddress, uint8_t memAddress,
                      uint32_t len, const uint8_t *data);

/**
 * Write a byte to an I2C peripheral
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param memAddress  The internal address to write from, I2CDEV_NO_MEM_ADDR if none.
 * @param data  The byte to write.
 *
 * @return TRUE if write was successful, otherwise FALSE.
 */
bool i2cdevWriteByte(const struct device *dev, uint8_t devAddress, uint8_t memAddress,
                     uint8_t data);

/**
 * Write a bit to an I2C peripheral
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param memAddress  The internal address to write to, I2CDEV_NO_MEM_ADDR if none.
 * @param bitNum  The bit number, 0 - 7, to write.
 * @param data  The bit to write.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevWriteBit(const struct device *dev, uint8_t devAddress, uint8_t memAddress,
                    uint8_t bitNum, uint8_t data);

/**
 * Write up to 8 bits to an I2C peripheral
 * @param dev  Pointer to I2C peripheral to write to
 * @param devAddress  The device address to write to
 * @param memAddress  The internal address to write to, I2CDEV_NO_MEM_ADDR if none.
 * @param bitStart The bit to start from, 0 - 7.
 * @param length  The number of bits to write, 1 - 8.
 * @param data  The byte containing the bits to write.
 *
 * @return TRUE if read was successful, otherwise FALSE.
 */
bool i2cdevWriteBits(const struct device *dev, uint8_t devAddress, uint8_t memAddress,
                     uint8_t bitStart, uint8_t length, uint8_t data);

#endif //__I2CDEV_H__
