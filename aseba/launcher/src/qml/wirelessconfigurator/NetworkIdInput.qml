import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import ".."

TextField  {
    color: "white"
    font.capitalization: Font.AllUppercase
    font.pointSize: 16
    font.bold: true
    background: Rectangle {
        radius: 5
        color: Style.light
    }
    validator: RegExpValidator { regExp: /[0-9A-Fa-f]+/ }
    maximumLength:4
}
