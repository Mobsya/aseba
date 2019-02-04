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

#ifndef THYMIO_VPL_STANDALONE_H
#define THYMIO_VPL_STANDALONE_H

#include <QSplitter>
#include <QDomDocument>
#include <qt-thymio-dm-client-lib/thymionode.h>

class QTranslator;
class QVBoxLayout;
class QLabel;

namespace Aseba {
/** \addtogroup studio */
/*@{*/

class ThymioVPLStandalone;

struct ThymioVPLStandaloneInterface {
    ThymioVPLStandaloneInterface(ThymioVPLStandalone* vplStandalone);
    void displayCode(const QList<QString>& code, int line);
    void loadAndRun();
    void stop();
    bool saveFile(bool as = false);
    void openFile();
    bool newFile();
    void clearOpenedFileName(bool isModified);
    QString openedFileName() const;

private:
    ThymioVPLStandalone* vplStandalone;
};

namespace ThymioVPL {
    class ThymioVisualProgramming;
}
class AeslEditor;

//! Container for VPL standalone and its code viewer
class ThymioVPLStandalone : public QSplitter {
    Q_OBJECT

public:
    ThymioVPLStandalone(const QUuid &node);
    ~ThymioVPLStandalone() override;

protected:
    void setupWidgets();
    void setupConnections();
    void resizeEvent(QResizeEvent* event) override;
    void resetSizes();
    // void variableValueUpdated(const QString& name, const VariablesDataVector& values) override;
    void closeEvent(QCloseEvent* event) override;
    bool saveFile(bool as);
    void openFile();

protected slots:
    void editorContentChanged();
    void nodeConnected(unsigned node);
    void nodeDisconnected(unsigned node);
    void variablesMemoryEstimatedDirty(unsigned node);
    // void variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector& variables);
    void updateWindowTitle(bool modified);
    void toggleFullScreen();

protected:
    friend struct ThymioVPLStandaloneInterface;
    QUuid m_thymioId;
    std::shared_ptr<mobsya::ThymioNode> m_thymio;



    // std::unique_ptr<Target> target;  //!< pointer to target

    bool useAnyTarget;  //!< if true, allow to connect to non-Thymoi II targets
    bool debugLog;      //!< if true, generate debug log events
    bool execFeedback;  //!< if true, blink executed events, imples debugLog = true

    QLayout* vplLayout;                       //!< layout to add/remove VPL to/from
    ThymioVPL::ThymioVisualProgramming* vpl;  //!< VPL widget
    QLabel* disconnectedMessage;              //!< message for VPL area when disconnected
    QDomDocument savedContent;                //!< saved VPL content across disconnections
    AeslEditor* editor;                       //! viewer of code produced by VPL
    // BytecodeVector bytecode;                  //!< bytecode resulting of last successfull compilation
    unsigned allocatedVariablesCount;  //!< number of allocated variables
    int getDescriptionTimer;           //!< timer to periodically get description after a reconnection
    QString fileName;                  //!< file name of last saved/opened file
};

/*@}*/
}  // namespace Aseba

#endif  // THYMIO_VPL_STANDALONE_H
