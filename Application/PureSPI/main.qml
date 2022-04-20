import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.0
import QtQml 2.1

Window {
    width: 320
    height: 240
    visible: true
    title: qsTr("Hello World")

    TextField {
        id: textfield_input
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.right: parent.right
        anchors.rightMargin: 20
        placeholderText: qsTr("Enter less than 9 characters")
        enabled: true
        onAccepted: {
            sendDataOnSPI();
        }
    }

    Button {
        id: button_SendSPI
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: textfield_input.bottom
        anchors.topMargin: 10
        text: qsTr("Send SPI Data")
        onClicked: {
            sendDataOnSPI();
        }
    }

    Button {
        anchors.left: button_SendSPI.right
        anchors.leftMargin: 20
        anchors.top: textfield_input.bottom
        anchors.topMargin: 10
        text: qsTr("Read SPI")
        onClicked: {
            guibackObj.readFromSPI()
        }
    }

    Text {
        id: text_status
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

    //    Timer {
    //        id: timer_status
    //        interval: 2000
    //        running: false
    //        repeat: false
    //        onTriggered: updateTransferStatus()
    //    }

    function sendDataOnSPI(){
        //        if(textfield_input.text != ""){
//        console.log("Writing data to SPI");
        textfield_input.enabled = false;
        textfield_input.text = "ABCDFGHJQ";
        text_status.text = "";
        guibackObj.writeToSPI(textfield_input.text);
        //        }
    }

    Item {
        Connections {
            target: guibackObj
            onSendstatusChanged: {
                if(b === true){
                    //                    timer_status.start();
                    textfield_input.text = "";
                    textfield_input.enabled = true;
                    text_status.text = "Message sent successfully";
                }
                else {
                    console.log("Problem in Writing data to SPI");
                }
            }
        }
    }

    function updateTransferStatus() {
        textfield_input.text = "";
        textfield_input.enabled = true;
    }

}
