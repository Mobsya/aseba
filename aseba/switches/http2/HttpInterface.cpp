/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

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

#include <algorithm>
#include <iostream>
#include <sstream>
#include "HttpInterface.h"
#include "HttpInterfaceHandlers.h"

#define RECONNECT_TIMEOUT 1000

using Aseba::Http::EventsHandler;
using Aseba::Http::HttpDashelTarget;
using Aseba::Http::HttpInterface;
using Aseba::Http::LoadHandler;
using Aseba::Http::NodesHandler;
using Aseba::Http::OptionsHandler;
using Aseba::Http::ResetHandler;
using std::cerr;
using std::endl;
using std::find;
using std::istringstream;
using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::stringstream;
using std::vector;

HttpInterface::HttpInterface(const std::string& httpPort)
    : verbose(true), program(defaultProgram.c_str(), defaultProgram.size()) {
    addSubhandler(new OptionsHandler());
    addSubhandler(new LoadHandler(this));
    addSubhandler(new NodesHandler(this));
    addSubhandler(new EventsHandler(this));
    addSubhandler(new ResetHandler(this));
    addSubhandler(new FileHandler(this));

    // listen for incoming HTTP requests
    httpStream = connect("tcpin:port=" + httpPort);
}

HttpInterface::~HttpInterface() {}

bool HttpInterface::addTarget(const std::string& address) {
    if(targetAddressStreams.find(address) == targetAddressStreams.end()) {  // target is not yet in our list of targets
        targetAddressStreams[address] = nullptr;                            // mark this as a new target to initialize
        targetAddressReconnectionTime[address] = UnifiedTime(0);
        return true;
    }

    return false;
}

void HttpInterface::step() {
    auto end = targetAddressStreams.end();
    for(auto iter = targetAddressStreams.begin(); iter != end; iter++) {
        const std::string& address = iter->first;

        if(iter->second == nullptr) {
            UnifiedTime now;
            if((now - targetAddressReconnectionTime[address]).value > RECONNECT_TIMEOUT) {
                // trying to reconnect to disconnected targets
                Dashel::Stream* stream = nullptr;
                try {
                    stream = connect(address);
                    targets[stream] = new HttpDashelTarget(this, address, stream);
                    iter->second = stream;
                    if(verbose) {
                        cerr << "Successfully connected target " << address << " with stream " << stream << endl;
                    }
                } catch(Dashel::DashelException e) {
                    if(verbose) {
                        cerr << "Failed to connect target " << address << ": " << e.what() << endl;
                    }

                    if(stream != nullptr) {  // uncreate target
                        auto query = targets.find(stream);
                        if(query != targets.end()) {
                            delete query->second;
                            targets.erase(query);
                        }
                    }

                    targetAddressReconnectionTime[address] = now;
                }
                // ping the connected networks
                for(auto iter = targets.begin(); iter != targets.end();
                    ++iter) {
                    HttpDashelTarget* target = iter->second;
                    target->pingNetwork();
                }
            }
        }
    }

    closeClosingHttpConnections();
    sendHttpResponses();

    Dashel::Hub::step(2);
}

bool HttpInterface::sendEvent(const std::vector<std::string>& args) {
    auto end = targets.begin();
    for(auto iter = targets.begin(); iter != end; ++iter) {
        HttpDashelTarget* target = iter->second;

        if(!target->sendEvent(args)) {
            return false;
        }
    }

    return true;
}

void HttpInterface::notifyEventSubscribers(const std::string& event, const std::vector<int16_t>& data) {
    // set up SSE message
    std::stringstream reply;
    reply << "data: " << event;
    auto dataSize = (int)data.size();
    for(int i = 0; i < dataSize; i++) {
        reply << " " << data[i];
    }
    reply << "\r\n\r\n";
    string replyString = reply.str();

    auto end = httpConnections.end();
    for(auto iter = httpConnections.begin(); iter != end; ++iter) {
        Dashel::Stream* stream = iter->first;
        HttpConnection& connection = iter->second;

        if(connection.eventSubscriptions.find("*") != connection.eventSubscriptions.end() ||
           connection.eventSubscriptions.find(event) != connection.eventSubscriptions.end()) {
            try {
                stream->write(replyString.c_str(), replyString.size());
                stream->flush();
            } catch(Dashel::DashelException e) {
                if(verbose) {
                    cerr << stream << " Failed to send HTTP event subscriber notification: " << e.what() << endl;
                }
                closingHttpConnections.insert(stream);
            }
        }
    }
}

