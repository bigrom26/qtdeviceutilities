/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.6
import QtQuick.Controls 2.0
import QtDeviceUtilities.TimeDateSettings 1.0

Item {
    property var currentTime: TimeManager.time
    property var newTime: new Date
    property bool editMode: false
    property alias handPressed: mouseArea.pressed

    onEditModeChanged: if (editMode) newTime = new Date

    Rectangle {
        border.color: "#bdbebf"
        border.width: 1
        color: "white"
        anchors.fill: parent

        Connections {
            target: TimeManager
            function onTimeChanged() { if (!mouseArea.pressed) newTime.setSeconds(currentTime.getSeconds()) }
            function onTimeZoneChanged() { Date.timeZoneUpdated() }
        }
        Label {
            id: timeLabel
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 10
            anchors.top: parent.top
            text: currentTime.toTimeString()
        }
        Rectangle {
            id: root
            anchors.fill: parent
            anchors.margins: 40
            color: "white"
            border.color: editMode ? "#d6d6d6" : "#5caa15"
            border.width: Math.round(root.width * 0.120)
            radius: parent.width / 2
            property int handOffset: Math.round(root.width * 0.040)
            antialiasing: true

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                enabled: editMode
                property var handleItem: undefined

                function findHandle(item, point) {
                    if (item.objectName === "handle") {
                        var mapped = mouseArea.mapToItem(item, point.x, point.y)
                        if (item.contains(mapped)) {
                            return item.parent;
                        }
                    }

                    for (var i=0; i < item.children.length; i++) {
                        var ret = findHandle(item.children[i], point)
                        if (ret)
                            return ret;
                    }
                    return undefined;
                }

                onPressed: {
                    handleItem = findHandle(root, Qt.point(mouse.x, mouse.y))
                    currentTime.setSeconds(0);
                    currentTime.setMilliseconds(0);
                    newTime.setSeconds(0);
                    newTime.setMilliseconds(0);
                }

                onReleased: {
                    handleItem = undefined
                }

                onPositionChanged: {
                    if (!handleItem)
                        return;

                    var angle = (90 + Math.atan2((mouse.y-mouseArea.height/2), (mouse.x-mouseArea.width/2))*180/Math.PI)

                    if (handleItem.angle < 60 && handleItem.angle > 0 && angle <= 0) {

                        if (handleItem === hours) {
                            hours.pm = !hours.pm
                        } else {
                            var a = hours.angle - 30
                            if (a > 360) a -= 360
                            hours.angle = a
                        }

                    } else if (handleItem.angle > 300 && handleItem.angle < 360 && angle >= 0) {

                        if (handleItem === hours) {
                            hours.pm = !hours.pm
                        } else {
                            var a = hours.angle + 30
                            if (a < 0) a += 360
                            hours.angle = a
                        }
                    }

                    if (angle < 0) {
                        angle += 360
                    } else if (angle > 360) {
                        angle -= 360
                    }

                    handleItem.angle = angle

                    var newhours = Math.floor(hours.angle / 30);
                    if (hours.pm)
                        newhours += 12

                    newTime.setHours(newhours);
                    newTime.setMinutes(Math.round(minutes.angle / 6));

                    newTime.setSeconds(0);
                    newTime.setMilliseconds(0);

                    TimeManager.time = newTime;
                }
            }
            ClockHand {
                id: minutes
                value: currentTime.getMinutes() * 6
            }
            ClockHand {
                id: hours
                height: root.height / 2 * 0.4 + root.handOffset
                value: (currentTime.getHours() * 30) + (currentTime.getMinutes() * 0.5)
                property bool pm: false
            }
            ClockHand {
                id: seconds
                visible: !editMode
                color: "#46a2da"
                width: root.width * 0.0128
                height: root.height / 2 * 0.74
                value: currentTime.getSeconds() * 6
            }
        }
    }
}
