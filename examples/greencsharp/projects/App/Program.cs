using System;
using Demo.Math;

namespace Demo.App;

internal class Program
{
    public static int Main()
    {
        Console.WriteLine("Project references resolved by greencsharp");
        int total = Arithmetic.Sum(6);
        Console.WriteLine(total);
        Console.WriteLine(Arithmetic.Factorial(5));

        if (total != 21)
        {
            return 1;
        }

        return 0;
    }
}
