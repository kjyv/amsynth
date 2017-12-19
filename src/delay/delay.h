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
#ifndef DELAY_H
#define DELAY_H

#include "memory.h"
#include "smooth_value.h"

// A signal delay processor with wet/dry, delay time and feedback controls.
// Handles fractional delay amounts through interpolation.
class Delay {
    public:
        Delay(int size);
        virtual ~Delay();

        void setSampleRate(float value);
        void setParamWet(float value);
        void setParamFrequency(float value);
        void setParamFeedback(float value);

        void process(float *audio, float* dest, unsigned buffer_size_, int stride);
        void tick(int i, float* audio, float* dest);

    protected:
        Memory* memory_;
        SmoothValue *smooth_frequency_value;
        float sampleRate;
        float current_feedback_;
        float current_wet_;
        float current_dry_;
        float current_period_;
        float new_feedback;
};

#endif // DELAY_H
