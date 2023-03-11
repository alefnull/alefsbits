#pragma once

#include <rack.hpp>

// ResizbleRingBuffer is a resizable ring buffer that can be used to store
// floats Each time a value is added to the buffer, the head is moved forward If
// the head reaches the end of the buffer, it wraps around to the beginning
// Resizing the buffer will always attempt to keep the latest data

template <typename T>
struct ResizableRingBuffer {
    std::vector<T> buffer;
    int head = 0;
    int size = 0;

    void resize(int newSize);
    void add(T value);
    T get(int index);
};

template <typename T>
void ResizableRingBuffer<T>::resize(int newSize) {
    if (newSize == size) {
        return;
    }

    if (newSize < size) {
        // if the new size is smaller than the current size, we need to
        // copy size - newSize elements from the end of the buffer to the
        // beginning of the buffer
        int offset = size - newSize;
        for (int i = 0; i < newSize; i++) {
            buffer[i] = buffer[i + offset];
        }
        head = newSize;
    } else {
        // if the new size is larger than the current size, we need to
        // copy size elements from the beginning of the buffer to the
        // end of the buffer
        buffer.resize(newSize);
        for (int i = size; i < newSize; i++) {
            buffer[i] = buffer[i - size];
        }
        head = size;
    }

    size = newSize;
}

template <typename T>
void ResizableRingBuffer<T>::add(T value) {
    buffer[head] = value;
    head = (head + 1) % size;
}

template <typename T>
T ResizableRingBuffer<T>::get(int index) {
    return buffer[(head + index) % size];
}