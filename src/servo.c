#include <zephyr/zephyr.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

#include "servo.h"

static const struct pwm_dt_spec servo0 = PWM_DT_SPEC_GET(DT_NODELABEL(servo0));
static const struct pwm_dt_spec servo1 = PWM_DT_SPEC_GET(DT_NODELABEL(servo1));
static const struct pwm_dt_spec servo2 = PWM_DT_SPEC_GET(DT_NODELABEL(servo2));
static const struct pwm_dt_spec servo3 = PWM_DT_SPEC_GET(DT_NODELABEL(servo3));
static const uint32_t min_pulse = DT_PROP(DT_NODELABEL(servo0), min_pulse);
static const uint32_t max_pulse = DT_PROP(DT_NODELABEL(servo0), max_pulse);

// 100us steps and 40ms update rate is the max speed
#define STEP PWM_USEC(50)

enum direction
{
    DOWN,
    UP,
};

#define CALL_DEPTH 4
#define SEQ_END 0xffff
#define SEQ_SUB(X) (0xff00 | X)
#define LAST_MOVE_COMMAND 9999

uint16_t seq1[] = {505, 1515, 2525, 3535, 4545, 5555, 5656, 5757, 5858, 5959,
                   6969, 7979, 8888, 9797, 9696, 9595, 9494, 8383, 7272, 6161, 5050,
                   5051, 5052, 5053, 5054, 5055, 5056, 5057, 5058, 5059, 5057, 5055, 5053, 5051, SEQ_END};

uint16_t seqRL[] = {505, 1515, 2525, 3535, 4545, 5555, 6565, 7575, 8585, 9595, SEQ_END};
uint16_t seqLR[] = {9595, 8585, 7575, 6565, 5555, 4545, 3535, 2525, 1515, 505, SEQ_END};
uint16_t seqLRoll[] = {505, 1616, 2727, 3838, 4949, 5959, 6868, 7777, 8686, 9595, SEQ_END};
uint16_t seqRRoll[] = {9595, 8686, 7777, 6868, 5959, 4949, 3838, 2727, 1616, 505, SEQ_END};
uint16_t seqWonkey[] = {9595, 8695, 7795, 6895, 5995, 4995, 3895, 2795, 1695, 595,
                        495, 1395, 2295, 3195, 4095, 5095, 6195, 7295, 8395, 9495, 9595, SEQ_END};
uint16_t seqSuffle[] = {SEQ_SUB(1), SEQ_SUB(2), SEQ_SUB(1), SEQ_SUB(2), SEQ_END};
uint16_t seqSuffleRoll[] = {SEQ_SUB(1), SEQ_SUB(2), SEQ_SUB(3), SEQ_SUB(4), SEQ_SUB(3), SEQ_SUB(4), SEQ_SUB(1), SEQ_SUB(2), SEQ_END};
uint16_t seqWonkeyRoll[] = {SEQ_SUB(2), SEQ_SUB(6), SEQ_SUB(1), SEQ_END};

uint16_t *list_sequences[] = {seq1, seqLR, seqRL, seqRRoll, seqLRoll, seqRRoll, seqWonkey, seqSuffle, seqSuffleRoll, seqWonkeyRoll};

uint32_t pulse_width = min_pulse;
enum direction dir = UP;
// static bool sequence = false;

static struct
{
    uint16_t id;
    uint16_t step;
} seq[CALL_DEPTH];
int stack;

int32_t servo_timeout;

void servo_init()
{
    stack = -1;
    servo_timeout = 150;

    if (!device_is_ready(servo0.dev))
    {
        printk("Error: PWM device %s is not ready\n", servo0.dev->name);
        return;
    }

    if (!device_is_ready(servo1.dev))
    {
        printk("Error: PWM device %s is not ready\n", servo1.dev->name);
        return;
    }

    if (!device_is_ready(servo2.dev))
    {
        printk("Error: PWM device %s is not ready\n", servo2.dev->name);
        return;
    }

    if (!device_is_ready(servo3.dev))
    {
        printk("Error: PWM device %s is not ready\n", servo3.dev->name);
        return;
    }
}

void start_sequence(int sequence)
{
    stack = 0;
    seq[0].id = sequence;
    seq[0].step = 0;
}

void eye_position(enum WhichEye eye, uint8_t pos)
{
    uint32_t p1;
    uint32_t p2;

    stack = -1;
    switch (pos)
    {
    case 9:
        p1 = min_pulse;
        p2 = min_pulse;
        break;
    case 8:
        p1 = (max_pulse + min_pulse) / 2;
        p2 = min_pulse;
        break;
    case 7:
        p1 = max_pulse;
        p2 = min_pulse;
        break;
    case 6:
        p1 = min_pulse;
        p2 = (max_pulse + min_pulse) / 2;
        break;
    case 5:
        p1 = (max_pulse + min_pulse) / 2;
        p2 = (max_pulse + min_pulse) / 2;
        break;
    case 4:
        p1 = max_pulse;
        p2 = (max_pulse + min_pulse) / 2;
        break;
    case 3:
        p1 = min_pulse;
        p2 = max_pulse;
        break;
    case 2:
        p1 = (max_pulse + min_pulse) / 2;
        p2 = max_pulse;
        break;
    case 1:
        p1 = max_pulse;
        p2 = max_pulse;

        break;
    default:
        p1 = (max_pulse + min_pulse) / 2;
        p2 = (max_pulse + min_pulse) / 2;
        break;
    }
    printk("pos %d, %d\n", p1, p2);

    if (eye != Eye_right)
    {
        pwm_set_pulse_dt(&servo0, p1);
        pwm_set_pulse_dt(&servo1, p2);
    }
    if (eye != Eye_left)
    {
        pwm_set_pulse_dt(&servo2, p1);
        pwm_set_pulse_dt(&servo3, p2);
    }
}

void servo_update()
{
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint32_t p4;

    if (stack < 0)
        return;

    uint16_t id = seq[stack].id;
    uint16_t step = seq[stack].step;
    uint16_t *pattern = list_sequences[id];

    while (pattern[step] > LAST_MOVE_COMMAND)
    {
        if (pattern[step] == SEQ_END)
        {
            --stack;
            if (stack < 0)
            {
                return;
            }
            id = seq[stack].id;
            step = seq[stack].step;
            pattern = list_sequences[id];
        }
        else if (pattern[step] >= SEQ_SUB(0))
        {
//            __ASSERT(pattern[step] & 0x00ff < ARRAY_SIZE(list_sequences), "SEQ_SUB error");
            ++stack;
 //           __ASSERT(stack < CALL_DEPTH, "CALL_DEPTH error");
            id = pattern[step] & 0x00ff;
            seq[stack].id = id;
            step = 0xFFFF;
            seq[stack].step = step;
            pattern = list_sequences[id];
        }
        seq[stack].step = ++step;
    }
    p1 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] % 10);
    p2 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 10 % 10);
    p3 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 100 % 10);
    p4 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 1000 % 10);

    seq[stack].step = ++step;

    pwm_set_pulse_dt(&servo0, p1);
    pwm_set_pulse_dt(&servo1, p2);
    pwm_set_pulse_dt(&servo2, p3);
    pwm_set_pulse_dt(&servo3, p4);
}

