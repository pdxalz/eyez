#ifndef SERVO_H__
#define SERVO_H__

enum WhichEye {Eye_left, Eye_right, Eye_both};

void servo_init();
void servo_update();
void start_sequence(int sequence);
void eye_position(enum WhichEye eye, uint8_t pos);


#endif // SERVO_H__