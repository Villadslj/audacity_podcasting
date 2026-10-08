/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.UiComponents 1.0

RowLayout {
    id: root

    property string title: ""
    property alias value: control.currentValue
    property alias measureUnitsSymbol: control.measureUnitsSymbol
    property alias minValue: control.minValue
    property alias maxValue: control.maxValue
    property alias step: control.step
    property alias decimals: control.decimals

    signal valueEdited(var newValue)

    spacing: 12

    StyledTextLabel {
        Layout.fillWidth: true

        horizontalAlignment: Text.AlignLeft
        text: root.title
    }

    IncrementalPropertyControl {
        id: control

        Layout.preferredWidth: 104

        decimals: 1
        step: 1

        onValueEdited: function(newValue) {
            root.valueEdited(newValue)
        }
    }
}
