//
// Created by Emanuele on 09/08/2016.
//

#ifndef _CBT_MessageQueue_H
#define _CBT_MessageQueue_H


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
    MessageQueue(const unsigned int bufSize, Message * buffer)
	   : contents(buffer), size(bufSize), items(0), head(0), tail(0) {}
    bool isEmpty() const;
    Message pop();
    bool push(const Message msg);
    bool isFull() const;

private:
    Message * contents;
    const unsigned int size;
    int items;
    int head;
    int tail;
};


bool MessageQueue::isEmpty() const {
    return items == 0;
}

bool MessageQueue::isFull() const {
    return items == size;
}

bool MessageQueue::push(const Message msg) {
    if (isFull()) return false;

    contents[tail++] = msg;
    if (tail == size) tail = 0; // wrap-around index.
    items++;

    return true;
}

Message MessageQueue::pop() {
    if (isEmpty()) {
        Message m;
        m.frame_id = 0;
        m.dispatch = false;
        return m;
    }
    Message m = contents[head++];
    items--;
    if (head == size) head = 0; // wrap-around index.
    return m;
}


#endif // _CBT_MessageQueue_H
