//
// Created by marek on 12/3/2025.
//

#ifndef EZLANG_STDIO_H
#define EZLANG_STDIO_H

class Interpreter;

/**
 * Registers standard input/output native words to the provided interpreter.
 *
 * This method defines native words for reading standard input (e.g., lines,
 * integers, and floating-point numbers) and writing output (e.g., plain strings
 * or lines).
 *
 * @param interpreter The interpreter to which the standard I/O functionality will be registered.
 */
void registerStdIo(Interpreter& interpreter);


#endif //EZLANG_STDIO_H