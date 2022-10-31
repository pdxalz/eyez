#include <zephyr/zephyr.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <math.h>
#include "servo.h"
#include "led.h"

#if 1 // printk info
#define ZPR printk
#else
static void ZPR(char *fmt, ...)
{
}
#endif

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
#define SEQ_LOOP 0xfffe
#define SEQ_NXTCLR 0xfffd
#define SEQ_SUB(X) (0xff00 | X)
#define SEQ_DLY(X) (0xfe00 | X)
#define SEQ_DLYS(X) (0xfd00 | X)
#define SEQ_DLYR(X) (0xfc00 | X)
#define SEQ_SPD(X) (0xfb00 | X)
#define SEQ_CLR(X) (0xfa00 | X)
#define SEQ_MOV(X) (10000 + (X % 10000))
#define SEQ_ARC(X) (20000 + (X % 10000))
#define LAST_MOVE_COMMAND 29999

#define SPD_NORMAL SEQ_SPD(10)
// uint16_t seqCenter[] = {5555, SEQ_END};
// uint16_t seqRoll[] = {SEQ_ARC(1515), SEQ_ARC(5959), SEQ_ARC(9595), SEQ_ARC(5151), SEQ_ARC(1515), SEQ_ARC(5555), SEQ_END};
// uint16_t seqRL[] = {SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959), SEQ_END};
// uint16_t seqFunny[] = {SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959), SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959), SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959),
// SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959), SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959), SEQ_NXTCLR, SEQ_MOV(5151), SEQ_MOV(5959),
// SEQ_ARC(1515), SEQ_ARC(5959), SEQ_ARC(9595), SEQ_ARC(5151), SEQ_ARC(1515), SEQ_ARC(5555),  SEQ_LOOP};
// uint16_t seqCrss[] = {SEQ_MOV(5151), SEQ_MOV(5959),  SEQ_LOOP};
// uint16_t seqUncross[] = {SEQ_MOV(9191), SEQ_MOV(1515), SEQ_MOV(9999), SEQ_MOV(1515), SEQ_DLY(20), SEQ_END};
// uint16_t seqCross[] = {SEQ_SPD(6), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SUB(5), SEQ_SPD(10), 5555, SEQ_END};
// uint16_t seqColor[] = {SEQ_SPD(6), SEQ_SUB(5), SEQ_CLR(0), SEQ_SUB(5), SEQ_CLR(1), SEQ_SUB(5), SEQ_CLR(2), SEQ_SUB(5), SEQ_CLR(3), SEQ_SUB(5), SEQ_CLR(4), SEQ_SUB(5), SEQ_CLR(5), SEQ_SUB(5), SEQ_CLR(6), SEQ_SUB(5), SEQ_CLR(7), SEQ_SUB(5), SEQ_CLR(8), SEQ_SUB(5), SEQ_CLR(9), SEQ_SUB(5), SEQ_CLR(10), SEQ_SUB(5), SEQ_CLR(11), SEQ_SUB(5), SEQ_CLR(12), SEQ_SPD(10), 5555, SEQ_END};
// uint16_t seqDemo[] = {SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(1), SEQ_SUB(1), SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(2), SEQ_SUB(1), SEQ_SUB(0), SEQ_END};
// uint16_t seqWonkey[] = {SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_SUB(8), SEQ_END};
// uint16_t seqSuffle[] = {SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_SUB(9), SEQ_END};
// uint16_t seqSuffleRoll[] = {SEQ_SUB(1), SEQ_SUB(2), SEQ_SUB(3), SEQ_SUB(4), SEQ_SUB(3), SEQ_SUB(4), SEQ_SUB(1), SEQ_SUB(2), SEQ_END};
// uint16_t seqWonkeyRoll[] = {SEQ_SUB(2), SEQ_SUB(6), SEQ_SUB(1), SEQ_END};

// uint16_t *list_sequences[] = {seqCenter, seqRoll, seqRL, seqFunny, seqCrss, seqUncross, seqCross, seqColor, seqDemo, seqWonkey, seqSuffle, seqSuffleRoll, seqWonkeyRoll};
#define CENTER 0
#define RL 1
#define ROLL 2
#define RHYTHM 3
#define DOWN 4
#define CROSS 5
#define WONKY 6

