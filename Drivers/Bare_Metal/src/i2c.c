#include "i2c.h"

void I2C1_Init(void)
{
    //Enable GPIOB and I2C1 clocks
    RCC->AHB1ENR |= 0x00000002;
    RCC->APB1ENR |= 0x00200000;

    //Disable I2C1
    I2C1->CR1 &= ~0x0001;

    //Configure PB8 (SCL) and PB9 (SDA)
    //Alternate function mode
    GPIOB->MODER &= ~0x000F0000;
    GPIOB->MODER |=  0x000A0000;

    //Open-drain output
    GPIOB->OTYPER |= 0x00000300;

    //High-speed GPIO
    GPIOB->OSPEEDR &= ~0x000F0000;
    GPIOB->OSPEEDR |=  0x000A0000;

    //No internal pull-up/down
    GPIOB->PUPDR &= ~0x000F0000;

    //Alternate function AF4
    GPIOB->AFR[1] &= ~0x000000FF;
    GPIOB->AFR[1] |=  0x00000044;

    //APB1 clock = 16 MHz
    I2C1->CR2 = 0x0010;

    //Configure own address register
    I2C1->OAR1 = 0x4000;

    //Set I2C speed to 100 kHz
    I2C1->CCR = 0x0050;

    //Configure maximum rise time
    I2C1->TRISE = 0x0011;

    //Enable I2C1
    I2C1->CR1 |= 0x0001;

    //Enable acknowledgment
    I2C1->CR1 |= 0x0400;
}

// Wait for a flag in SR1
static bool I2C1_WaitFlag(uint16_t flag)
{
    uint32_t timeout = 0x000FFFFF;

    while (timeout--)
    {
        uint16_t status = I2C1->SR1;

        // Bus error, arbitration lost, NACK, overrun
        if (status & 0x0F00)
            return false;

        if (status & flag)
            return true;
    }

    return false;
}

// Wait for bus to become available
static bool I2C1_WaitIdle(void)
{
    uint32_t timeout = 0x000FFFFF;

    while ((I2C1->SR2 & 0x0002) ||
           (I2C1->CR1 & 0x0200))
    {
        if (timeout-- == 0x00000000)
            return false;
    }

    return true;
}

bool I2C1_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    if (data == 0 || length == 0 || address > 0x7F)
        return false;

    //Wait until bus is free
    if (!I2C1_WaitIdle())
        return false;

    //Generate START
    I2C1->CR1 |= 0x0100;

    //Wait for START condition
    if (!I2C1_WaitFlag(0x0001))
        goto error;

    //Send 7-bit address with write bit (0)
    I2C1->DR = address << 1;

    //Wait for address acknowledgment
    if (!I2C1_WaitFlag(0x0002))
        goto error;

    //Clear ADDR flag
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    //Transmit bytes
    for (uint16_t i = 0; i < length; i++)
    {
        // Wait until data register is empty
        if (!I2C1_WaitFlag(0x0080))
            goto error;

        I2C1->DR = data[i];
    }

    //Wait until last byte finishes
    if (!I2C1_WaitFlag(0x0004))
        goto error;

    //Generate STOP
    I2C1->CR1 |= 0x0200;

    return true;

error:
    // Generate STOP if still controller and permitted
    if ((I2C1->SR2 & 0x0001) &&
        !(I2C1->CR1 & 0x0300))
        I2C1->CR1 |= 0x0200;

    // Clear error flags
    I2C1->SR1 &= ~0x0F00;

    return false;
}


bool I2C1_Read(uint8_t address, uint8_t *data, uint16_t length)
{
    if (data == 0 || length == 0 || address > 0x7F)
        return false;

    // Wait until bus is free
    if (!I2C1_WaitIdle())
        return false;

    //Prepare ACK and POS
    I2C1->CR1 &= ~0x0800;
    I2C1->CR1 |=  0x0400;

    //Generate START
    I2C1->CR1 |= 0x0100;

    if (!I2C1_WaitFlag(0x0001))
        goto error;

    //Send 7-bit address with read bit (1)
    I2C1->DR = (address << 1) | 0x01;

    //Wait for address acknowledgment
    if (!I2C1_WaitFlag(0x0002))
        goto error;

    /*don't understand 100% 
    but read protocal is different 
    based on the # of bytes recieved
    */
    //Receive one byte
    if (length == 0x01)
    {
        //interrupt messing with code?
        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        // Disable ACK before clearing ADDR
        I2C1->CR1 &= ~0x0400;

        (void)I2C1->SR1;
        (void)I2C1->SR2;

        // Generate STOP
        I2C1->CR1 |= 0x0200;

        __set_PRIMASK(irq);

        if (!I2C1_WaitFlag(0x0040))
            goto error;

        data[0] = (uint8_t)I2C1->DR;
    }
    //Receive two bytes
    else if (length == 0x02)
    {
        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        // Set POS and disable ACK
        I2C1->CR1 |=  0x0800;
        I2C1->CR1 &= ~0x0400;

        (void)I2C1->SR1;
        (void)I2C1->SR2;

        __set_PRIMASK(irq);

        if (!I2C1_WaitFlag(0x0004))
            goto error;

        irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 |= 0x0200;

        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);
    }
    //Receive three or more bytes
    else
    {
        (void)I2C1->SR1;
        (void)I2C1->SR2;

        uint16_t i = 0;

        while ((length - i) > 0x03)
        {
            if (!I2C1_WaitFlag(0x0040))
                goto error;

            data[i++] = (uint8_t)I2C1->DR;
        }

        // Three bytes remaining
        if (!I2C1_WaitFlag(0x0004))
            goto error;

        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 &= ~0x0400;
        data[i++] = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);

        // Wait for final two bytes
        if (!I2C1_WaitFlag(0x0004))
            goto error;

        irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 |= 0x0200;

        data[i++] = (uint8_t)I2C1->DR;
        data[i]   = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);
    }

    // Wait for STOP to finish before changing CR1
    uint32_t timeout = 0x000FFFFF;

    while (I2C1->CR1 & 0x0200)
    {
        if (timeout-- == 0x00000000)
            return false;
    }

    // Restore ACK and POS for future transfers
    I2C1->CR1 &= ~0x0800;
    I2C1->CR1 |=  0x0400;

    return true;

