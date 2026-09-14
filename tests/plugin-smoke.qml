import QtQuick
import Quickshell
import "plugin"

Scope {
    property int attempts: 0
    FloatingWindow {
        visible: true; implicitWidth: 160; implicitHeight: 40
        BarWidget { id: widget }
    }
    Timer {
        interval: 100; repeat: true; running: true
        onTriggered: {
            attempts++
            if (!widget.player) {
                if (attempts > 50) { console.error("Cloudlane MPRIS player not found"); Qt.exit(1) }
                return
            }
            if (widget.player.desktopEntry !== "io.github.charleszheng44.Cloudlane"
                || widget.player.identity !== "Cloudlane"
                || widget.player.canGoPrevious || !widget.player.canGoNext) {
                console.error("Unexpected Cloudlane identity/capabilities"); Qt.exit(1); return
            }
            function find(item, name) {
                if (item.objectName === name) return item
                for (const child of item.children) { const result = find(child, name); if (result) return result }
                return null
            }
            const next = find(widget, "cloudlaneNext")
            const previous = find(widget, "cloudlanePrevious")
            if (!next || !next.enabled || !previous || previous.enabled) {
                console.error("Unexpected widget controls"); Qt.exit(1); return
            }
            next.triggered()
            console.log("Cloudlane widget MPRIS dispatch passed")
            running = false
            finish.start()
        }
    }
    Timer { id: finish; interval: 200; onTriggered: Qt.quit() }
}
