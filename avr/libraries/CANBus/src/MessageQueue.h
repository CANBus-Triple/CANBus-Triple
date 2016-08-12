//
// Created by Emanuele on 09/08/2016.
//

#ifndef CANBUS_TRIPLE_CANFRAMEQUEUE_H
#define CANBUS_TRIPLE_CANFRAMEQUEUE_H

struct Message {
    byte length;
    unsigned short frame_id;
    byte frame_data[8];
    unsigned int busStatus;
    unsigned int busId;
    bool dispatch;
};

class MessageQueue {
public:
    MessageQueue(const unsigned int capacity, Message * buffer)
            : contents(buffer), size(capacity), head(-1), tail(-1) {};

    bool isEmpty();
    Message pop();
    Message * write(const unsigned int busId, const unsigned int busStatus = 0);
    bool push(const Message msg);

private:
    Message* contents;
    unsigned int size;
    int head;
    int tail;
};

bool MessageQueue::isEmpty()
{
    return (head == -1);
}

Message MessageQueue::pop()
{
    if (head == -1) {
        Message m;
        m.frame_id = 0;
        m.dispatch = false;
        return m;
    }
    int lastHead = head;
    if (head == tail) tail = head = -1;
    else head = (head + 1) % size;
    return contents[lastHead];
}

Message * MessageQueue::write(const unsigned int busId, const unsigned int busStatus)
{
    if (tail > -1) {
        int newTail = (tail + 1) % size;
        if (newTail == head) return NULL;
        tail = newTail;
    } else {
        tail = head = 0;
    }
    Message * m = &contents[tail];
    m->length = m->frame_id = 0;
    memset(m->frame_data, 0, 8);
    m->busStatus = busStatus;
    m->busId = busId;
    m->dispatch = false;
    return m;
}

bool MessageQueue::push(const Message msg)
{
    Message * m = this->write(msg.busStatus, msg.busId);
    if (m == NULL) return false;
    m->length = msg.length;
    m->frame_id = msg.frame_id;
    m->dispatch = msg.dispatch;
    for(int d = 0; d < msg.length; d++) m->frame_data[d] = msg.frame_data[d];
    return true;
}

#endif //CANBUS_TRIPLE_CANFRAMEQUEUE_H