error:
    if ((I2C1->SR2 & 0x0001) &&
        !(I2C1->CR1 & 0x0300))
        I2C1->CR1 |= 0x0200;

    I2C1->SR1 &= ~0x0F00;

    return false;
}


bool I2C1_ReadRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length)
{
    if (data == 0 || length == 0 || address > 0x7F)
        return false;

    //Wait until bus is free
    if (!I2C1_WaitIdle())
        return false;

    // Enable ACK, disable POS
    I2C1->CR1 &= ~0x0800;
    I2C1->CR1 |=  0x0400;

    //Generate START
    I2C1->CR1 |= 0x0100;

    if (!I2C1_WaitFlag(0x0001))
        goto error;

    //Send device address with write bit
    I2C1->DR = (uint16_t)address * 0x02;

    if (!I2C1_WaitFlag(0x0002))
        goto error;

    //Clear ADDR
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    //Send register address
    if (!I2C1_WaitFlag(0x0080))
        goto error;

    I2C1->DR = reg;

    if (!I2C1_WaitFlag(0x0004))
        goto error;

    //Generate repeated START
    I2C1->CR1 |= 0x0100;

    if (!I2C1_WaitFlag(0x0001))
        goto error;

    //Send device address with read bit
    I2C1->DR = ((uint16_t)address * 0x02) | 0x01;

    if (!I2C1_WaitFlag(0x0002))
        goto error;

    //Receive one byte
    if (length == 0x01)
    {
        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        // Disable ACK before clearing ADDR
        I2C1->CR1 &= ~0x0400;

        (void)I2C1->SR1;
        (void)I2C1->SR2;

        // Generate STOP
        I2C1->CR1 |= 0x0200;

        __set_PRIMASK(irq);

        if (!I2C1_WaitFlag(0x0040))
            goto error;

        data[0] = (uint8_t)I2C1->DR;
    }
    //Receive two bytes
    else if (length == 0x02)
    {
        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        // Set POS and disable ACK
        I2C1->CR1 |=  0x0800;
        I2C1->CR1 &= ~0x0400;

        (void)I2C1->SR1;
        (void)I2C1->SR2;

        __set_PRIMASK(irq);

        if (!I2C1_WaitFlag(0x0004))
            goto error;

        irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 |= 0x0200;

        data[0] = (uint8_t)I2C1->DR;
        data[1] = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);
    }
    //Receive three or more bytes
    else
    {
        (void)I2C1->SR1;
        (void)I2C1->SR2;

        uint16_t i = 0;

        while ((length - i) > 0x03)
        {
            if (!I2C1_WaitFlag(0x0040))
                goto error;

            data[i++] = (uint8_t)I2C1->DR;
        }

        // Three bytes remaining
        if (!I2C1_WaitFlag(0x0004))
            goto error;

        uint32_t irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 &= ~0x0400;
        data[i++] = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);

        // Wait for final two bytes
        if (!I2C1_WaitFlag(0x0004))
            goto error;

        irq = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 |= 0x0200;

        data[i++] = (uint8_t)I2C1->DR;
        data[i] = (uint8_t)I2C1->DR;

        __set_PRIMASK(irq);
    }

    //Wait for STOP to complete
    if (!I2C1_WaitIdle())
        goto error;

    // 12. Restore ACK and POS
    I2C1->CR1 &= ~0x0800;
    I2C1->CR1 |=  0x0400;

    return true;

error:
    // Request STOP if we still control the bus
    if ((I2C1->SR2 & 0x0001) &&
        !(I2C1->CR1 & 0x0300))
        I2C1->CR1 |= 0x0200;

    // Clear error flags
    I2C1->SR1 &= ~0x0F00;

    return false;
}
