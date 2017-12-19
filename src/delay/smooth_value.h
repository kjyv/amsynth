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
#ifndef SMOOTH_VALUE_H
#define SMOOTH_VALUE_H

class SmoothValue {
  public:
    SmoothValue(float value = 0.0);

    void tick(int i);
    void process();
    void setSampleRate(int sample_rate);
    void setBufferSize(int buffer_size);

    void set(float value) {
        target_value_ = value;
    }

    void setHard(float value) {
        current_value_ = value;
        target_value_ = value;
    }

    float value() const{ return current_value_; }

  private:
    void computeDecay();
    float current_value_;
    float target_value_;
    float decay_;
    int num_samples_;
    int sample_rate_;
    int buffer_size_;
};

#endif // SMOOTH_VALUE_H
