import QtQuick 2.12
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
        color: Style.mid
    }
    validator: RegExpValidator { regExp: /[0-9A-Fa-f]+/ }
    maximumLength:4
}