void HttpInterface::addEventSubscription(HttpRequest* request, const std::string& subscription) {
    assert(dynamic_cast<DashelHttpRequest*>(request) != nullptr);
    auto* dashelHttpRequest = static_cast<DashelHttpRequest*>(request);
    Dashel::Stream* stream = dashelHttpRequest->getStream();

    httpConnections[stream].eventSubscriptions.insert(subscription);

    if(verbose) {
        cerr << stream << " Added HTTP request " << request << " event subscription for '" << subscription << "'"
             << endl;
    }
}

unsigned HttpInterface::registerNode(HttpDashelTarget* target, unsigned localNodeId) {
    bool found = false;
    unsigned globalNodeId = localNodeId;

    while(!found) {
        auto query = nodeIds.find(globalNodeId);
        if(query != nodeIds.end()) {
            globalNodeId++;
        } else {
            found = true;
        }
    }

    nodeIds[globalNodeId] = make_pair(target, localNodeId);

    if(verbose) {
        cerr << "Registered node with remapped global id " << globalNodeId << " from target " << target->getAddress()
             << " with local id " << localNodeId << endl;
    }

    return globalNodeId;
}

bool HttpInterface::runProgram(std::string& errorString) {
    const vector<AeslProgram::NodeEntry>& entries = program.getEntries();
    for(int i = 0; i < (int)entries.size(); i++) {
        const AeslProgram::NodeEntry& entry = entries[i];

        set<pair<HttpDashelTarget*, const HttpDashelTarget::Node*> > matchingNodes;

        if(!entry.nodeId.empty()) {  // if there is a node id, both it and the name have to match exactly
            pair<HttpDashelTarget*, const HttpDashelTarget::Node*> nodeReference = getNodeByIdString(entry.nodeId);

            if(entry.nodeName.empty() || nodeReference.second->name == entry.nodeName) {
                matchingNodes.insert(nodeReference);
            }
        } else {  // if there is only a name, multiples can match
            matchingNodes = getNodesByName(entry.nodeName);
        }

        if(matchingNodes.empty()) {
            if(verbose) {
                cerr << "Warning: No target nodes match program entry with node name '" << entry.nodeName
                     << "' and node id '" << entry.nodeId << "', skipping..." << endl;
            }
            continue;
        }

        auto end = matchingNodes.end();
        for(auto iter = matchingNodes.begin();
            iter != end; ++iter) {
            HttpDashelTarget* target = iter->first;
            const HttpDashelTarget::Node* node = iter->second;

            if(!target->compileAndRunCode(node->globalId, entry.code, errorString)) {
                return false;
            }

            if(verbose) {
                cerr << "Compiled and sent AESL entry for node " << (entry.nodeId.empty() ? "*" : entry.nodeId) << " ("
                     << (entry.nodeName.empty() ? "*" : entry.nodeName) << ") to node " << node->globalId << " ("
                     << node->name << ")" << endl;
            }
        }
    }

    return true;
}

std::set<std::pair<HttpDashelTarget*, const HttpDashelTarget::Node*> >
HttpInterface::getNodesByName(const std::string& name) {
    set<pair<HttpDashelTarget*, const HttpDashelTarget::Node*> > results;

    // search by name
    auto end = targets.end();
    for(auto iter = targets.begin(); iter != end; ++iter) {
        HttpDashelTarget* target = iter->second;

        set<const HttpDashelTarget::Node*> nodes = target->getNodesByName(name);
        auto nodesEnd = nodes.end();
        for(auto nodesIter = nodes.begin(); nodesIter != nodesEnd;
            ++nodesIter) {
            const HttpDashelTarget::Node* node = *nodesIter;
            results.insert(make_pair(target, node));
        }
    }

    return results;
}

std::pair<HttpDashelTarget*, const HttpDashelTarget::Node*>
HttpInterface::getNodeByIdString(const std::string& nodeIdString) {
    unsigned nodeId;
    if(istringstream(nodeIdString) >> nodeId) {
        return getNodeById(nodeId);
    }

    return make_pair((HttpDashelTarget*)nullptr, (const HttpDashelTarget::Node*)nullptr);
}

std::pair<HttpDashelTarget*, const HttpDashelTarget::Node*> HttpInterface::getNodeById(unsigned nodeId) {
    // search by global id
    auto query = nodeIds.find(nodeId);
    if(query != nodeIds.end()) {
        HttpDashelTarget* target = query->second.first;
        const HttpDashelTarget::Node* node = target->getNodeById(nodeId);

        if(node != nullptr) {
            return make_pair(target, node);
        }
    }

    return make_pair((HttpDashelTarget*)nullptr, (const HttpDashelTarget::Node*)nullptr);
}

