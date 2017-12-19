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

#include "delay.h"

#define DEFAULT_PERIOD 100.0

Delay::Delay(int size) {
    memory_ = new Memory(size);
    smooth_frequency_value = new SmoothValue(0.0);
    current_feedback_ = new_feedback = 0.0;
    current_wet_ = 0.0;
    current_dry_ = 0.0;
    current_period_ = DEFAULT_PERIOD;
    setSampleRate(44100);
}

Delay::~Delay() {
  delete memory_;
  delete smooth_frequency_value;
}

void Delay::setSampleRate(float value){
    sampleRate = value;
    smooth_frequency_value->setSampleRate(sampleRate);
}

void Delay::setParamWet(float value){
    current_wet_ = value;
    current_dry_ = 1 - value;
}

void Delay::setParamFrequency(float value){
    //new_sample_delay = sampleRate * value;
    smooth_frequency_value->set(sampleRate * value);
}

void Delay::setParamFeedback(float value){
    new_feedback = value;
}

void Delay::process(float *audio, float* dest, unsigned buffer_size_, int stride) {
    //float wet = utils::clamp(input(kWet)->at(0), 0.0, 1.0);
    //float new_wet = sqrt(wet);
    //float new_dry = sqrt(1.0 - wet);
    //float wet_inc = (new_wet - current_wet_) / buffer_size_;
    //float dry_inc = (new_dry - current_dry_) / buffer_size_;
    //float new_period = utils::clamp(new_sample_delay, 2.0f, memory_->getSize() - 1.0f);
    //float period_inc = ((new_period - current_period_) / buffer_size_) * stride;
    float feedback_inc = ((new_feedback - current_feedback_) / buffer_size_) * stride;

    smooth_frequency_value->setBufferSize(buffer_size_ / stride);

    // parameter smoothing
    for (uint i = 0; i < buffer_size_; i+=stride) {
        current_feedback_ += feedback_inc;
        /*current_wet_ += wet_inc;
        current_dry_ += dry_inc;
        current_period_ += period_inc;
        */
        smooth_frequency_value->tick(i);
        current_period_ = smooth_frequency_value->value();
        tick(i, audio, dest);
  }
}

//process one sample
void Delay::tick(int i, float* audio, float* dest) {
    float read = memory_->get(current_period_);
    memory_->push(audio[i] + read * current_feedback_);
    dest[i] = current_dry_ * audio[i] + current_wet_ * read;
    MOPO_ASSERT(std::isfinite(dest[i]));
}
