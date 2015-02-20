#ifndef CANMessage_H
#define CANMessage_H

#include <Arduino.h>
#include <CANBus.h>


class Message {
    public:
        Message();
        ~Message();

        byte length;
        unsigned short frame_id;
        byte frame_data[8];

        unsigned int busStatus; // Intended to hold status of the bus imediately before reading of the buffer.
        unsigned int busId;
        bool dispatch;

};

#endif