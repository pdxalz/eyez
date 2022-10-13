#ifndef COMMAND_H__
#define COMMAND_H__

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

void command_init();

extern const struct shell *chat_shell;

#define cmd_print(_ft,__VA_ARGS__...) shell_fprintf(chat_shell, SHELL_NORMAL, _ft "\n", ## __VA_ARGS__)

#endif // COMMAND_H__