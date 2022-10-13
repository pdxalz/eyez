#include <zephyr/zephyr.h>
#include "servo.h"
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

const struct shell *chat_shell;
static enum WhichEye current_eye = Eye_both;

static int cmd_lighta(const struct shell *shell, size_t argc, char *argv[])
{
    cmd_print(">>> x light <<<");

    return 0;
}

static int cmd_lightb(const struct shell *shell, size_t argc, char *argv[])
{
    cmd_print(">>> y light <<<");

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(light_cmds,
                               SHELL_CMD_ARG(x, NULL, "Print client status", cmd_lighta, 1, 0),
                               SHELL_CMD_ARG(y, NULL, "Send a text message to the chat <message>", cmd_lightb, 1, 0),
                               SHELL_SUBCMD_SET_END);

static int cmd_eye_select(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        shell_help(shell);
        // shell returns 1 when help is printed
        return 1;
    }
    cmd_print("eye select %s", argv[1]);
    return 0;
}

static int cmd_eye_position(const struct shell *shell, size_t argc, char **argv)
{
    uint8_t position;
    if (argc == 1)
    {
        shell_help(shell);
        // shell returns 1 when help is printed
        return 1;
    }
   	position = atoi(argv[1]);

    eye_position(current_eye, position);

    cmd_print("eye postion %s", argv[1]);
    return 0;

}

static int cmd_eye_sequence(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        shell_help(shell);
        // shell returns 1 when help is printed
        return 1;
    }
    int n = atoi(argv[1]);
    start_sequence(n);
    cmd_print("eye sequence %s", argv[1]);
    return 0;

}

static int cmd_led_colors(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1)
    {
        shell_help(shell);
        // shell returns 1 when help is printed
        return 1;
    }

    cmd_print("LED sequence %s", argv[1]);
    return 0;
}

SHELL_CMD_ARG_REGISTER(e, NULL, "eye to move", cmd_eye_select, 1, 1);
SHELL_CMD_ARG_REGISTER(p, NULL, "pos to move to", cmd_eye_position, 1, 1);
SHELL_CMD_ARG_REGISTER(s, NULL, "sequence move", cmd_eye_sequence, 1, 1);
SHELL_CMD_ARG_REGISTER(l, NULL, "led", cmd_led_colors, 1, 1);

void command_init()
{
    chat_shell = shell_backend_uart_get_ptr();
    shell_use_colors_set(chat_shell, 0);
}
