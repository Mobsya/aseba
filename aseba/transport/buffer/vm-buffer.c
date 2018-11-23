/*
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2010:
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

#include "vm-buffer.h"
#include "common/consts.h"
#include "common/types.h"
#include <string.h>
#include <assert.h>

static unsigned char buffer[ASEBA_MAX_INNER_PACKET_SIZE];
static unsigned buffer_pos;

static void buffer_add(const uint8_t* data, const uint16_t len) {
    uint16_t i = 0;
    while(i < len) {
        /* uncomment this to check for buffer overflow in sent packets
        if (buffer_pos >= ASEBA_MAX_INNER_PACKET_SIZE)
        {
            printf("buffer pos %d max size %d\n", buffer_pos, ASEBA_MAX_INNER_PACKET_SIZE);
            abort();
        }*/
        buffer[buffer_pos++] = data[i++];
    }
}

static void buffer_add_uint8(const uint8_t value) {
    buffer_add(&value, 1);
}

static void buffer_add_uint16(const uint16_t value) {
    const uint16_t temp = bswap16(value);
    buffer_add((const unsigned char*)&temp, 2);
}

static void buffer_add_int16(const int16_t value) {
    const uint16_t temp = bswap16(value);
    buffer_add((const unsigned char*)&temp, 2);
}

static void buffer_add_string(const char* s) {
    uint16_t len = strlen(s);
    buffer_add_uint8((uint8_t)len);
    while(*s)
        buffer_add_uint8(*s++);
}

/* implementation of vm hooks */

void AsebaSendMessage(AsebaVMState* vm, uint16_t type, const void* data, uint16_t size) {
    uint16_t i;

    buffer_pos = 0;
    buffer_add_uint16(type);
    for(i = 0; i < size; i++)
        buffer_add_uint8(((const unsigned char*)data)[i]);

    AsebaSendBuffer(vm, buffer, buffer_pos);
}

#ifdef __BIG_ENDIAN__
void AsebaSendMessageWords(AsebaVMState* vm, uint16_t type, const uint16_t* data, uint16_t count) {
    uint16_t i;

    buffer_pos = 0;
    buffer_add_uint16(type);
    for(i = 0; i < count; i++)
        buffer_add_uint16(data[i]);

    AsebaSendBuffer(vm, buffer, buffer_pos);
}
#endif

void AsebaSendVariables(AsebaVMState* vm, uint16_t start, uint16_t length) {
    uint16_t i;
#ifndef ASEBA_LIMITED_MESSAGE_SIZE  // This is usefull with device that cannot send big packets
                                    // like Thymio Wireless module.
    buffer_pos = 0;
    buffer_add_uint16(ASEBA_MESSAGE_VARIABLES);
    buffer_add_uint16(start);
    for(i = start; i < start + length; i++)
        buffer_add_uint16(vm->variables[i]);

    AsebaSendBuffer(vm, buffer, buffer_pos);
#else
    const uint16_t MAX_VARIABLES_SIZE = ((100 - 6) / 2);
    do {
        uint16_t size;
        buffer_pos = 0;
        buffer_add_uint16(ASEBA_MESSAGE_VARIABLES);
        buffer_add_uint16(start);
        if(length > MAX_VARIABLES_SIZE)
            size = MAX_VARIABLES_SIZE;
        else
            size = length;
        for(i = start; i < start + size; i++)
            buffer_add_uint16(vm->variables[i]);

        AsebaSendBuffer(vm, buffer, buffer_pos);

        start += size;
        length -= size;
    } while(length);
#endif
}

void AsebaSendChangedVariables(AsebaVMState* vm) {

   /*
    * The wirelesss dongle has a max outgoing packet size that isnt really documented
    * anywhere
    */
#ifdef ASEBA_LIMITED_MESSAGE_SIZE
    const uint16_t MAX_PACKET_SIZE = 90;
#else
    const uint16_t MAX_PACKET_SIZE = ASEBA_MAX_OUTER_PACKET_SIZE - 4;
#endif

    buffer_pos = 0;
    int has_modified = 0;
    unsigned size_pos = 0;
    uint16_t first_idx = 0;
    uint16_t idx = 0;
    uint16_t size = 0;
    unsigned old_pos = buffer_pos;

    int has_header = 0;
    for(idx = 0; vm->variablesOld && idx <= vm->variablesSize; idx++) {

        int at_end = idx == vm->variablesSize;

        if(!has_header) {
            if(buffer_pos == 0)
                buffer_add_uint16(ASEBA_MESSAGE_CHANGED_VARIABLES);
            size_pos = buffer_pos;
            buffer_add_uint16(0);  //reserve  2 for start
            buffer_add_uint16(0);  //reserve 2 for size
            has_header = 1;
        }

        int modified = !at_end && vm->variablesOld[idx] != vm->variables[idx];
        if(modified) {
            if(!has_modified) {
                has_modified = 1;
                first_idx = idx;
            }
            buffer_add_int16(vm->variables[idx]);
            size ++;
            vm->variablesOld[idx] = vm->variables[idx];
        }

        int need_to_send_packet = buffer_pos >= MAX_PACKET_SIZE || at_end;

        if((!modified && has_modified) || need_to_send_packet) {
            old_pos = buffer_pos; //save the buffer pos
            buffer_pos = size_pos; //then place the cursor where to read the size
            buffer_add_uint16(first_idx); //write start
            buffer_add_uint16(size); //write size
            buffer_pos = old_pos;
            has_modified = 0;
            size = 0;
            first_idx = 0;
            has_header = 0;
        }

        if(need_to_send_packet) {
            AsebaSendBuffer(vm, buffer, buffer_pos);
            buffer_pos = 0;
        }
    }
}

