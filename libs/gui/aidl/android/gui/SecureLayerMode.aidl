package android.gui;

/**
 * Defines how secure layers are handled during screen capture.
 */
enum SecureLayerMode {
    /** When a secure layer is encountered, redact its content, by blacking out the layer. */
    Redact = 0,
    /** When a secure layer is encountered, attempt to capture its content. */
    Capture = 1,
    /** When a secure layer is encountered, return an error. */
    Error = 2
}
