#include "lzport/gpio.h"

#include "ch32h417_exti.h"
#include "ch32h417_gpio.h"
#include "ch32h417_rcc.h"

#define GPIO_PORT_STRIDE      0x400U
#define GPIO_REG(port)        ((GPIO_TypeDef *)(GPIOA_BASE + ((uint32_t)(port) * GPIO_PORT_STRIDE)))
#define GPIO_CLOCK(port)      (RCC_HB2Periph_GPIOA << (uint32_t)(port))
#define GPIO_IRQ_PRIORITY     0xA0U

typedef struct
{
    lzport_gpio_irq_cb callback;
    void *user;
    lzport_gpio_port port;
    uint8_t attached;
} gpio_irq_state;

static gpio_irq_state g_irq[LZPORT_GPIO_PIN_COUNT];

static int gpio_pin_valid(lzport_gpio_port port, lzport_gpio_pin pin)
{
    return ((uint32_t)port < LZPORT_GPIO_PORT_COUNT) &&
           ((uint32_t)pin < LZPORT_GPIO_PIN_COUNT);
}

static uint16_t gpio_pin_mask(lzport_gpio_pin pin)
{
    return (uint16_t)(1U << (uint32_t)pin);
}

static GPIOMode_TypeDef gpio_input_mode(lzport_gpio_pull pull)
{
    if (pull == LZPORT_GPIO_PULL_UP) {
        return GPIO_Mode_IPU;
    }
    if (pull == LZPORT_GPIO_PULL_DOWN) {
        return GPIO_Mode_IPD;
    }
    return GPIO_Mode_IN_FLOATING;
}

static void gpio_clock_enable(lzport_gpio_port port, int afio)
{
    uint32_t clock = GPIO_CLOCK(port);

    if (afio != 0) {
        clock |= RCC_HB2Periph_AFIO;
    }
    RCC_HB2PeriphClockCmd(clock, ENABLE);
}

void lzport_gpio_mode_input(lzport_gpio_port port, lzport_gpio_pin pin,
                            lzport_gpio_pull pull)
{
    GPIO_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin) || ((uint32_t)pull >= LZPORT_GPIO_PULL_COUNT)) {
        return;
    }
    gpio_clock_enable(port, 0);
    init.GPIO_Pin = gpio_pin_mask(pin);
    init.GPIO_Speed = GPIO_Speed_Low;
    init.GPIO_Mode = gpio_input_mode(pull);
    GPIO_Init(GPIO_REG(port), &init);
}

void lzport_gpio_mode_analog(lzport_gpio_port port, lzport_gpio_pin pin)
{
    GPIO_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin)) {
        return;
    }
    gpio_clock_enable(port, 0);
    init.GPIO_Pin = gpio_pin_mask(pin);
    init.GPIO_Speed = GPIO_Speed_Low;
    init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIO_REG(port), &init);
}

void lzport_gpio_mode_output(lzport_gpio_port port, lzport_gpio_pin pin,
                             lzport_gpio_speed speed, lzport_gpio_otype otype)
{
    GPIO_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin) || ((uint32_t)speed >= LZPORT_GPIO_SPEED_COUNT) ||
        ((uint32_t)otype >= LZPORT_GPIO_OTYPE_COUNT)) {
        return;
    }
    gpio_clock_enable(port, 0);
    init.GPIO_Pin = gpio_pin_mask(pin);
    init.GPIO_Speed = (GPIOSpeed_TypeDef)speed;
    init.GPIO_Mode = (otype == LZPORT_GPIO_OPEN_DRAIN) ? GPIO_Mode_Out_OD
                                                       : GPIO_Mode_Out_PP;
    GPIO_Init(GPIO_REG(port), &init);
}

void lzport_gpio_mode_af_input(lzport_gpio_port port, lzport_gpio_pin pin,
                               lzport_gpio_af af, lzport_gpio_pull pull)
{
    GPIO_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin) || ((uint32_t)af >= LZPORT_GPIO_AF_COUNT) ||
        ((uint32_t)pull >= LZPORT_GPIO_PULL_COUNT)) {
        return;
    }
    gpio_clock_enable(port, 1);
    GPIO_PinAFConfig(GPIO_REG(port), (uint8_t)pin, (uint8_t)af);
    init.GPIO_Pin = gpio_pin_mask(pin);
    init.GPIO_Speed = GPIO_Speed_Low;
    init.GPIO_Mode = gpio_input_mode(pull);
    GPIO_Init(GPIO_REG(port), &init);
}

void lzport_gpio_mode_af_output(lzport_gpio_port port, lzport_gpio_pin pin,
                                lzport_gpio_af af, lzport_gpio_speed speed,
                                lzport_gpio_otype otype)
{
    GPIO_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin) || ((uint32_t)af >= LZPORT_GPIO_AF_COUNT) ||
        ((uint32_t)speed >= LZPORT_GPIO_SPEED_COUNT) ||
        ((uint32_t)otype >= LZPORT_GPIO_OTYPE_COUNT)) {
        return;
    }
    gpio_clock_enable(port, 1);
    GPIO_PinAFConfig(GPIO_REG(port), (uint8_t)pin, (uint8_t)af);
    init.GPIO_Pin = gpio_pin_mask(pin);
    init.GPIO_Speed = (GPIOSpeed_TypeDef)speed;
    init.GPIO_Mode = (otype == LZPORT_GPIO_OPEN_DRAIN) ? GPIO_Mode_AF_OD
                                                       : GPIO_Mode_AF_PP;
    GPIO_Init(GPIO_REG(port), &init);
}

