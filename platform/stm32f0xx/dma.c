/*
 * Copyright (c) 2016 Erik Gilling
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/dma.h>

#include <assert.h>
#include <kern/compiler.h>

#include <arch/arm/cm.h>
#include <kernel/event.h>
#include <platform/rcc.h>
#include <sys/types.h>

#include <stm32f0xx.h>

typedef DMA_Channel_TypeDef dma_channel_regs_t;

event_t dma_events[DMA_CHANNELS];

static void dma_channel_assert(dma_channel_t chan) {
    assert(DMA_CHANNEL_1 <= chan && chan < DMA_CHANNEL_7);
}

static dma_channel_regs_t *dma_get_channel(dma_channel_t chan) {
    unsigned long addr =
        DMA1_Channel1_BASE + (chan - 1) * 0x14;
    return (dma_channel_regs_t *)addr;
}

static event_t *dma_event(dma_channel_t chan) {
    return &dma_events[chan - 1];
}

// TODO(konkers): Separate out DMA IRQ handling by channel group.
void dma_irq(void) {
    arm_cm_irq_entry();
    bool resched = false;

    uint32_t sr = DMA1->ISR;

    size_t i;
    for (i = 0; i < countof(dma_events); i++) {
        uint32_t ch_sr = (sr >> (i * 4)) & 0xf;

        // TODO(konkers): Report error.
        if (ch_sr & (DMA_ISR_TCIF1 | DMA_ISR_TEIF1)) {
            event_signal(&dma_events[i], false);
            resched = true;
        }
    }
    DMA1->IFCR = sr;
    arm_cm_irq_exit(resched);
}

void stm32_DMA1_Channel1_IRQ(void) {
    dma_irq();
}

void stm32_DMA1_Channel2_3_IRQ(void) {
    dma_irq();
}

void stm32_DMA1_Channel4_5_6_7_IRQ(void) {
    dma_irq();
}

void dma_transfer_start(dma_channel_t chan,
                        uint32_t periph_addr,
                        uint32_t mem_addr,
                        uint16_t count,
                        uint32_t flags) {
    dma_channel_assert(chan);
    event_unsignal(dma_event(chan));

    dma_channel_regs_t *chan_regs = dma_get_channel(chan);
    chan_regs->CCR &= ~DMA_CCR_EN;

    chan_regs->CPAR = periph_addr;
    chan_regs->CMAR = mem_addr;
    chan_regs->CNDTR = count;
    chan_regs->CCR = flags | DMA_CCR_TEIE | DMA_CCR_TCIE | DMA_CCR_EN;
}

void dma_wait(dma_channel_t chan) {
    dma_channel_assert(chan);

    event_wait(dma_event(chan));
}

void dma_init(void) {
    stm32_rcc_set_enable(STM32_RCC_CLK_DMA, true);

    size_t i;
    for (i = 0; i < countof(dma_events); i++) {
        // Initialize all the channel events as signaled because they're idle.
        event_init(&dma_events[i], true, 0);
    }
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}
