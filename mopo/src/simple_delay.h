/* Copyright 2013-2016 Matt Tytel
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
#ifndef SIMPLE_DELAY_H
#define SIMPLE_DELAY_H

#include "memory.h"
#include "processor.h"

namespace mopo {

  // A signal delay processor with wet/dry, delay time and feedback controls.
  // Handles fractional delay amounts through interpolation.
  class SimpleDelay : public Processor {
    public:
      enum Inputs {
        kAudio,
        kSampleDelay,
        kFeedback,
        kNumInputs
      };

      SimpleDelay(int size);
      SimpleDelay(const SimpleDelay& other);
      virtual ~SimpleDelay();

      virtual Processor* clone() const override {
        return new SimpleDelay(*this);
      }

      virtual void process() override;

      void tick(int i) {
        mopo_float audio = input(kAudio)->at(i);
        mopo_float period = input(kSampleDelay)->at(i);
        mopo_float feedback = input(kFeedback)->at(i);

        mopo_float read = memory_->get(period);
        mopo_float value = audio + read * feedback;
        memory_->push(value);
        output(0)->buffer[i] = value;
        MOPO_ASSERT(std::isfinite(value));
      }

    protected:
      Memory* memory_;
  };
} // namespace mopo

#endif // SIMPLE_DELAY_H
