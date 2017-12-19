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

#include "smooth_value.h"
#include <cmath>
#include "utils.h"
#include <stdio.h>

#define SMOOTH_CUTOFF 0.02f

SmoothValue::SmoothValue(float value):
    current_value_(value), target_value_(value), decay_(1.0) { }

void SmoothValue::setSampleRate(int sample_rate) {
    sample_rate_ = sample_rate;
    computeDecay();
}

void SmoothValue::process() {
    if (current_value_ == target_value_) {
        return;
    }
    for (int i = 0; i < buffer_size_; ++i)
        tick(i);
    }

inline void SmoothValue::tick(int i) {
    current_value_ = utils::interpolate(current_value_, target_value_, decay_);
}

void SmoothValue::setBufferSize(int buffer_size) {
  buffer_size_ = buffer_size;
  computeDecay();
}

void SmoothValue::computeDecay() {
  decay_ = 1 - exp(-2.0 * PI * SMOOTH_CUTOFF * buffer_size_ / sample_rate_);
}