static void AsebaSendDescriptionHead(AsebaVMState* vm) {
    const AsebaVMDescription* vmDescription = AsebaGetVMDescription(vm);
    const AsebaVariableDescription* namedVariables = vmDescription->variables;
    const AsebaNativeFunctionDescription* const* nativeFunctionsDescription = AsebaGetNativeFunctionsDescriptions(vm);
    const AsebaLocalEventDescription* localEvents = AsebaGetLocalEventsDescriptions(vm);

    uint16_t i = 0;
    buffer_pos = 0;

    buffer_add_uint16(ASEBA_MESSAGE_DESCRIPTION);

    buffer_add_string(vmDescription->name);

    buffer_add_uint16(ASEBA_PROTOCOL_VERSION);

    buffer_add_uint16(vm->bytecodeSize);
    buffer_add_uint16(vm->stackSize);
    buffer_add_uint16(vm->variablesSize);

    // compute the number of variables descriptions
    for(i = 0; namedVariables[i].size; i++)
        ;
    buffer_add_uint16(i);

    // compute the number of local event functions
    for(i = 0; localEvents[i].name; i++)
        ;
    buffer_add_uint16(i);

    // compute the number of native functions
    for(i = 0; nativeFunctionsDescription[i]; i++)
        ;
    buffer_add_uint16(i);

    // send buffer
    AsebaSendBuffer(vm, buffer, buffer_pos);
}

void AsebaSendDescriptionFragment(AsebaVMState* vm, int16_t fragment) {
    const AsebaVMDescription* vmDescription = AsebaGetVMDescription(vm);
    const AsebaVariableDescription* namedVariables = vmDescription->variables;
    const AsebaNativeFunctionDescription* const* nativeFunctionsDescription = AsebaGetNativeFunctionsDescriptions(vm);
    const AsebaLocalEventDescription* localEvents = AsebaGetLocalEventsDescriptions(vm);

    if(fragment < 0)
        AsebaSendDescriptionHead(vm);

    int send_all = fragment == -2;
    uint16_t total = 0;
    uint16_t i = 0;

    // send named variables description
    for(i = 0; namedVariables[i].name; i++, total++) {

        if(!send_all  && fragment != total )
            continue;

        buffer_pos = 0;

        buffer_add_uint16(ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION);

        buffer_add_uint16(namedVariables[i].size);
        buffer_add_string(namedVariables[i].name);

        // send buffer
        AsebaSendBuffer(vm, buffer, buffer_pos);
    }

    // send local events description
    for(i = 0; localEvents[i].name; i++, total++) {

        if(!send_all  && fragment != total )
            continue;

        buffer_pos = 0;

        buffer_add_uint16(ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION);

        buffer_add_string(localEvents[i].name);
        buffer_add_string(localEvents[i].doc);

        // send buffer
        AsebaSendBuffer(vm, buffer, buffer_pos);
    }

    // send native functions description
    for(i = 0; nativeFunctionsDescription[i]; i++, total++) {

        if(!send_all  && fragment != total )
            continue;

        uint16_t j;

        buffer_pos = 0;

        buffer_add_uint16(ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION);


        buffer_add_string(nativeFunctionsDescription[i]->name);
        buffer_add_string(nativeFunctionsDescription[i]->doc);
        for(j = 0; nativeFunctionsDescription[i]->arguments[j].size; j++)
            ;
        buffer_add_uint16(j);
        for(j = 0; nativeFunctionsDescription[i]->arguments[j].size; j++) {
            buffer_add_int16(nativeFunctionsDescription[i]->arguments[j].size);
            buffer_add_string(nativeFunctionsDescription[i]->arguments[j].name);
        }

        // send buffer
        AsebaSendBuffer(vm, buffer, buffer_pos);
    }
}

void AsebaSendDescription(AsebaVMState* vm) {
    AsebaSendDescriptionFragment(vm, -2);
}

void AsebaProcessIncomingEvents(AsebaVMState* vm) {
    uint16_t source;
    const AsebaVMDescription* desc = AsebaGetVMDescription(vm);

    uint16_t amount = AsebaGetBuffer(vm, buffer, ASEBA_MAX_INNER_PACKET_SIZE, &source);

    if(amount > 0) {
        uint16_t type = bswap16(((uint16_t*)buffer)[0]);
        uint16_t* payload = (uint16_t*)(buffer + 2);
        uint16_t payloadSize = (amount - 2) / 2;
        if(type < 0x8000) {
            // user message, only process if we are not stepping inside an event
            if(AsebaMaskIsClear(vm->flags, ASEBA_VM_STEP_BY_STEP_MASK) ||
               AsebaMaskIsClear(vm->flags, ASEBA_VM_EVENT_ACTIVE_MASK)) {
                // by convention. the source begin at variables, address 1
                // then it's followed by the args
                uint16_t argPos = desc->variables[1].size;
                uint16_t argsSize = desc->variables[2].size;
                uint16_t i;
                vm->variables[argPos++] = source;
                for(i = 0; (i < argsSize) && (i < payloadSize); i++)
                    vm->variables[argPos + i] = bswap16(payload[i]);
                AsebaVMSetupEvent(vm, type);
            }
        } else {
            // debug message
            AsebaVMDebugMessage(vm, type, payload, payloadSize);
        }
    }
}