std::set<std::pair<HttpDashelTarget*, const HttpDashelTarget::Node*> >
HttpInterface::getNodesByNameOrId(const std::string& nameOrId) {
    set<pair<HttpDashelTarget*, const HttpDashelTarget::Node*> > result = getNodesByName(nameOrId);

    if(result.empty()) {
        std::pair<HttpDashelTarget*, const HttpDashelTarget::Node*> nodeReference = getNodeByIdString(nameOrId);
        if(nodeReference.first != nullptr) {
            result.insert(nodeReference);
        }
    }

    return result;
}

void HttpInterface::connectionCreated(Dashel::Stream* stream) {
    if(verbose) {
        cerr << stream << " Incoming connection from " << stream->getTargetName() << endl;
    }
}

void HttpInterface::connectionClosed(Dashel::Stream* stream, bool abnormal) {
    // handle dashel target disconnection
    auto targetQuery = targets.find(stream);
    if(targetQuery != targets.end()) {  // target stream
        HttpDashelTarget* target = targetQuery->second;

        const std::map<unsigned, HttpDashelTarget::Node>& nodes = target->getNodes();
        auto end = nodes.end();
        for(auto iter = nodes.begin(); iter != end; ++iter) {
            const HttpDashelTarget::Node& node = iter->second;

            // free up global node id
            nodeIds.erase(node.globalId);

            // cancel all pending variable requests for the disconnected node
            auto pendingVariablesEnd =
                node.pendingVariables.end();
            for(auto pendingVariablesIter =
                    node.pendingVariables.begin();
                pendingVariablesIter != pendingVariablesEnd; ++pendingVariablesIter) {
                const set<pair<Dashel::Stream*, DashelHttpRequest*> >& pendingRequests = pendingVariablesIter->second;

                auto pendingRequestsEnd =
                    pendingRequests.end();
                for(auto pendingRequestsIter =
                        pendingRequests.begin();
                    pendingRequestsIter != pendingRequestsEnd; ++pendingRequestsIter) {
                    Dashel::Stream* stream = pendingRequestsIter->first;
                    DashelHttpRequest* request = pendingRequestsIter->second;

                    // We store pending requests as pair<stream, request> to be able to check here
                    // if they still exist. It could be that a request was pending but then
                    // destroyed because the HTTP connection died, so we need to first make sure
                    // everything is still okay before trusting these pointers...
                    map<Dashel::Stream*, HttpConnection>::const_iterator connectionQuery = httpConnections.find(stream);
                    if(connectionQuery != httpConnections.end()) {  // we can trust stream for here on...
                        const HttpConnection& connection = connectionQuery->second;
                        if(find(connection.queue.begin(), connection.queue.end(), request) !=
                           connection.queue.end()) {  // we can trust request from here on...
                            request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
                        }
                    }
                }
            }

            sendHttpResponses();

            // notify event subscribers about the disconnected node
            std::vector<int16_t> data;
            data.push_back(node.globalId);

            notifyEventSubscribers("disconnect", data);
        }

        if(verbose) {
            cerr << "Target " << target->getAddress() << " disconnected, trying to reconnect..." << endl;
        }

        targetAddressStreams[target->getAddress()] = nullptr;
        targetAddressReconnectionTime[target->getAddress()] = UnifiedTime(0);
        targets.erase(targetQuery);

        delete target;

        return;
    }

    // handle http connection disconnection
    auto httpQuery = httpConnections.find(stream);
    if(httpQuery != httpConnections.end()) {  // http connection
        if(verbose) {
            cerr << stream << " HTTP connection reset by peer" << endl;
        }
        closeHttpConnection(httpQuery->second);

        return;
    }
}

