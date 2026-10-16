package orphen.bridge;

public final class TickBridge {
    private TickBridge() {}
    private static native void nativeTick();
    public static void tick() {
        // An old Java frame may finish after native callbacks have been removed.
        try { nativeTick(); } catch (UnsatisfiedLinkError unloaded) { }
    }
}
