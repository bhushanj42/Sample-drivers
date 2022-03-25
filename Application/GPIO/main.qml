import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Text {
        id: name_txt
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20
//        text: qsTr("Some text")
        text: guibackObj.sometext
    }

    Button{
        id: button_red
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: name_txt.bottom
        anchors.topMargin: 20
        text: qsTr("Red button")
        onClicked: {
//            name_txt.text = "Red button clicked"
            guibackObj.redButtonClicked();
        }
    }

    Button{
        id: button_green
        anchors.left: button_red.right
        anchors.leftMargin: 20
        anchors.top: name_txt.bottom
        anchors.topMargin: 20
        text: qsTr("Green button")
        onClicked: {
//            name_txt.text = "Green button clicked"
            guibackObj.greenButtonClicked();
        }
    }
}
