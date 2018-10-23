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

#include "StudioAeslEditor.h"
#include "NodeTab.h"
#include <QtWidgets>
#include <QtGlobal>

#include <cassert>

namespace Aseba {
/** \addtogroup studio */
/*@{*/

StudioAeslEditor::StudioAeslEditor(const ScriptTab* tab) : tab(tab), dropSourceWidget(nullptr) {}

void StudioAeslEditor::dropEvent(QDropEvent* event) {
    dropSourceWidget = dynamic_cast<QWidget*>(event->source());
    QTextEdit::dropEvent(event);
    dropSourceWidget = nullptr;
    setFocus(Qt::MouseFocusReason);
}

void StudioAeslEditor::insertFromMimeData(const QMimeData* source) {
    QTextCursor cursor(this->textCursor());

    // check whether we are at the beginning of a line
    bool startOfLine(cursor.atBlockStart());
    const int posInBlock(cursor.position() - cursor.block().position());
    if(!startOfLine && posInBlock) {
        const QString startText(cursor.block().text().left(posInBlock));
        startOfLine = !startText.contains(QRegExp("\\S"));
    }
    if(!(dropSourceWidget && startOfLine)) {
        cursor.insertText(source->text());
        return;
    }


    // if beginning of a line and source widget is known, add some helper text
    const auto* nodeTab(dynamic_cast<const NodeTab*>(tab));
    assert(nodeTab);
    QString prefix(QLatin1String(""));   // before the text
    QString midfix(QLatin1String(""));   // between the text and the cursor
    QString postfix(QLatin1String(""));  // after the curser
    if(dropSourceWidget == nodeTab->vmFunctionsView) {
        prefix = "call ";
    } else if(dropSourceWidget == nodeTab->vmLocalEvents) {
        // inserting local event
        prefix = "onevent ";
        midfix = "\n";
    } /*else if(dropSourceWidget == nodeTab->mainWindow->eventsDescriptionsView) {
        // inserting global event
        prefix = "onevent ";
        midfix = "\n";
    }*/

    cursor.beginEditBlock();
    cursor.insertText(prefix + source->text() + midfix);
    const int pos = cursor.position();
    cursor.insertText(postfix);
    cursor.setPosition(pos);
    cursor.endEditBlock();

    this->setTextCursor(cursor);
}

}  // namespace Aseba