#define S_RL 11
#define S_ROLL 12
#define S_RHYTHM 13
#define S_DOWN 14
#define S_CROSS 15
#define S_WONKY 16
#define S_BIGSHOW 17

uint16_t qCenter[] = {SEQ_CLR(1), SEQ_MOV(5555), SEQ_END};
uint16_t qRL[] = {SEQ_MOV(5151), SEQ_MOV(5555), SEQ_MOV(5959), SEQ_MOV(5555), SEQ_END};
uint16_t qRoll[] = {SEQ_MOV(5151), SEQ_DLY(5), SEQ_ARC(9595), SEQ_ARC(5959), SEQ_DLY(5), SEQ_MOV(5555), SEQ_END};
uint16_t qRhythm[] = {SEQ_MOV(5151), SEQ_MOV(9595), SEQ_MOV(5959), SEQ_MOV(9595), SEQ_END};
uint16_t qSDown[] = {SEQ_MOV(1515), SEQ_ARC(5252), SEQ_ARC(1515), SEQ_ARC(5858), SEQ_ARC(1515), SEQ_END};
uint16_t qCross[] = {SEQ_MOV(5159), SEQ_DLY(5), SEQ_MOV(5555), SEQ_MOV(5951), SEQ_DLY(5), SEQ_MOV(5555), SEQ_END};
uint16_t qWonky[] = {SEQ_ARC(5559), SEQ_ARC(5595), SEQ_ARC(5551), SEQ_ARC(5515), SEQ_ARC(5559), SEQ_MOV(5555), SEQ_END};
uint16_t qRight[] = {SEQ_MOV(1111), SEQ_END};
uint16_t qDown[] = {SEQ_MOV(1515), SEQ_END};
uint16_t qLeft[] = {SEQ_MOV(1919), SEQ_END};

