#ifndef SERVO_H__
#define SERVO_H__

#define BIG_EYE 0
#define HEAD_MOM 1
#define HEAD_BABY 2
#define HEAD_JUNIOR 3
#define HEAD_DAD 4
#define LAST_HEAD 5

#define WHICH_HEAD BIG_EYE
#define REV_WHICH_HEAD (LAST_HEAD - WHICH_HEAD)
enum WhichEye
{
    Eye_left,
    Eye_right,
    Eye_both
};

void servo_init();
void servo_update();
void start_sequence(int sequence);
void eye_position(enum WhichEye eye, uint8_t pos);

void move_linear(int16_t xA, int16_t yA, int16_t xB, int16_t yB, int16_t steps);
void move_arc(int16_t xAi, int16_t yAi, int16_t xBi, int16_t yBi, int16_t steps);

#endif // SERVO_H__