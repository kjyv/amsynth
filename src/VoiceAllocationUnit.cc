/*
 *	VoiceAllocationUnit.cc
 *
 *	Copyright (c) 2001-2012 Nick Dowell
 *
 *	This file is part of amsynth.
 *
 *	amsynth is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	amsynth is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with amsynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "VoiceAllocationUnit.h"

#include "Effects/SoftLimiter.h"
#include "Effects/Distortion.h"
#include "freeverb/revmodel.hpp"
#include "delay/delay.h"
#include "VoiceBoard/VoiceBoard.h"

#include <iostream>
#include <math.h>
#include <cstring>
#include <assert.h>


#define MAX_DELAY_SAMPLES 300000

using namespace std;

const unsigned kBufferSize = 1024;

VoiceAllocationUnit::VoiceAllocationUnit ()
:		mMaxVoices (0)
,		mPortamentoTime (0.0f)
,		mPortamentoMode(PortamentoModeAlways)
,		sustain (0)
,		_keyboardMode(KeyboardModePoly)
,		mMasterVol (1.0)
,		mPanGainLeft(1)
,		mPanGainRight(1)
,		mPitchBendRangeSemitones(2)
,		mPitchBendValue(1)
,		mLastNoteFrequency (0.0f)
{
		limiter = new SoftLimiter;
		reverb = new revmodel;
		delay_l = new Delay(MAX_DELAY_SAMPLES);
		delay_r = new Delay(MAX_DELAY_SAMPLES);
		distortion = new Distortion;
		mBuffer = new float [kBufferSize * 2];
		mSmoothMasterVol = new SmoothValue(mMasterVol);
		mSmoothMasterVol->setBufferSize(0);

		for (int i = 0; i < 128; i++)
		{
				keyPressed[i] = false;
				active[i] = false;
				_voices.push_back (new VoiceBoard);
		}

		memset(&_keyPresses, 0, sizeof(_keyPresses));

		SetSampleRate (44100);
}

VoiceAllocationUnit::~VoiceAllocationUnit		()
{
		while (_voices.size()) { delete _voices.back(); _voices.pop_back(); }
		delete limiter;
		delete reverb;
		delete delay_l;
		delete delay_r;
		delete distortion;
		delete [] mBuffer;
		delete mSmoothMasterVol;
}

void
VoiceAllocationUnit::SetSampleRate	(int rate)
{
		limiter->SetSampleRate (rate);
		for (unsigned i=0; i<_voices.size(); ++i) _voices[i]->SetSampleRate (rate);
		delay_l->setSampleRate(rate);
		delay_r->setSampleRate(rate);
		mSmoothMasterVol->setSampleRate(rate);
}

void
VoiceAllocationUnit::HandleMidiNoteOn(int note, float velocity)
{
		assert (note >= 0);
		assert (note < 128);

		// Checks if the note is within the note ranges activated in the current keyboard map.
		// The above assertions guarantee the safety of this check.
		if (!tuningMap.inActiveRange(note))
				return;

		float pitch = (float) noteToPitch(note);
		if (pitch < 0) { // unmapped key
				return;
		}

		float portamentoTime = mPortamentoTime;
		if (mPortamentoMode == PortamentoModeLegato) {
				int count = 0;
				for (int i=0; i<128; i++) {
						if (keyPressed[i]) {
								count++;
						}
				}
				if (count == 0) {
						portamentoTime = 0;
				}
		}

		keyPressed[note] = true;

		if (_keyboardMode == KeyboardModePoly) {

				if (mMaxVoices) {
						unsigned count = 0;
						for (int i=0; i<128; i++)
								count = count + (active[i] ? 1 : 0);
						if (count >= (unsigned) mMaxVoices) {
								int idx = -1;
								// strategy 1) find the oldest voice in release phase
								unsigned keyPress = _keyPressCounter + 1;
								for (int i=0; i<128; i++) {
										if (active[i] && !keyPressed[i]) {
												if (keyPress > _keyPresses[i]) {
														keyPress = _keyPresses[i];
														idx = i;
												}
										}
								}
								if (idx < 0) {
										// strategy 2) find the oldest voice
										keyPress = _keyPressCounter + 1;
										for (int i=0; i<128; i++) {
												if (active[i]) {
														if (keyPress > _keyPresses[i]) {
																keyPress = _keyPresses[i];
																idx = i;
														}
												}
										}
								}
								assert(0 <= idx && idx < 128);
								active[idx] = false;
						}
				}

				_keyPresses[note] = (++_keyPressCounter);

				if (mLastNoteFrequency > 0.0f) {
						_voices[note]->setFrequency(mLastNoteFrequency, pitch, portamentoTime);
				} else {
						_voices[note]->setFrequency(pitch, pitch, 0);
				}

				if (_voices[note]->isSilent())
						_voices[note]->reset();

				_voices[note]->setVelocity(velocity);
				_voices[note]->triggerOn();

				active[note] = true;
		}

		if (_keyboardMode == KeyboardModeMono || _keyboardMode == KeyboardModeLegato) {

				int previousNote = -1;
				unsigned keyPress = 0;
				for (int i = 0; i < 128; i++) {
						if (keyPress < _keyPresses[i]) {
								keyPress = _keyPresses[i];
								previousNote = i;
						}
				}

				_keyPresses[note] = (++_keyPressCounter);

				VoiceBoard *voice = _voices[0];

				voice->setVelocity(velocity);
				voice->setFrequency(voice->getFrequency(), pitch, portamentoTime);

				if (_keyboardMode == KeyboardModeMono || previousNote == -1)
						voice->triggerOn();

				active[0] = true;
		}

		mLastNoteFrequency = pitch;
}

void
VoiceAllocationUnit::HandleMidiNoteOff(int note, float /*velocity*/)
{
		// No action is required if the note is outside the active range of notes.
		if (!tuningMap.inActiveRange(note))
				return;

		keyPressed[note] = false;

		if (_keyboardMode == KeyboardModePoly) {
				if (!sustain) {
						_voices[note]->triggerOff();
				}
				_keyPresses[note] = 0;
		}

		if (_keyboardMode == KeyboardModeMono || _keyboardMode == KeyboardModeLegato) {

				int currentNote = -1;
				unsigned keyPress = 0;
				for (int i = 0; i < 128; i++) {
						if (keyPress < _keyPresses[i]) {
								keyPress = _keyPresses[i];
								currentNote = i;
						}
				}

				_keyPresses[note] = 0;

				int nextNote = -1;
				for (unsigned i = 0, keyPress = 0; i < 128; i++) {
						if (keyPress < _keyPresses[i]) {
								keyPress = _keyPresses[i];
								nextNote = i;
						}
				}

				if (!keyPress) {
						_keyPressCounter = 0;
				}

				if (note != currentNote) {
						return;
				}

				VoiceBoard *voice = _voices[0];

				if (0 <= nextNote) {
						voice->setFrequency(voice->getFrequency(), (float) noteToPitch(nextNote), mPortamentoTime);
						if (_keyboardMode == KeyboardModeMono)
								voice->triggerOn();
				} else {
						voice->triggerOff();
				}
		}
}

