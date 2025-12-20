package android.gui;

/**
 * Specifies the requested capture mode for a screenshot operation.
 * This allows clients to indicate their preference for how the screenshot is captured,
 * particularly to request an optimized path if available and its constraints are acceptable.
 */
enum CaptureMode {
    /**
     * Default capture mode.
     * Uses the standard screenshot path, which may involve GPU composition or other
     * optimized paths if possible. This mode typically allows for more flexibility,
     * such as layer exclusion or redaction of secure content.
     */
    None = 0,

    /**
     * Requires the system to use an optimized capture path, such as DPU (Display
     * Processing Unit) readback. This path is designed for minimal performance and
     * power impact, making it suitable for frequent captures.
     *
     * Constraints when using RequireOptimized:
     * - The content is captured exactly as presented on the display.
     * - Layers generally cannot be excluded (e.g., screen decorations like rounded
     *   corners or camera cutouts might be included if they are part of the
     *   final displayed image).
     * - Secure layers: If present, they must be captured. If capturing secure
     *   layers is not permitted by policy or not possible with the optimized path,
     *   the operation will fail.
     * - Protected layers: If present, the screenshot operation will fail as these
     *   cannot be captured by the optimized path.
     *
     * If the optimized path cannot satisfy the request under these constraints
     * (e.g., due to the presence of protected layers, or if the hardware path
     * is unavailable or cannot fulfill the request for other reasons), the
     * screenshot operation will return an error. It will NOT automatically
     * fall back to the 'None' behavior.
     */
    RequireOptimized = 1
}
