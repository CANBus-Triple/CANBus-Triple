
#include "Message.h"

Message::Message(){
    dispatch = false;
    frame_data[0] =
    frame_data[1] =
    frame_data[2] =
    frame_data[3] =
    frame_data[4] =
    frame_data[5] =
    frame_data[6] =
    frame_data[7] = 0;
}

Message::~Message(){
    // delete &length;
    // delete &frame_id;
    // delete frame_data;
}