void HttpInterface::incomingData(Dashel::Stream* stream) {
    auto query = targets.find(stream);
    if(query != targets.end()) {  // target stream
        HttpDashelTarget* target = query->second;

        Message* message(Message::receive(stream));

        // pass message to description manager, which builds the node descriptions in background
        // warning: do this before dynamic casts because otherwise the parsing doesn't work (why?)
        target->processMessage(message);

        // See if we know this node already or if the source is 0 (meaning coming from IDE)
        const HttpDashelTarget::Node* node = target->getNodeByLocalId(message->source);
        if(node != nullptr || message->source == 0) {  // else if we cannot remap the source,
                                                       // discard the message and do not relay it
            // remap source node to global node id if not sent from IDE
            if(node != nullptr)
                message->source = node->globalId;

            // check for execution error messages
            switch(message->type) {
                case ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS:
                case ASEBA_MESSAGE_DIVISION_BY_ZERO:
                case ASEBA_MESSAGE_EVENT_EXECUTION_KILLED:
                case ASEBA_MESSAGE_NODE_SPECIFIC_ERROR: incomingErrorMessage(target, message);
            }

            // if variables, check for pending requests
            const Variables* variables(dynamic_cast<Variables*>(message));
            if(variables != nullptr) {
                incomingVariables(target, variables);
            }

            // if event, retransmit it on an HTTP SSE channel if one exists
            const UserMessage* userMessage(dynamic_cast<UserMessage*>(message));
            if(userMessage != nullptr) {
                incomingUserMessage(target, userMessage);
            }

            // act like asebaswitch: rebroadcast this message to the other streams
            auto* cmdMessage(dynamic_cast<CmdMessage*>(message));
            if(cmdMessage != nullptr) {  // targeted message, only rebroadcast to correct target
                                         // with remapped destination
                auto query =
                    nodeIds.find(cmdMessage->dest);
                if(query != nodeIds.end()) {  // only relay it to known targets, else if we cannot remap
                                              // the target, discard the message and do not relay it
                    HttpDashelTarget* target = query->second.first;
                    unsigned localNodeId = query->second.second;

                    try {
                        cmdMessage->dest = localNodeId;
                        cmdMessage->serialize(target->getStream());
                        target->getStream()->flush();
                    } catch(Dashel::DashelException e) {
                        cerr << "Error while rebroadcasting to stream " << target->getStream() << " of target "
                             << target->getAddress() << endl;
                    }
                }
            } else {  // not a targeted message, just rebroadcast to all other targets
                map<Dashel::Stream*, HttpDashelTarget*>::const_iterator end = targets.end();
                for(map<Dashel::Stream*, HttpDashelTarget*>::const_iterator iter = targets.begin();
                    iter != targets.end(); ++iter) {
                    Dashel::Stream* outStream = iter->first;

                    if(outStream != stream) {
                        try {
                            message->serialize(outStream);
                            outStream->flush();
                        } catch(Dashel::DashelException e) {
                            cerr << "Error while rebroadcasting to stream " << outStream << " of target "
                                 << iter->second->getAddress() << endl;
                        }
                    }
                }
            }
        }

        delete message;
    } else {  // HTTP stream
        HttpConnection& connection = httpConnections[stream];
        connection.stream = stream;

        auto* request = new DashelHttpRequest(stream);

        if(verbose) {
            cerr << stream << " Incoming HTTP connection, creating request " << request << endl;
            request->setVerbose(verbose);
        }

        try {
            if(!request->receive()) {
                request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
                request->respond().send();

                stream->fail(Dashel::DashelException::Unknown, 0, "400 Bad request");
                closeHttpConnection(connection);

                delete request;
                return;
            }
        } catch(Dashel::DashelException e) {
            cerr << stream << " Incoming HTTP connection unexpectedly closed: " << e.what() << endl;
            closeHttpConnection(connection);
            delete request;
            return;
        }

        connection.queue.push_back(request);

        handleRequest(request, request->getTokens());

        // run response queues immediately to save time
        sendHttpResponses();
    }
}

void HttpInterface::sendHttpResponses() {
    auto end = httpConnections.end();
    for(auto iter = httpConnections.begin(); iter != end; ++iter) {
        HttpConnection& connection = iter->second;

        while(!connection.queue.empty() && connection.queue.front()->isResponseReady()) {
            HttpRequest* request = connection.queue.front();

            try {
                request->respond().send();
            } catch(Dashel::DashelException e) {
                if(verbose) {
                    cerr << request << " Failed to send HTTP response: " << e.what() << endl;
                }
                closingHttpConnections.insert(connection.stream);
                break;
            }

            if(request->getHeader("Connection") == "close" ||
               (request->getProtocol() == "HTTP/1.0" && request->getHeader("Connection") != "keep-alive")) {
                closingHttpConnections.insert(connection.stream);
                break;
            } else {
                // just this request is finished, delete and remove from queue
                delete request;
                connection.queue.pop_front();
            }
        }
    }
}

void HttpInterface::closeClosingHttpConnections() {
    auto end = closingHttpConnections.end();
    for(auto iter = closingHttpConnections.begin(); iter != end; ++iter) {
        auto query = httpConnections.find(*iter);
        if(query != httpConnections.end()) {  // make sure the connection exists, it could have been closed already
            closeHttpConnection(query->second);
        }
    }

    closingHttpConnections.clear();
}

