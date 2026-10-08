/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents 1.0

import Audacity.Podcast 1.0

StyledDialogView {
    id: root

    title: qsTrc("podcast", "Podcast settings")

    contentWidth: 520
    contentHeight: 620

    margins: 16

    PodcastSettingsModel {
        id: settingsModel
    }

    Component.onCompleted: {
        settingsModel.load()
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 12

        StyledFlickable {
            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: width
            contentHeight: content.height

            ColumnLayout {
                id: content

                width: parent.width
                spacing: 12

                StyledTextLabel {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font: ui.theme.bodyBoldFont
                    text: qsTrc("podcast", "Speech detection")
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Frame length")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 1
                    maxValue: 200
                    value: settingsModel.frameMs
                    onValueEdited: function(newValue) { settingsModel.frameMs = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Open threshold above noise floor")
                    measureUnitsSymbol: qsTrc("podcast", "dB")
                    minValue: 0
                    maxValue: 60
                    value: settingsModel.openDb
                    onValueEdited: function(newValue) { settingsModel.openDb = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Close threshold above noise floor")
                    measureUnitsSymbol: qsTrc("podcast", "dB")
                    minValue: 0
                    maxValue: 60
                    value: settingsModel.closeDb
                    onValueEdited: function(newValue) { settingsModel.closeDb = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Hangover")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 0
                    maxValue: 5000
                    value: settingsModel.hangoverMs
                    onValueEdited: function(newValue) { settingsModel.hangoverMs = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Shortest speech region")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 0
                    maxValue: 5000
                    value: settingsModel.minSpeechMs
                    onValueEdited: function(newValue) { settingsModel.minSpeechMs = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Shortest silence")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 0
                    maxValue: 5000
                    value: settingsModel.minSilenceMs
                    onValueEdited: function(newValue) { settingsModel.minSilenceMs = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Margin around speech")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 0
                    maxValue: 2000
                    value: settingsModel.marginMs
                    onValueEdited: function(newValue) { settingsModel.marginMs = newValue }
                }

                CheckBox {
                    Layout.fillWidth: true
                    text: qsTrc("podcast", "Reject bleed from the other microphones")
                    checked: settingsModel.bleedRejection
                    onClicked: {
                        settingsModel.bleedRejection = !settingsModel.bleedRejection
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Bleed margin")
                    measureUnitsSymbol: qsTrc("podcast", "dB")
                    minValue: 0
                    maxValue: 60
                    value: settingsModel.bleedMarginDb
                    onValueEdited: function(newValue) { settingsModel.bleedMarginDb = newValue }
                }

                SeparatorLine { Layout.fillWidth: true }

                StyledTextLabel {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font: ui.theme.bodyBoldFont
                    text: qsTrc("podcast", "Tighten gaps")
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Shortest gap to tighten")
                    measureUnitsSymbol: qsTrc("podcast", "ms")
                    minValue: 0
                    maxValue: 10000
                    value: settingsModel.minGapMs
                    onValueEdited: function(newValue) { settingsModel.minGapMs = newValue }
                }

                SettingRow {
                    Layout.fillWidth: true
                    title: qsTrc("podcast", "Share of a gap to remove")
                    measureUnitsSymbol: qsTrc("podcast", "%")
                    minValue: 0
                    maxValue: 100
                    value: settingsModel.tightenPercent
                    onValueEdited: function(newValue) { settingsModel.tightenPercent = newValue }
                }

                SeparatorLine { Layout.fillWidth: true }

                StyledTextLabel {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font: ui.theme.bodyBoldFont
                    text: qsTrc("podcast", "Per track")
                }

                Repeater {
                    model: settingsModel.trackOverrides

                    delegate: ColumnLayout {
                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        spacing: 8

                        CheckBox {
                            Layout.fillWidth: true
                            text: modelData.title.length > 0 ? modelData.title : qsTrc("podcast", "Untitled track")
                            checked: modelData.overridden
                            onClicked: {
                                settingsModel.setTrackOverrideValue(index, "overridden", !modelData.overridden)
                            }
                        }

                        SettingRow {
                            Layout.fillWidth: true
                            Layout.leftMargin: 24
                            enabled: modelData.overridden
                            title: qsTrc("podcast", "Open threshold")
                            measureUnitsSymbol: qsTrc("podcast", "dB")
                            minValue: 0
                            maxValue: 60
                            value: modelData.openDb
                            onValueEdited: function(newValue) {
                                settingsModel.setTrackOverrideValue(index, "openDb", newValue)
                            }
                        }

                        SettingRow {
                            Layout.fillWidth: true
                            Layout.leftMargin: 24
                            enabled: modelData.overridden
                            title: qsTrc("podcast", "Close threshold")
                            measureUnitsSymbol: qsTrc("podcast", "dB")
                            minValue: 0
                            maxValue: 60
                            value: modelData.closeDb
                            onValueEdited: function(newValue) {
                                settingsModel.setTrackOverrideValue(index, "closeDb", newValue)
                            }
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            Layout.leftMargin: 24
                            enabled: modelData.overridden
                            text: qsTrc("podcast", "Reject bleed on this track")
                            checked: modelData.bleedRejection
                            onClicked: {
                                settingsModel.setTrackOverrideValue(index, "bleedRejection", !modelData.bleedRejection)
                            }
                        }
                    }
                }
            }
        }

        ButtonBox {
            Layout.fillWidth: true

            buttons: [ButtonBoxModel.Cancel, ButtonBoxModel.Apply]

            onStandardButtonClicked: function(buttonId) {
                switch (buttonId) {
                case ButtonBoxModel.Cancel:
                    root.reject()
                    return
                case ButtonBoxModel.Apply:
                    settingsModel.apply()
                    root.hide()
                    return
                }
            }
        }
    }
}
