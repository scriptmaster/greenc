namespace Scale;
public static class Library06 {
    private static int Increment() => 6;
    public static int Value() => Library05.Value() + Increment();
}
