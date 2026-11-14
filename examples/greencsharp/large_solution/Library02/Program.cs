namespace Scale;
public static class Library02 {
    private static int Increment() => 2;
    public static int Value() => Library01.Value() + Increment();
}
