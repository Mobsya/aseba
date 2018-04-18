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

#ifndef ASEBA_HTTP_INTERFACE_HANDLERS
#define ASEBA_HTTP_INTERFACE_HANDLERS

#include "HttpHandler.h"

namespace Aseba {
namespace Http {
    /**
     * Handles OPTIONS requests for CORS preflighting requests
     */
    class OptionsHandler : public HttpHandler {
    public:
        OptionsHandler();
        ~OptionsHandler() override;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override;
        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;
    };

    class NodesHandler : public HierarchicalTokenHttpHandler, public InterfaceHttpHandler {
    public:
        NodesHandler(HttpInterface* interface);
        ~NodesHandler() override;
    };

    class EventsHandler : public TokenHttpHandler, public InterfaceHttpHandler {
    public:
        EventsHandler(HttpInterface* interface);
        ~EventsHandler() override;

        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;
    };

    class ResetHandler : public TokenHttpHandler, public InterfaceHttpHandler {
    public:
        ResetHandler(HttpInterface* interface);
        ~ResetHandler() override;

        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;
    };

    class LoadHandler : public virtual InterfaceHttpHandler {
    public:
        LoadHandler(HttpInterface* interface);
        ~LoadHandler() override;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override;
        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;
    };

    class NodeInfoHandler : public virtual InterfaceHttpHandler {
    public:
        NodeInfoHandler(HttpInterface* interface);
        ~NodeInfoHandler() override;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override;
        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;
    };

    class VariableOrEventHandler : public virtual WildcardHttpHandler, public virtual InterfaceHttpHandler {
    public:
        VariableOrEventHandler(HttpInterface* interface);
        ~VariableOrEventHandler() override;

        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;

    private:
        void parseJsonForm(std::string content, std::vector<std::string>& values);
    };

    class FileHandler : public InterfaceHttpHandler {
    public:
        FileHandler(HttpInterface* interface);
        ~FileHandler() override;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override;
        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override;

    private:
        std::string filePath(HttpRequest* request) const;
        bool doesFileExist(HttpRequest* request) const;
        static std::map<std::string, std::string> const suffixMapping;
    };
}  // namespace Http
}  // namespace Aseba

#endif
