#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <vector>
#include <iostream>
#include <cstring>
#include <atomic>

#include "ScopedLocker.h"

template<class T>
class Ringbuffer
{
private:
    std::vector<T> data_;
    typename std::vector<T>::iterator write_pos, read_pos;
    std::atomic<bool> newDataAvailable_;
    SemaphoreHandle_t BinarySemaphore;
    SemaphoreHandle_t Mutex;
public:
    Ringbuffer(int size)
    : data_(size), write_pos(data_.begin()), read_pos(data_.begin()), newDataAvailable_(false)
    , BinarySemaphore(xSemaphoreCreateBinary()), Mutex((xSemaphoreCreateMutex()))
    {}

    ~Ringbuffer() {
      vSemaphoreDelete(BinarySemaphore);
      vSemaphoreDelete(Mutex);
    }

    void TakeBinarySemaphore() {
      xSemaphoreTake(BinarySemaphore, portMAX_DELAY);
    }

    void GiveBinarySemaphore() {
      xSemaphoreGive(BinarySemaphore);
    }

    int size() {
        return data_.size();
    }

    void add(T value) {
        *write_pos = value;
        write_pos++;

        if(write_pos == data_.end()){
            write_pos = data_.begin();
        }
    }

    T &get(int pos) {
        return data_[pos % size()];
    }

    int write(const T *buffer, uint32_t Writelen){

        if(Writelen > 0){
            for (size_t i = 0; i < Writelen; i++)
            {
                add(buffer[i]);
            }
            newDataAvailable_ = true;
            return Writelen;
        }
        return 0;
    }

    /* It is the users responsibility to make sure that all available bytes are read before a new write is performed */
    int read(T *buffer, int Readlen){
      //ScopedLocker Idiom(&Mutex);
      if(newDataAvailable_){
          //Calculate new available bytes from read to write iterator
          int bytesAvailableDist_ = std::distance(read_pos, write_pos);
          if(bytesAvailableDist_ > 0) { // If write_pos grater than read_pos
              if(Readlen <= bytesAvailableDist_) {
                  std::copy(read_pos, read_pos + Readlen, buffer);
                  read_pos = read_pos + Readlen;
                  if(read_pos == write_pos){
                      newDataAvailable_ = false;
                  }
                  return Readlen;
              } else {
                  std::copy(read_pos, read_pos + bytesAvailableDist_, buffer);
                  read_pos = read_pos + bytesAvailableDist_;
                  newDataAvailable_ = false;
                  return bytesAvailableDist_;
              }
          } else if(bytesAvailableDist_ < 0) { //if write_pos less than read_pos
              int distanceRE_ = std::distance(read_pos, data_.end());
              if(Readlen <= distanceRE_) {
                  std::copy(read_pos, read_pos + Readlen, buffer);
                  read_pos = read_pos + Readlen == data_.end() ? data_.begin() : read_pos + Readlen;
                  if(read_pos == write_pos){
                      newDataAvailable_ = false;
                  }
                  return Readlen;
              } else {
                  std::copy(read_pos, data_.end(), buffer);

                  int distanceBW_ =  std::distance(data_.begin(), write_pos);

                  if(distanceBW_ <= Readlen-distanceRE_) {
                      std::copy(data_.begin(), data_.begin()+distanceBW_, buffer + distanceRE_);
                      read_pos = write_pos;
                      newDataAvailable_ = false;
                      return distanceRE_ + distanceBW_;
                  } else {
                      std::copy(data_.begin(), data_.begin()+Readlen-distanceRE_, buffer + distanceRE_);
                      read_pos = data_.begin()+Readlen-distanceRE_;
                      return distanceRE_+Readlen-distanceRE_;
                  }
              }
          } else { //Else there must be size() bytes available
              if(Readlen >= size()) {
                  if(read_pos == data_.begin()){
                  std::copy(data_.begin(), data_.end(), buffer);
                  newDataAvailable_ = false;
                  return size();
                  } else {
                      std::copy(read_pos, data_.end(), buffer);
                      std::copy(data_.begin(), read_pos, buffer+distance(read_pos, data_.end()));
                      newDataAvailable_ = false;
                      return size();
                  }
              } else if (read_pos + Readlen > data_.end()) {
                  std::copy(read_pos, data_.end(), buffer);
                  int rest = Readlen - distance(read_pos, data_.end());
                  std::copy(data_.begin(), data_.begin() + rest, buffer + rest);
                  read_pos = data_.begin() + rest;
                  return Readlen;

              } else {
                  std::copy(read_pos, read_pos + Readlen, buffer);
                  read_pos = read_pos + Readlen == data_.end() ? data_.begin() : read_pos + Readlen;
                  return Readlen;
              }
          }
      } else {
          //If no new data available
          return 0;
      }
    }
};

#endif