void lzport_gpio_set(lzport_gpio_port port, lzport_gpio_pin pin)
{
    if (gpio_pin_valid(port, pin)) {
        GPIO_REG(port)->BSHR = gpio_pin_mask(pin);
    }
}

void lzport_gpio_reset(lzport_gpio_port port, lzport_gpio_pin pin)
{
    if (gpio_pin_valid(port, pin)) {
        GPIO_REG(port)->BCR = gpio_pin_mask(pin);
    }
}

void lzport_gpio_toggle(lzport_gpio_port port, lzport_gpio_pin pin)
{
    GPIO_TypeDef *gpio;
    uint16_t mask;

    if (!gpio_pin_valid(port, pin)) {
        return;
    }
    gpio = GPIO_REG(port);
    mask = gpio_pin_mask(pin);
    if ((gpio->OUTDR & mask) != 0U) {
        gpio->BCR = mask;
    } else {
        gpio->BSHR = mask;
    }
}

void lzport_gpio_write(lzport_gpio_port port, lzport_gpio_pin pin,
                       lzport_gpio_level level)
{
    if (level == LZPORT_GPIO_HIGH) {
        lzport_gpio_set(port, pin);
    } else if (level == LZPORT_GPIO_LOW) {
        lzport_gpio_reset(port, pin);
    }
}

lzport_gpio_level lzport_gpio_read(lzport_gpio_port port, lzport_gpio_pin pin)
{
    if (!gpio_pin_valid(port, pin)) {
        return LZPORT_GPIO_LOW;
    }
    return ((GPIO_REG(port)->INDR & gpio_pin_mask(pin)) != 0U)
               ? LZPORT_GPIO_HIGH
               : LZPORT_GPIO_LOW;
}

lzport_status lzport_gpio_irq_attach(lzport_gpio_port port,
                                     lzport_gpio_pin pin,
                                     lzport_gpio_irq_edge edge,
                                     lzport_gpio_irq_cb callback, void *user)
{
    EXTI_InitTypeDef init = {0};
    IRQn_Type irq;

    if (!gpio_pin_valid(port, pin) ||
        ((uint32_t)edge >= LZPORT_GPIO_IRQ_EDGE_COUNT) || (callback == 0)) {
        return LZPORT_EINVAL;
    }
    if ((g_irq[pin].attached != 0U) && (g_irq[pin].port != port)) {
        return LZPORT_EBUSY;
    }

    gpio_clock_enable(port, 1);
    GPIO_EXTILineConfig((uint8_t)port, (uint8_t)pin);
    init.EXTI_Line = 1UL << (uint32_t)pin;
    init.EXTI_Mode = EXTI_Mode_Interrupt;
    init.EXTI_Trigger = (edge == LZPORT_GPIO_IRQ_RISING)
                            ? EXTI_Trigger_Rising
                            : ((edge == LZPORT_GPIO_IRQ_FALLING)
                                   ? EXTI_Trigger_Falling
                                   : EXTI_Trigger_Rising_Falling);
    init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&init);
    EXTI_ClearITPendingBit(init.EXTI_Line);

    g_irq[pin].callback = callback;
    g_irq[pin].user = user;
    g_irq[pin].port = port;
    g_irq[pin].attached = 1U;
    irq = ((uint32_t)pin < 8U) ? EXTI7_0_IRQn : EXTI15_8_IRQn;
    NVIC_ClearPendingIRQ(irq);
    NVIC_SetPriority(irq, GPIO_IRQ_PRIORITY);
    NVIC_EnableIRQ(irq);
    return LZPORT_OK;
}

lzport_status lzport_gpio_irq_detach(lzport_gpio_port port,
                                     lzport_gpio_pin pin)
{
    EXTI_InitTypeDef init = {0};

    if (!gpio_pin_valid(port, pin) || (g_irq[pin].attached == 0U) ||
        (g_irq[pin].port != port)) {
        return LZPORT_EINVAL;
    }
    init.EXTI_Line = 1UL << (uint32_t)pin;
    init.EXTI_Mode = EXTI_Mode_Interrupt;
    init.EXTI_Trigger = EXTI_Trigger_Rising;
    init.EXTI_LineCmd = DISABLE;
    EXTI_Init(&init);
    EXTI_ClearITPendingBit(init.EXTI_Line);
    g_irq[pin].callback = 0;
    g_irq[pin].user = 0;
    g_irq[pin].attached = 0U;
    return LZPORT_OK;
}

static void gpio_irq_dispatch(uint8_t first, uint8_t last)
{
    uint8_t pin;

    for (pin = first; pin <= last; ++pin) {
        uint32_t line = 1UL << pin;
        lzport_gpio_irq_cb callback;
        void *user;

        if (EXTI_GetITStatus(line) == RESET) {
            continue;
        }
        EXTI_ClearITPendingBit(line);
        if (g_irq[pin].attached == 0U) {
            continue;
        }
        callback = g_irq[pin].callback;
        user = g_irq[pin].user;
        callback(g_irq[pin].port, (lzport_gpio_pin)pin, user);
    }
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void)
{
    gpio_irq_dispatch(0U, 7U);
}

void EXTI15_8_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI15_8_IRQHandler(void)
{
    gpio_irq_dispatch(8U, 15U);
}
