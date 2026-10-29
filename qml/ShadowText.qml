import QtQuick
import HillClimb

// Text with a solid drop shadow, aligned inside its box like
// QPainter::drawText(QRectF, Qt::AlignCenter, ...).
Item {
    id: root

    property string text
    property font font
    property color color: "white"
    property color shadowColor: Theme.rgb(8, 12, 16, 240)
    property real shadowX: 2
    property real shadowY: 2
    property int horizontalAlignment: Text.AlignHCenter
    property int verticalAlignment: Text.AlignVCenter

    Text {
        x: root.shadowX
        y: root.shadowY
        width: root.width
        height: root.height
        text: root.text
        textFormat: Text.PlainText
        font: root.font
        color: root.shadowColor
        horizontalAlignment: root.horizontalAlignment
        verticalAlignment: root.verticalAlignment
    }

    Text {
        width: root.width
        height: root.height
        text: root.text
        textFormat: Text.PlainText
        font: root.font
        color: root.color
        horizontalAlignment: root.horizontalAlignment
        verticalAlignment: root.verticalAlignment
    }
}
