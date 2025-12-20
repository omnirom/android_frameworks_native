package android.gui;

/**
 * Defines how protected layers are handled during screen capture.
 */
enum ProtectedLayerMode {
    /** When a protected layer is encountered, redact its content by blacking out the layer. */
    Redact = 0,
    /** When a protected layer is encountered, capture its content. */
    Capture = 1,
    /** When a protected layer is encountered, throw an error. */
    Error = 2
}
