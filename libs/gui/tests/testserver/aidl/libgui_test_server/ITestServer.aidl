package libgui_test_server;

import android.view.Surface;

// Test server for libgui_test
interface ITestServer {
    // Create a new producer. The server will have connected to the consumer.
    Surface createProducer();

    // Kills the server immediately.
    void killNow();
}
