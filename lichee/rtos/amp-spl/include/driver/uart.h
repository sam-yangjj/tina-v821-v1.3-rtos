/*
 * include/drivers/uart.h
 *
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: Sunny <Sunny@allwinnertech.com>
 *
 */
#ifndef __UART_H__
#define __UART_H__

void uart_init(unsigned long iobase);
extern void uart_putc(char ch);
extern char uart_getc(void);

#endif  /* __UART_H__ */