void
VoiceAllocationUnit::HandleMidiPitchWheel(float value)
{
		mPitchBendValue = pow(2.0f, value * mPitchBendRangeSemitones / 12.0f);
}

void
VoiceAllocationUnit::HandleMidiPitchWheelSensitivity(uchar semitones)
{
		setPitchBendRangeSemitones(semitones);
}

void
VoiceAllocationUnit::HandleMidiAllSoundOff()
{
		resetAllVoices();
		reverb->mute();
}

void
VoiceAllocationUnit::HandleMidiAllNotesOff()
{
		resetAllVoices();
}

void
VoiceAllocationUnit::HandleMidiSustainPedal(uchar value)
{
		sustain = value ? 1 : 0;
		if (sustain) return;
		for(unsigned i=0; i<_voices.size(); i++) {
				if (!keyPressed[i]) _voices[i]->triggerOff();
		}
}

void
VoiceAllocationUnit::resetAllVoices()
{
		for (unsigned i=0; i<_voices.size(); i++) {
				active[i] = false;
				keyPressed[i] = false;
				_keyPresses[i] = 0;
				_voices[i]->reset();
		}
		_keyPressCounter = 0;
		sustain = false;
}

void
VoiceAllocationUnit::Process(float *l, float *r, unsigned nframes, int stride)
{
		assert(nframes <= VoiceBoard::kMaxProcessBufferSize);
		memset(mBuffer, 0, nframes * sizeof (float));

		for (unsigned i=0; i<_voices.size(); i++) {
				if (active[i]) {
						if (_voices[i]->isSilent()) {
								active[i] = false;
						} else {
								_voices[i]->SetPitchBend(mPitchBendValue);
								_voices[i]->ProcessSamplesMix (mBuffer, nframes, 1.0);
						}
				}
		}

		distortion->Process (mBuffer, nframes);

		for (unsigned i=0; i<nframes; i++) {
				l[i * stride] = mBuffer[i] * mPanGainLeft;
				r[i * stride] = mBuffer[i] * mPanGainRight;
		}

		delay_l->process(l, l, nframes, stride);
		delay_r->process(r, r, nframes, stride);

		reverb->processmix (l, r, l, r, nframes, stride);

		if (mSmoothMasterVol->getBufferSize() != int(nframes / stride)) {
				mSmoothMasterVol->setBufferSize(nframes / stride);
				mSmoothMasterVol->computeDecay(0.05f);
		}

		for (unsigned i=0; i<nframes; i++) {
				mSmoothMasterVol->tick(i);
				mMasterVol = mSmoothMasterVol->value();
				l[i * stride] = l[i] * mMasterVol;
				r[i * stride] = r[i] * mMasterVol;
		}
		limiter->Process (l,r, nframes, stride);
}

