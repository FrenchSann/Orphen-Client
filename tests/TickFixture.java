public final class TickFixture {
    public static int ticks;
    public static int frames;
    public void runTick() { ticks++; }
    public void render() { frames++; }
    public static void workerTick(final TickFixture game) throws InterruptedException {
        Thread worker = new Thread(new Runnable() {
            @Override public void run() { game.runTick(); }
        });
        worker.start();
        worker.join();
    }
    public static final class WithoutTick {
        public void render() { }
    }
}
