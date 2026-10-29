import QtQuick
import HillClimb

// Drop-shadowed text whose (x, y) is the start of its baseline, like
// QPainter::drawText(QPointF, ...).
Item {
    id: root

    property string text
    property font font
    property color color: "white"

    Text {
        x: 2
        y: 2 - baselineOffset
        text: root.text
        textFormat: Text.PlainText
        font: root.font
        color: Theme.rgb(8, 12, 16, 240)
    }

    Text {
        y: -baselineOffset
        text: root.text
        textFormat: Text.PlainText
        font: root.font
        color: root.color
    }
}
