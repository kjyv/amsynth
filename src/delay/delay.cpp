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
  current_feedback_ = 0.0;
  current_wet_ = 0.0;
  current_dry_ = 0.0;
  current_period_ = DEFAULT_PERIOD;
  sampleRate = 44100;
  paramSampleDelay = sampleRate * 0.0;
}

Delay::~Delay() {
  delete memory_;
}

void Delay::setParamWet(float value){
    current_wet_ = value;
    current_dry_ = 1 - value;
}

void Delay::setParamFrequency(float value){
    paramSampleDelay = sampleRate * value;
}

void Delay::setParamFeedback(float value){
    current_feedback_ = value;
}

void Delay::process(float *audio, float* dest, unsigned buffer_size_, int stride) {
	//MOPO_ASSERT(inputMatchesBufferSize(kAudio));

	//const float* audio = input(kAudio)->source->buffer;
	//float* dest = output()->buffer;

	//float wet = utils::clamp(input(kWet)->at(0), 0.0, 1.0);
	//float new_wet = sqrt(wet);
	//float new_dry = sqrt(1.0 - wet);
	//float wet_inc = (new_wet - current_wet_) / buffer_size_;
	//float dry_inc = (new_dry - current_dry_) / buffer_size_;

	//float new_feedback = input(kFeedback)->at(0);
	//float feedback_inc = (new_feedback - current_feedback_) / buffer_size_;

	//float new_period = utils::clamp(input(kSampleDelay)->at(0), 2.0, memory_->getSize() - 1.0);
	//float period_inc = (new_period - current_period_) / buffer_size_;

	for (int i = 0; i < buffer_size_; i+=stride) {
		/*current_feedback_ += feedback_inc;
		current_wet_ += wet_inc;
		current_dry_ += dry_inc;
		current_period_ += period_inc;
		*/
		tick(i, audio, dest);
  }
}

//process one sample
void Delay::tick(int i, float* audio, float* dest) {
  float new_period = utils::clamp(paramSampleDelay, 2.0f, memory_->getSize() - 1.0f);
  float period_inc = (new_period - current_period_);
  current_period_ += period_inc;
  float read = memory_->get(current_period_);
  memory_->push(audio[i] + read * current_feedback_);
  dest[i] = current_dry_ * audio[i] + current_wet_ * read;
  MOPO_ASSERT(std::isfinite(dest[i]));
}
