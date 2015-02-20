/*
 *  QueueArray.h
 *
 *  Library implementing a generic, dynamic queue (array version).
 *
 *  ---
 *
 *  Copyright (C) 2010  Efstathios Chatzikyriakidis (contact@efxa.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *   2013-11-07 Marcus Nowotny <interactive-matter.eu>
 *     - pushing to a full array does not crash but returns false
 *
 *   2013-11-05 Marcus Nowotny <interactive-matter.eu>
 *     - rewritten to use a default size
 *     - texts are now in flash
 *
 *  Version 1.0
 *
 *    2010-09-29  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - added resize(): for growing, shrinking the array size.
 *
 *    2010-09-25  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - added exit(), blink(): error reporting and handling methods.
 *
 *    2010-09-24  Alexander Brevig  <alexanderbrevig@gmail.com>
 *
 *      - added setPrinter(): indirectly reference a Serial object.
 *
 *    2010-09-20  Efstathios Chatzikyriakidis  <contact@efxa.org>
 *
 *      - initial release of the library.
 *
 *  ---
 *
 *  For the latest version see: http://www.arduino.cc/
 */

// header defining the interface of the source.
#ifndef _QUEUEARRAY_H
#define _QUEUEARRAY_H

// include Arduino basic header.
#include <Arduino.h>

// the definition of the queue class.
template<typename T>
class QueueArray {
  public:
    // init the queue (constructor).
    QueueArray (const unsigned char initialSize);

    // clear the queue (destructor).
    ~QueueArray ();

    // push an item to the queue.
    bool push (const T i);

    // pop an item from the queue.
    T pop ();

    // get an item from the queue.
    T peek () const;

    // check if the queue is empty.
    bool isEmpty () const;

    // get the number of items in the queue.
    int count () const;

    // check if the queue is full.
    bool isFull () const;

    // set the printer of the queue.
    void setStream (Stream & s);

  private:

    // exit report method in case of error.
    void exit(const __FlashStringHelper*) const;

    // led blinking method in case of error.
    void blink () const;

    // the pin number of the on-board led.
    static const int ledPin = 6;

    Stream * stream; // the printer of the queue.
    T * contents;    // the array of the queue.

    int size;        // the size of the queue.
    int items;       // the number of items of the queue.

    int head;        // the head of the queue.
    int tail;        // the tail of the queue.
};

// init the queue (constructor).
template<typename T>
QueueArray<T>::QueueArray (const unsigned char initialSize) {
  size = 0;       // set the size of queue to zero.
  items = 0;      // set the number of items of queue to zero.

  head = 0;       // set the head of the queue to zero.
  tail = 0;       // set the tail of the queue to zero.

  stream = NULL; // set the printer of queue to point nowhere.

  // allocate enough memory for the array.
  contents = (T *) malloc (sizeof (T) * initialSize);

  // if there is a memory allocation error.
  if (contents == NULL)
    exit (F("QUEUE: insufficient memory to initialize queue."));

  // set the initial size of the queue.
  size = initialSize;
}

// clear the queue (destructor).
template<typename T>
QueueArray<T>::~QueueArray () {
  free (contents); // deallocate the array of the queue.

  contents = NULL; // set queue's array pointer to nowhere.
  stream = NULL;  // set the printer of queue to point nowhere.

  size = 0;        // set the size of queue to zero.
  items = 0;       // set the number of items of queue to zero.

  head = 0;        // set the head of the queue to zero.
  tail = 0;        // set the tail of the queue to zero.
}

// push an item to the queue.
template<typename T>
bool QueueArray<T>::push (const T i) {
  // check if the queue is full.
  if (isFull ())
    // we cannot add anythif - just return false
    return false;

  // store the item to the array.
  contents[tail++] = i;

  // wrap-around index.
  if (tail == size) tail = 0;

  // increase the items.
  items++;

  //ok everything was fin
  return true;
}

// pop an item from the queue.
template<typename T>
T QueueArray<T>::pop () {
  // check if the queue is empty.
  if (isEmpty ())
    exit (F("QUEUE: can't pop item from queue: queue is empty."));

  // fetch the item from the array.
  T item = contents[head++];

  // decrease the items.
  items--;

  // wrap-around index.
  if (head == size) head = 0;

  // return the item from the array.
  return item;
}

// get an item from the queue.
template<typename T>
T QueueArray<T>::peek () const {
  // check if the queue is empty.
  if (isEmpty ())
    exit (F("QUEUE: can't peek item from queue: queue is empty."));

  // get the item from the array.
  return contents[head];
}

// check if the queue is empty.
template<typename T>
bool QueueArray<T>::isEmpty () const {
  return items == 0;
}

// check if the queue is full.
template<typename T>
bool QueueArray<T>::isFull () const {
  return items == size;
}

// get the number of items in the queue.
template<typename T>
int QueueArray<T>::count () const {
  return items;
}

// set the printer of the queue.
template<typename T>
void QueueArray<T>::setStream (Stream & s) {
  stream = &s;
}

// exit report method in case of error.
template<typename T>
void QueueArray<T>::exit (const __FlashStringHelper * m) const {
  // print the message if there is a printer.
  if (stream)
    stream->println (m);

  // loop blinking until hardware reset.
  blink ();
}

// led blinking method in case of error.
template<typename T>
void QueueArray<T>::blink () const {
  // set led pin as output.
  pinMode (ledPin, OUTPUT);

  // continue looping until hardware reset.
  while (true) {
    digitalWrite (ledPin, HIGH); // sets the LED on.
    delay (250);                 // pauses 1/4 of second.
    digitalWrite (ledPin, LOW);  // sets the LED off.
    delay (250);                 // pauses 1/4 of second.
  }

  // solution selected due to lack of exit() and assert().
}

#endif // _QUEUEARRAY_H
