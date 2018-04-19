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

#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QWidget>
#include <QString>
#include <QDialog>
#include <map>

#define CONFIG_PROPERTY_CHECKBOX_DECLARE(name) \
public:                                        \
    static bool get##name(void);               \
    static void set##name(bool);

class QStackedWidget;
class QVBoxLayout;
class QPushButton;
class QListWidget;
class QCheckBox;

namespace Aseba {
class GeneralPage;
class EditorPage;

// ConfigDialog
class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    // instantiate / kill the singleton
    static void init(QWidget* parent = nullptr);
    static void bye();
    static const ConfigDialog* getInstance() {
        return me;
    }

    static void showConfig();

    // access to properties
    // layout properties
    CONFIG_PROPERTY_CHECKBOX_DECLARE(ShowHidden)
    CONFIG_PROPERTY_CHECKBOX_DECLARE(ShowLineNumbers)
    CONFIG_PROPERTY_CHECKBOX_DECLARE(ShowKeywordToolbar)
    CONFIG_PROPERTY_CHECKBOX_DECLARE(ShowMemoryUsage)
    // autocompletion behaviour
    CONFIG_PROPERTY_CHECKBOX_DECLARE(AutoCompletion)

signals:
    void settingsChanged();

protected:
    ConfigDialog(QWidget* parent = nullptr);
    ~ConfigDialog() override;
    void setupWidgets();

protected:
    static ConfigDialog* me;  // pointer to the singleton

    QPushButton* okButton;
    QPushButton* cancelButton;
    QListWidget* topicList;
    QStackedWidget* configStack;

    // individual pages
    GeneralPage* generalpage;
    EditorPage* editorpage;

    void readSettings();
    void writeSettings();

public slots:
    virtual void saveState();
    virtual void reloadFromCache();
    void accept() override;
    void reject() override;

protected slots:
    void flushCache();
};

// ConfigPage
class ConfigPage : public QWidget {
    Q_OBJECT

public:
    ConfigPage(QString title = QString(), QWidget* parent = nullptr);
    ~ConfigPage() override = default;

    friend class ConfigDialog;

protected:
    virtual void readSettings();
    virtual void writeSettings();

    virtual void saveState();
    virtual void flushCache();
    virtual void discardChanges();
    virtual void reloadFromCache();

    QCheckBox* newCheckbox(QString label, QString ID, bool checked = false);

protected:
    template <class T>
    struct WidgetCache {
        WidgetCache() = default;
        WidgetCache(QWidget* widget, T value) : widget(widget), value(value) {}
        QWidget* widget;
        T value;
    };
    std::map<QString, WidgetCache<bool> > checkboxCache;
    std::map<QString, WidgetCache<bool> > checkboxCacheSave;  // used to save the state prior to changes

    QVBoxLayout* mainLayout;
};

// GeneralPage
class GeneralPage : public ConfigPage {
    Q_OBJECT

public:
    GeneralPage(QWidget* parent = nullptr);
    ~GeneralPage() override = default;

    friend class ConfigDialog;

protected:
    void readSettings() override;
    void writeSettings() override;
};

// EditorPage
class EditorPage : public ConfigPage {
    Q_OBJECT

public:
    EditorPage(QWidget* parent = nullptr);
    ~EditorPage() override = default;

    friend class ConfigDialog;

protected:
    void readSettings() override;
    void writeSettings() override;
};
}  // namespace Aseba

#endif  // CONFIG_DIALOG_H