uint16_t s0[] = {SEQ_SPD(75), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s1[] = {SEQ_SPD(50), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s2[] = {SEQ_SPD(30), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s3[] = {SEQ_SPD(25), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s4[] = {SEQ_SPD(20), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s5[] = {SEQ_SPD(15), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s6[] = {SEQ_SPD(10), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s7[] = {SEQ_SPD(8), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s8[] = {SEQ_SPD(5), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t s9[] = {SEQ_SPD(20), SEQ_CLR(0), SEQ_SUB(RHYTHM), SEQ_CLR(1), SEQ_SUB(RHYTHM), SEQ_LOOP};

uint16_t home[] = {5555, SEQ_END};
uint16_t lRL[] = {SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_END};
uint16_t lRoll[] = {SEQ_NXTCLR, SEQ_SUB(ROLL), SEQ_SUB(ROLL), SEQ_SUB(ROLL), SEQ_END};
uint16_t lRhythm[] = {SEQ_NXTCLR, SEQ_SUB(RHYTHM), SEQ_NXTCLR, SEQ_SUB(RHYTHM), SEQ_NXTCLR, SEQ_SUB(RHYTHM), SEQ_LOOP};
uint16_t lDown[] = {SEQ_NXTCLR, SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(DOWN), SEQ_SUB(CENTER), SEQ_END};
uint16_t lCross[] = {SEQ_NXTCLR, SEQ_SUB(CROSS), SEQ_SUB(CROSS), SEQ_SUB(CROSS), SEQ_END};
uint16_t lWonky[] = {SEQ_NXTCLR, SEQ_SUB(CROSS), SEQ_SUB(CROSS), SEQ_SUB(WONKY), SEQ_END};
uint16_t lShow1[] = {SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(ROLL),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(ROLL),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(CENTER), SEQ_DLY(10), SEQ_SUB(CROSS), SEQ_DLY(5),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(ROLL),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(ROLL),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(RL), SEQ_SUB(CENTER), SEQ_DLY(10), SEQ_SUB(WONKY), SEQ_DLY(5),
                     SEQ_LOOP};
uint16_t lShow2[] = {SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(DOWN), SEQ_SUB(CENTER), SEQ_SUB(WONKY), SEQ_SUB(ROLL),
                     SEQ_NXTCLR, SEQ_SUB(RL), SEQ_SUB(DOWN), SEQ_SUB(CENTER), SEQ_SUB(WONKY), SEQ_SUB(ROLL),
                     SEQ_LOOP};
uint16_t lKids[] = {SEQ_MOV(1515), SEQ_DLY(5), SEQ_MOV(1111), SEQ_DLY(5),
                    SEQ_MOV(1515), SEQ_DLY(5), SEQ_MOV(1919), SEQ_DLY(5),
                    SEQ_MOV(1515), SEQ_DLY(5), SEQ_END};
uint16_t lStagger[] = {SEQ_SPD(20),// SEQ_NXTCLR,
 SEQ_MOV(5151), SEQ_DLY(4), SEQ_DLYS(7), SEQ_MOV(5959),
                       SEQ_DLYR(14), SEQ_MOV(5151), SEQ_DLYS(7), SEQ_MOV(5555), SEQ_END};
uint16_t lDark[] = {SEQ_SPD(20), SEQ_NXTCLR, SEQ_MOV(5151), SEQ_DLY(4), SEQ_DLYS(7), SEQ_MOV(5959),
                       SEQ_DLYR(14), SEQ_MOV(5151), SEQ_DLYS(7), SEQ_MOV(5555), SEQ_END};

uint16_t *list_sequences[] = {qCenter, qRL, qRoll, qRhythm, qSDown, qCross, qWonky, qRight, qDown, qLeft,
                              s0, s1, s2, s3, s4, s5, s6, s7, s8, s9,
                              home, lRL, lRoll, lRhythm, lDown, lCross, lWonky, lShow1, lShow2, lKids,
                              lStagger};

/*  Sequence list
20 - home
21 - Right to left
22 - Roll
23 - Rhythm
24 - Roll down
25 - Cross
26 - Wonky
27 - Show1
28 - Show2


*/
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
int16_t default_steps;

int16_t xA_last_pos;
int16_t yA_last_pos;
int16_t xB_last_pos;
int16_t yB_last_pos;

struct coord
{
    int16_t xA0;
    int16_t yA0;
    int16_t xA1;
    int16_t yA1;
    int16_t xB0;
    int16_t yB0;
    int16_t xB1;
    int16_t yB1;
    int16_t step;
    int16_t num_steps;
} pos;

struct polar_coord
{
    float rA0;
    float aA0;
    float rA1;
    float aA1;

    float rB0;
    float aB0;
    float rB1;
    float aB1;

    int16_t step;
    int16_t num_steps;
} arcpos;

static bool update_linear_move();
static bool update_arc_move();

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
    //    ZPR("pos %d, %d\n", p1, p2);

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
    if (update_linear_move())
        return;

    if (update_arc_move())
        return;

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
            ZPR("end\n");
            --stack;
            if (stack < 0)
            {
                stack = -1;
                return;
            }
            id = seq[stack].id;
            step = seq[stack].step;
            pattern = list_sequences[id];
        }
        else if (pattern[step] == SEQ_LOOP)
        {
            ZPR("loop\n");
            step = 0xFFFF;
            //                ZPR("loop\n");
        }
        else if (pattern[step] == SEQ_NXTCLR)
        {
            ZPR("next color\n");
            next_color_pattern();
        }
        else if (pattern[step] >= SEQ_SUB(0))
        {
            ZPR("sub\n");
            //            __ASSERT(pattern[step] & 0x00ff < ARRAY_SIZE(list_sequences), "SEQ_SUB error");
            //           __ASSERT(stack < CALL_DEPTH, "CALL_DEPTH error");
            ++stack;
            id = pattern[step] & 0x00ff;
            seq[stack].id = id;
            step = 0xFFFF;
            seq[stack].step = step;
            pattern = list_sequences[id];
        }
        else if (pattern[step] >= SEQ_DLY(0))
        {
            ZPR("delay\n");
            id = pattern[step] & 0x00ff;
            k_msleep(id * 100);
        }
        else if (pattern[step] >= SEQ_DLYS(0))
        {
            ZPR("delay stagger\n");
            id = pattern[step] & 0x00ff;
            k_msleep(id * 100 * WHICH_HEAD+100);
        }
        else if (pattern[step] >= SEQ_DLYR(0))
        {
            ZPR("delay rev stagger\n");
            id = pattern[step] & 0x00ff;
            k_msleep(id * 100 * REV_WHICH_HEAD);
        }
        else if (pattern[step] >= SEQ_SPD(0))
        {
            ZPR("speed\n");
            id = pattern[step] & 0x00ff;
            default_steps = id;
        }
        else if (pattern[step] >= SEQ_CLR(0))
        {
            ZPR("color\n");
            id = pattern[step] & 0x00ff;
            set_color_pattern(id);
        }
        seq[stack].step = ++step;
        ZPR("# %d %d\n", id, step);
    }

    if (pattern[step] < 20000 && pattern[step] >= 10000)
    {
        move_linear(((pattern[step] % 10) - 1) * 12,
                    ((pattern[step] / 10 % 10) - 1) * 12,
                    ((pattern[step] / 100 % 10) - 1) * 12,
                    ((pattern[step] / 1000 % 10) - 1) * 12, 0);
        seq[stack].step = ++step;
        ZPR("move %d %d\n", id, step);
        return;
    }

    if (pattern[step] < 30000 && pattern[step] >= 20000)
    {
        move_arc(((pattern[step] % 10) - 1) * 12,
                 ((pattern[step] / 10 % 10) - 1) * 12,
                 ((pattern[step] / 100 % 10) - 1) * 12,
                 ((pattern[step] / 1000 % 10) - 1) * 12, 0);
        seq[stack].step = ++step;
        ZPR("arc %d %d\n", id, step);
        return;
    }

    p1 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] % 10);
    p2 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 10 % 10);
    p3 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 100 % 10);
    p4 = min_pulse + (max_pulse - min_pulse) / 9 * (pattern[step] / 1000 % 10);

    seq[stack].step = ++step;
    ZPR("pwm %d %d\n", id, step);

    pwm_set_pulse_dt(&servo0, p1);
    pwm_set_pulse_dt(&servo1, p2);
    pwm_set_pulse_dt(&servo2, p3);
    pwm_set_pulse_dt(&servo3, p4);
}

//********************* Arc Move *******************************
static float finterpolate(float a, float b)
{
    return (a + (b - a) * arcpos.step / arcpos.num_steps);
}

static float polar_radius(int16_t xi, int16_t yi)
{
    float x = (xi - 50) * 2;
    float y = (yi - 50) * 2;
    return sqrt(x * x + y * y);
}

#define PI 3.14159
static float polar_angle(int16_t xi, int16_t yi)
{
    float x = (xi - 50) * 2;
    float y = (yi - 50) * 2;
    float angle;

    if (x != 0.0)
    {
        angle = atan(y / x);
    }
    else
    {
        angle = (y > 0) ? PI / 2.0 : -PI / 2.0;
    }
    if (x < 0)
    {
        angle += PI;
    }
    return angle;
}

void move_arc(int16_t xAi, int16_t yAi, int16_t xBi, int16_t yBi, int16_t steps)
{
    arcpos.rA0 = polar_radius(xA_last_pos, yA_last_pos);
    arcpos.aA0 = polar_angle(xA_last_pos, yA_last_pos);

    arcpos.rB0 = polar_radius(xB_last_pos, yB_last_pos);
    arcpos.aB0 = polar_angle(xB_last_pos, yB_last_pos);

    xA_last_pos = xAi;
    yA_last_pos = yAi;

    xB_last_pos = xBi;
    yB_last_pos = yBi;

    //    ZPR("movearc %d %d\n", xAi, yAi);

    arcpos.rA1 = polar_radius(xAi, yAi);
    arcpos.aA1 = polar_angle(xAi, yAi);

    //  no rotation > 180 degrees
    if (fabs(arcpos.aA1 - arcpos.aA0) > PI)
        arcpos.aA1 += (arcpos.aA1 < arcpos.aA0) ? PI * 2 : -PI * 2;

    arcpos.rB1 = polar_radius(xBi, yBi);
    arcpos.aB1 = polar_angle(xBi, yBi);

    //  no rotation > 180 degrees
    if (fabs(arcpos.aB1 - arcpos.aB0) > PI)
        arcpos.aB1 += (arcpos.aB1 < arcpos.aB0) ? PI * 2 : -PI * 2;

    //   ZPR("      %d %d\n", (int)arcpos.rB1, (int)(arcpos.aB1 * 180. / 3.14159));

    arcpos.num_steps = (steps == 0) ? default_steps : steps;
    arcpos.step = 0;
}

static bool update_arc_move()
{
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint32_t p4;
    int x;
    int y;

    if (arcpos.step >= arcpos.num_steps)
        return false;

    ++arcpos.step;
    float r = finterpolate(arcpos.rA0, arcpos.rA1);
    float a = finterpolate(arcpos.aA0, arcpos.aA1);

    x = (int)(r * cos(a) / 2.0 + 50);
    y = (int)(r * sin(a) / 2.0 + 50);

    //   ZPR("arc %d %d\n", x, y);
    p1 = min_pulse + (max_pulse - min_pulse) * x / 100;
    p2 = min_pulse + (max_pulse - min_pulse) * y / 100;

    r = finterpolate(arcpos.rB0, arcpos.rB1);
    a = finterpolate(arcpos.aB0, arcpos.aB1);

    x = (int)(r * cos(a) / 2.0 + 50);
    y = (int)(r * sin(a) / 2.0 + 50);

    p3 = min_pulse + (max_pulse - min_pulse) * x / 100;
    p4 = min_pulse + (max_pulse - min_pulse) * y / 100;

    pwm_set_pulse_dt(&servo0, p1);
    pwm_set_pulse_dt(&servo1, p2);
    pwm_set_pulse_dt(&servo2, p3);
    pwm_set_pulse_dt(&servo3, p4);
    return true;
}

//********************* Linear Move *******************************

static int16_t interpolate(int16_t a, int16_t b)
{
    return (a + (b - a) * pos.step / pos.num_steps);
}

void move_linear(int16_t xA, int16_t yA, int16_t xB, int16_t yB, int16_t steps)
{
    //   ZPR("move %d %d %d %d\n", xA, yA, xB, yB);

    pos.xA0 = xA_last_pos;
    pos.yA0 = yA_last_pos;
    pos.xA1 = xA;
    pos.yA1 = yA;
    xA_last_pos = xA;
    yA_last_pos = yA;

    pos.xB0 = xB_last_pos;
    pos.yB0 = yB_last_pos;
    xB_last_pos = xB;
    yB_last_pos = yB;
    pos.xB1 = xB;
    pos.yB1 = yB;

    pos.num_steps = (steps == 0) ? default_steps : steps;
    pos.step = 0;
}

static bool update_linear_move()
{
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint32_t p4;

    if (pos.step >= pos.num_steps)
        return false;

    ++pos.step;
    int16_t xA = interpolate(pos.xA0, pos.xA1);
    int16_t yA = interpolate(pos.yA0, pos.yA1);

    int16_t xB = interpolate(pos.xB0, pos.xB1);
    int16_t yB = interpolate(pos.yB0, pos.yB1);

    p1 = min_pulse + (max_pulse - min_pulse) * xA / 100;
    p2 = min_pulse + (max_pulse - min_pulse) * yA / 100;
    p3 = min_pulse + (max_pulse - min_pulse) * xB / 100;
    p4 = min_pulse + (max_pulse - min_pulse) * yB / 100;

    pwm_set_pulse_dt(&servo0, p1);
    pwm_set_pulse_dt(&servo1, p2);
    pwm_set_pulse_dt(&servo2, p3);
    pwm_set_pulse_dt(&servo3, p4);
    return true;
}
//*****************************************************************
void servo_init()
{
    stack = -1;
    servo_timeout = 30;

    arcpos.rA1 = 0.0;
    arcpos.aA1 = 0.0;
    arcpos.rB1 = 0.0;
    arcpos.aB1 = 0.0;

    xA_last_pos = 50;
    yA_last_pos = 50;
    pos.xA1 = 50;
    pos.yA1 = 50;
    pos.xB1 = 50;
    pos.yB1 = 50;
    default_steps = 10;
    pos.num_steps = default_steps;
    pos.step = default_steps;

    if (!device_is_ready(servo0.dev))
    {
        ZPR("Error: PWM device %s is not ready\n", servo0.dev->name);
        return;
    }

    if (!device_is_ready(servo1.dev))
    {
        ZPR("Error: PWM device %s is not ready\n", servo1.dev->name);
        return;
    }

    if (!device_is_ready(servo2.dev))
    {
        ZPR("Error: PWM device %s is not ready\n", servo2.dev->name);
        return;
    }

    if (!device_is_ready(servo3.dev))
    {
        ZPR("Error: PWM device %s is not ready\n", servo3.dev->name);
        return;
    }
}
