# SimpleCalculator
This is a simple calculator application that runs on the console.

You can build it with GCC 4.9 or above, Clang 3.8 or above, Apple LLVM 8.0 or above. The compiling option -std=gnu11 is necessary.

This application is very easy to use. If the executable name is SimpleCalculator, you can run it with:

SimpleCalculator 1+8%3/2-5

Attention, the arithmetic expression may not contain any spaces or tabs.

In addition, some shell consoles don't treat '(' or ')' as a common token. They may have special meaning. In this situation, you can substitute [ ] for ( ). In the main function, there is a filter to transform [ ] back to ( ).

So, if you want to input 「 SimpleCalculator (1+8)/(2+1) 」, use 「 SimpleCalculator [1+8]/[2+1] 」 instead.

