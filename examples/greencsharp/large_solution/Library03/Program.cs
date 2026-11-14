namespace Scale;
public static class Library03 {
    private static int Increment() => 3;
    public static int Value() => Library02.Value() + Increment();
}
