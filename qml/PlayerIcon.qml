import QtQuick
import QtQuick.Shapes

Item {
    id: icon
    property string name: "play"
    property color color: Backend.theme.foreground
    implicitWidth: 24
    implicitHeight: 24
    readonly property bool solid: ["play", "pause", "previous", "next", "heart-filled"].indexOf(name) >= 0
    readonly property var paths: ({
        "play": "M8 5 L19 12 L8 19 Z",
        "home": "M3 11 L12 3 L21 11 M5 9 V21 H10 V15 H14 V21 H19 V9",
        "search": "M16 16 L22 22 M18 10 A8 8 0 1 1 2 10 A8 8 0 1 1 18 10",
        "back": "M15 5 L8 12 L15 19",
        "library": "M4 4 V20 M9 4 V20 M14 5 L18 4 L22 19 L18 20 Z",
        "discover": "M21 12 A9 9 0 1 1 3 12 A9 9 0 1 1 21 12 M15 8 L13 13 L8 16 L10 11 Z",
        "activity": "M4 5 H20 V17 H10 L5 21 V17 H4 Z M8 9 H16 M8 13 H13",
        "download": "M12 3 V15 M7 10 L12 15 L17 10 M4 17 V21 H20 V17",
        "settings": "M4 6 H20 M4 12 H20 M4 18 H20 M8 3 V9 M16 9 V15 M10 15 V21",
        "now-playing": "M3 4 H21 V20 H3 Z M8 8 L15 12 L8 16 Z",
        "shuffle": "M3 6 H5 C11 6 13 18 19 18 H21 M18 15 L21 18 L18 21 M3 18 H5 C8 18 9 15 10 13 M14 8 C16 6 17 6 19 6 H21 M18 3 L21 6 L18 9",
        "repeat": "M18 3 L21 6 L18 9 M21 6 H7 C2 6 2 10 2 12 M6 21 L3 18 L6 15 M3 18 H17 C22 18 22 14 22 12",
        "repeat-one": "M18 3 L21 6 L18 9 M21 6 H7 C2 6 2 10 2 12 M6 21 L3 18 L6 15 M3 18 H17 C22 18 22 14 22 12 M10 10 L12 9 V15 M10 15 H14",
        "pause": "M6 5 H10 V19 H6 Z M14 5 H18 V19 H14 Z",
        "previous": "M5 5 H8 V19 H5 Z M19 5 V19 L9 12 Z",
        "next": "M16 5 H19 V19 H16 Z M5 5 L15 12 L5 19 Z",
        "volume": "M3 9 H7 L12 5 V19 L7 15 H3 Z M16 8 C18 10 18 14 16 16 M19 5 C23 9 23 15 19 19",
        "mute": "M3 9 H7 L12 5 V19 L7 15 H3 Z M16 9 L22 15 M22 9 L16 15",
        "queue": "M4 6 H20 M4 12 H14 M4 18 H12 M18 11 L23 15 L18 19 Z",
        "lyrics": "M5 4 H19 V20 H5 Z M8 8 H16 M8 12 H16 M8 16 H13",
        "more": "M5 11 H6 V12 H5 Z M11 11 H12 V12 H11 Z M17 11 H18 V12 H17 Z",
        "music": "M9 18 V6 L20 4 V16 M9 9 L20 7 M9 18 C9 22 2 22 2 18 C2 15 9 15 9 18 M20 16 C20 20 13 20 13 16 C13 13 20 13 20 16",
        "heart": "M12 20 L4 12 C-2 5 7 0 12 7 C17 0 26 5 20 12 Z",
        "heart-filled": "M12 20 L4 12 C-2 5 7 0 12 7 C17 0 26 5 20 12 Z"
    })
    Shape {
        preferredRendererType: Shape.CurveRenderer
        anchors.centerIn: parent
        width: 24; height: 24
        scale: Math.min(icon.width, icon.height) / 24
        ShapePath {
            fillColor: icon.solid ? icon.color : "transparent"
            strokeColor: icon.solid ? "transparent" : icon.color
            strokeWidth: 1.7
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: icon.paths[icon.name] || "" }
        }
    }
}
