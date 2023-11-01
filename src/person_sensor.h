#ifndef PERSON_SENSOR_H__
#define LED_H__


#define PERSON_ADDR 0x62
#define DEBUG_REG 0x07


// The person sensor will never output more than four faces.
#define PERSON_SENSOR_MAX_FACES_COUNT (4)

// How many different faces the sensor can recognize.
#define PERSON_SENSOR_MAX_IDS_COUNT (7)

// The following structures represent the data format returned from the person
// sensor over the I2C communication protocol. The C standard doesn't
// guarantee the byte-wise layout of a regular struct across different
// platforms, so we add the non-standard (but widely supported) __packed__
// attribute to ensure the layouts are the same as the wire representation.

// The results returned from the sensor have a short header providing
// information about the length of the data packet:
//   reserved: Currently unused bytes.
//   data_size: Length of the entire packet, excluding the header and checksum.
//     For version 1.0 of the sensor, this should be 40.
typedef struct __attribute__((__packed__))
{
    uint8_t reserved[2]; // Bytes 0-1.
    uint16_t data_size;  // Bytes 2-3.
} person_sensor_results_header_t;

// Each face found has a set of information associated with it:
//   box_confidence: How certain we are we have found a face, from 0 to 255.
//   box_left: X coordinate of the left side of the box, from 0 to 255.
//   box_top: Y coordinate of the top edge of the box, from 0 to 255.
//   box_width: Width of the box, where 255 is the full view port size.
//   box_height: Height of the box, where 255 is the full view port size.
//   id_confidence: How sure the sensor is about the recognition result.
//   id: Numerical ID assigned to this face.
//   is_looking_at: Whether the person is facing the camera, 0 or 1.
typedef struct __attribute__((__packed__))
{
    uint8_t box_confidence; // Byte 1.
    uint8_t box_left;       // Byte 2.
    uint8_t box_top;        // Byte 3.
    uint8_t box_right;      // Byte 4.
    uint8_t box_bottom;     // Byte 5.
    int8_t id_confidence;   // Byte 6.
    int8_t id;              // Byte 7
    uint8_t is_facing;      // Byte 8.
} person_sensor_face_t;

// This is the full structure of the packet returned over the wire from the
// sensor when we do an I2C read from the peripheral address.
// The checksum should be the CRC16 of bytes 0 to 38. You shouldn't need to
// verify this in practice, but we found it useful during our own debugging.
typedef struct __attribute__((__packed__))
{
    person_sensor_results_header_t header;                     // Bytes 0-4.
    int8_t num_faces;                                          // Byte 5.
    person_sensor_face_t faces[PERSON_SENSOR_MAX_FACES_COUNT]; // Bytes 6-37.
    uint16_t checksum;                                         // Bytes 38-39.
} person_sensor_results_t;

void person_sensor_init();
void check_person_sensor();
bool get_person_position(uint8_t *x, uint8_t *y);

#endif // PERSON_SENSOR_H__