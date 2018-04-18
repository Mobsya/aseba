/*
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2013:
        Stephane Magnenat <stephane at magnenat dot net>
        (http://stephane.magnenat.net)
        and other contributors, see authors.txt for details

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __PLAYGROUND_EPUCK_H
#define __PLAYGROUND_EPUCK_H

#include "../../AsebaGlue.h"
#include <enki/PhysicalEngine.h>
#include <enki/robots/e-puck/EPuck.h>

namespace Enki {
// FIXME: this is ugly and should be attached to Enki::World after ECS refactoring
extern unsigned energyPool;

class EPuckFeeding : public LocalInteraction {
public:
    double energy;

public:
    EPuckFeeding(Robot* owner);

    void objectStep(double dt, World* w, PhysicalObject* po) override;

    void finalize(double dt, World* w) override;
};

class EPuckFeeder : public Robot {
public:
    EPuckFeeding feeding;

public:
    EPuckFeeder();
};

class ScoreModifier : public GlobalInteraction {
public:
    ScoreModifier(Robot* owner) : GlobalInteraction(owner) {}

    void step(double dt, World* w) override;
};

class FeedableEPuck : public EPuck {
public:
    double energy;
    double score;
    int diedAnimation;
    ScoreModifier scoreModifier;

public:
    FeedableEPuck();

    void controlStep(double dt) override;
};

class AsebaFeedableEPuck : public FeedableEPuck, public Aseba::SingleVMNodeGlue {
public:
    struct Variables {
        int16_t id;
        int16_t source;
        int16_t args[32];
        int16_t productId;
        int16_t speedL;    // left motor speed
        int16_t speedR;    // right motor speed
        int16_t colorR;    // body red [0..100] %
        int16_t colorG;    // body green [0..100] %
        int16_t colorB;    // body blue [0..100] %
        int16_t prox[8];   //
        int16_t camR[60];  // camera red (left, middle, right) [0..100] %
        int16_t camG[60];  // camera green (left, middle, right) [0..100] %
        int16_t camB[60];  // camera blue (left, middle, right) [0..100] %
        int16_t energy;
        int16_t user[256];
    } variables;

public:
    AsebaFeedableEPuck(std::string robotName, int16_t nodeId);

    // from FeedableEPuck

    void controlStep(double dt) override;

    // from AbstractNodeGlue

    const AsebaVMDescription* getDescription() const override;
    const AsebaLocalEventDescription* getLocalEventsDescriptions() const override;
    const AsebaNativeFunctionDescription* const* getNativeFunctionsDescriptions() const override;
    void callNativeFunction(uint16_t id) override;
};
}  // namespace Enki

#endif  // __PLAYGROUND_EPUCK_H