void HttpInterface::closeHttpConnection(HttpConnection& connection) {
    // delete all requests that are still in the queue
    while(!connection.queue.empty()) {
        delete connection.queue.front();
        connection.queue.pop_front();
    }

    // disconnect the stream if not yet done so
    if(!connection.stream->failed()) {
        try {
            connection.stream->fail(Dashel::DashelException::Unknown, 0, "Request handling complete");
        } catch(Dashel::DashelException& e) {
            if(verbose) {
                cerr << connection.stream << " Closed HTTP connection" << endl;
            }
        }
    }

    httpConnections.erase(connection.stream);
}

void HttpInterface::incomingVariables(HttpDashelTarget* target, const Variables* variables) {
    if(verbose) {
        cerr << "Incoming variable for target " << target->getAddress() << endl;
    }

    // first, build result string from message
    stringstream result;
    result << "[";
    auto numVariables = (int)variables->variables.size();
    for(int i = 0; i < numVariables; ++i) {
        result << (i ? "," : "") << variables->variables[i];
    }
    result << "]";
    string resultString = result.str();

    const HttpDashelTarget::Node* node = target->getNodeById(variables->source);

    if(node != nullptr) {
        auto query =
            node->pendingVariables.find(variables->start);

        if(query != node->pendingVariables.end()) {
            const set<pair<Dashel::Stream*, DashelHttpRequest*> >& pendingRequests = query->second;

            auto end = pendingRequests.end();
            for(auto iter = pendingRequests.begin();
                iter != end; ++iter) {
                Dashel::Stream* stream = iter->first;
                DashelHttpRequest* request = iter->second;

                // We store pending requests as pair<stream, request> to be able to check here if
                // they still exist. It could be that a request was pending but then destroyed
                // because the HTTP connection died, so we need to first make sure everything is
                // still okay before trusting these pointers...
                map<Dashel::Stream*, HttpConnection>::const_iterator connectionQuery = httpConnections.find(stream);
                if(connectionQuery != httpConnections.end()) {  // we can trust stream for here on...
                    const HttpConnection& connection = connectionQuery->second;
                    if(find(connection.queue.begin(), connection.queue.end(), request) !=
                       connection.queue.end()) {  // we can trust request from here on...
                        request->respond().setContent(resultString);
                    } else {
                        if(verbose) {
                            cerr << "Target " << target->getAddress() << " received variables for node "
                                 << node->globalId << " (" << node->name
                                 << "), but the originating HTTP request was discarded in the "
                                    "meanwhile"
                                 << endl;
                        }
                    }
                } else {
                    if(verbose) {
                        cerr << "Target " << target->getAddress() << " received variables for node " << node->globalId
                             << " (" << node->name
                             << "), but the originating HTTP connection was closed in the meanwhile" << endl;
                    }
                }
            }

            target->removePendingVariable(node->globalId, variables->start);
        } else if(verbose) {
            cerr << "Target " << target->getAddress() << " received variables for node " << node->globalId << " ("
                 << node->name << "), but there was no pending variable request for them" << endl;
        }
    } else if(verbose) {
        cerr << "Target " << target->getAddress() << " received variables for node " << variables->source
             << " but no node with this id is known" << endl;
    }

    sendHttpResponses();
}

void HttpInterface::incomingUserMessage(HttpDashelTarget* target, const UserMessage* userMessage) {
    if(verbose) {
        cerr << "Incoming user message for target " << target->getAddress() << endl;
    }

    const CommonDefinitions& commonDefinitions = program.getCommonDefinitions();

    if(userMessage->type >= 0 && userMessage->type < commonDefinitions.events.size()) {
        string eventName = WStringToUTF8(commonDefinitions.events[userMessage->type].name);
        notifyEventSubscribers(eventName, userMessage->data);
    } else if(verbose) {
        cerr << "Target " << target->getAddress() << " received user message with type " << userMessage->type
             << ", but no such event is known" << endl;
    }
}

void HttpInterface::incomingErrorMessage(HttpDashelTarget* target, const Message* message) {
    if(verbose) {
        cerr << "Incoming error message for target " << target->getAddress() << endl;
    }

    const char* errorString = nullptr;
    if(message->type == ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS) {
        errorString = "array_access_out_of_bounds";
    } else if(message->type == ASEBA_MESSAGE_DIVISION_BY_ZERO) {
        errorString = "division_by_zero";
    } else if(message->type == ASEBA_MESSAGE_EVENT_EXECUTION_KILLED) {
        errorString = "event_execution_killed";
    } else if(message->type == ASEBA_MESSAGE_NODE_SPECIFIC_ERROR) {
        errorString = "node_specific_error";
    }

    notifyEventSubscribers(errorString);
}

const std::string HttpInterface::defaultProgram =
    "<!DOCTYPE aesl-source><network><keywords flag=\"true\"/><node nodeId=\"1\" "
    "name=\"thymio-II\"></node></network>\n";