void
VoiceAllocationUnit::setKeyboardMode(KeyboardMode keyboardMode)
{
		if (_keyboardMode != keyboardMode) {
				_keyboardMode = keyboardMode;
				resetAllVoices();
		}
}

void
VoiceAllocationUnit::UpdateParameter		(Param param, float value)
{
		switch (param) {
		case kAmsynthParameter_MasterVolume:		mSmoothMasterVol->set(value);				break;
		case kAmsynthParameter_ReverbRoomsize:	reverb->setroomsize (value);		break;
		case kAmsynthParameter_ReverbDamp:			reverb->setdamp (value);		break;
		case kAmsynthParameter_ReverbWet:				reverb->setwet (value); reverb->setdry(1.0f-value); break;
		case kAmsynthParameter_ReverbWidth:			reverb->setwidth (value);		break;
		case kAmsynthParameter_AmpDistortion:		distortion->SetCrunch (value);	break;
		case kAmsynthParameter_PortamentoTime:	mPortamentoTime = value; break;
		case kAmsynthParameter_KeyboardMode:		setKeyboardMode((KeyboardMode)(int)value); break;
		case kAmsynthParameter_PortamentoMode:	mPortamentoMode = (int) value; break;
		case kAmsynthParameter_DelayWet:				delay_l->setParamWet (value); delay_r->setParamWet (value); break;
		case kAmsynthParameter_DelayFrequency:	delay_l->setParamFrequency(value); delay_r->setParamFrequency (value); break;
		case kAmsynthParameter_DelayFeedback:		delay_l->setParamFeedback (value); delay_r->setParamFeedback (value); break;

		default: for (unsigned i=0; i<_voices.size(); i++) _voices[i]->UpdateParameter (param, value); break;
		}
}

////////////////////////////////////////////////////////////////////////////////

double
VoiceAllocationUnit::noteToPitch		(int note) const
{
		return tuningMap.noteToPitch(note);
}

int
VoiceAllocationUnit::loadScale			(const string & sclFileName)
{
		return tuningMap.loadScale(sclFileName);
}

int
VoiceAllocationUnit::loadKeyMap			(const string & kbmFileName)
{
		return tuningMap.loadKeyMap(kbmFileName);
}

void
VoiceAllocationUnit::defaultTuning	()
{
		tuningMap.defaultScale();
		tuningMap.defaultKeyMap();
}
