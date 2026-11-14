namespace Demo.Math;

public static class Arithmetic
{
    public static int Sum(int limit)
    {
        int result = 0;

        for (int value = 1; value <= limit; value = value + 1)
        {
            result = result + value;
        }

        return result;
    }

    public static int Factorial(int value)
    {
        if (value <= 1)
        {
            return 1;
        }

        return value * Factorial(value - 1);
    }
}
