/* Copyright 2013-2017 Matt Tytel
 *
 * mopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mopo.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "utils.h"

  // A processor utility to store a stream of data for later lookup.
  class Memory {
    public:
      Memory(int size);
      Memory(const Memory& other);
      ~Memory();

      void push(float sample) {
        offset_ = (offset_ + 1) & bitmask_;
        memory_[offset_] = sample;
      }

      void pushBlock(const float* samples, unsigned int num) {
        unsigned int next_offset = (offset_ + num) & bitmask_;
        if (next_offset < offset_) {
          unsigned int block1 = num - next_offset - 1;
          memcpy(memory_ + offset_ + 1, samples, sizeof(float) * block1);
          memcpy(memory_, samples + block1, sizeof(float) * next_offset);
        }
        else
          memcpy(memory_ + offset_ + 1, samples, sizeof(float) * num);

        offset_ = next_offset;
      }

      void pushZero(unsigned int num) {
        unsigned int next_offset = (offset_ + num) & bitmask_;
        if (next_offset < offset_) {
          unsigned int block1 = num - next_offset - 1;
          memset(memory_ + offset_ + 1, 0, sizeof(float) * block1);
          memset(memory_, 0, sizeof(float) * next_offset);
        }
        else
          memset(memory_ + offset_ + 1, 0, sizeof(float) * num);

        offset_ = next_offset;
      }

      inline float getIndex(unsigned int index) const {
        unsigned int spot = (offset_ - index) & bitmask_;
        MOPO_ASSERT(spot < size_);
        return memory_[spot];
      }

      inline float get(float past) const {
        MOPO_ASSERT(past >= 0.0);
        int index = utils::imax(past, 1);
        float sample_fraction = past - index;

        float from = getIndex(index - 1);
        float to = getIndex(index);
        return utils::interpolate(from, to, sample_fraction);
      }

      unsigned int getOffset() const { return offset_; }

      void setOffset(int offset) { offset_ = offset; }

      const float* getPointer(int past) const {
        return memory_ + ((offset_ - past) & bitmask_);
      }

      const float* getBuffer() const {
        return memory_;
      }

      int getSize() const {
        return size_;
      }

    protected:
      float* memory_;
      unsigned int size_;
      unsigned int bitmask_;
      unsigned int offset_;
  };

#endif // MEMORY_H
