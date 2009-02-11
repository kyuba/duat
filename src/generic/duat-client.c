/*
 * This file is part of the kyuba.org Duat project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008, 2009, Kyuba Project Members
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include <curie/main.h>
#include <curie/sexpr.h>
#include <curie/multiplex.h>
#include <duat/9p-client.h>

struct sexpr_io *stdio;

static int print_help ()
{
    return 1;
}

static void on_connect (struct d9r_io *io, void *aux)
{
    cexit (0);
}

static void on_error (struct d9r_io *io, const char *error, void *aux)
{
    sx_write (stdio, make_string (error));

    cexit (3);
}

int cmain()
{
    sexpr i_socket = sx_false;
    stdio = sx_open_stdio();

    multiplex_d9c ();

    for (int i = 0; curie_argv[i] != (char *)0; i++)
    {
        if (curie_argv[i][0] == '-')
        {
            int xn = i + 1;

            for (int y = 1; curie_argv[i][y] != (char)0; y++)
            {
                switch (curie_argv[i][y])
                {
                    case 's':
                        if (curie_argv[xn] != (char *)0)
                        {
                            i_socket = make_string(curie_argv[xn]);
                            xn++;
                        }
                        break;
                    case 'h':
                    case '-':
                        return print_help ();
                }
            }

            i = xn;
        }
    }

    if (!stringp(i_socket)) return 2;

    multiplex_add_d9c_socket
            (sx_string (i_socket), on_connect, on_error, (void *)0);

    while (multiplex() == mx_ok);

    return 0;
}
