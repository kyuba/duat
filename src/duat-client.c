/**\file
 * \brief Duat generic 9P2000 CLI client
 *
 * Implements the 'd9c' programme, which can connect to 9P servers and query
 * information from them.
 *
 * This programme is heavily influenced by the ixpc(1) 9P client programme.
 *
 * \copyright
 * Copyright (c) 2008-2013, Kyuba Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/duat
 * \see Project Source Code: http://git.becquerel.org/kyuba/duat.git
 */

#include <curie/main.h>
#include <curie/sexpr.h>
#include <curie/multiplex.h>
#include <curie/memory.h>
#include <sievert/shell.h>
#include <duat/9p-client.h>

/**\brief Programme operation mode
 *
 * Which operation to perform on a given path. Set after parsing the command
 * line arguments to match the selection there.
 */
enum op
{
    op_nop,   /**< Do nothing */
    op_cat,   /**< Print the named resource's contents */
    op_ls,    /**< List directory contents (names only) */
    op_lsd,   /**< List directory contents in greater detail */
    op_write, /**< Write to a file; uses the data you enter on stdin */
    op_create /**< Create a file with the contents of stdin */
};

static char *i_path           = (char *)0;
static char *i_file           = (char *)0;
static enum op i_op           = op_nop;
static struct io *stdout      = (struct io *)0;
static struct io *stdin       = (struct io *)0;
static struct sexpr_io *stdio = (struct sexpr_io *)0;
static struct d9r_io *d9io    = (struct d9r_io *)0;

/**\brief Defines sexpr symbol "sym_directory"
 *
 * Defines sym_directory, a symbol that is used with the "lsd" option to
 * distinguish between directories and files.
 */
define_symbol (sym_directory, "directory");

/**\brief Defines sexpr symbol "sym_file"
 *
 * Defines sym_file, a symbol that is used with the "lsd" option to distinguish
 * between directories and files.
 */
define_symbol (sym_file,      "file");

/**\brief Usage summary
 *
 * Echoed to stdout in the print_help() function when no valid operation has
 * been selected.
 */
#define help "d9c -s <address> (read|write|create|ls|lsd) <path>\n"

static int print_help ()
{
    struct io *out = io_open (1);
    if (out != (struct io *)0)
    {
        io_write (out, help, (sizeof (help) -1));
    }

    return 1;
}

static void on_read_ls (struct io *io, void *aux)
{
    struct d9r_qid qid;
    int_16 type;
    int_32 dev, mode, atime, mtime, rp;
    int_64 length;
    int rd;
    char *name, *uid, *gid, *muid, *ex;

    while (((rd = (io->length - io->position)) > 0) &&
           ((rp = d9r_parse_stat_buffer
                      ((struct d9r_io *)aux, rd,
                       (int_8 *)io->buffer + io->position, &type,
                       &dev, &qid, &mode, &atime, &mtime, &length,
                       &name, &uid, &gid, &muid, &ex)) > 0))
    {
        sexpr l = sx_end_of_list, o;

        if (i_op == op_lsd)
        {
            l = cons (make_integer (length), cons (make_integer (mode),
                      cons (make_integer (atime), cons (make_integer (mtime),
                            cons (make_string (uid), cons (make_string (gid),
                                  cons (make_string (muid),
                                        sx_end_of_list)))))));
        }

        o = cons (((qid.type & QTDIR) ? sym_directory : sym_file),
                  cons (make_string(name), l));

        sx_write (stdio, o);

        io->position += rp;
    }
}

static void on_read (struct io *io, void *aux)
{
    io_write (stdout, io->buffer + io->position, io->length - io->position);
    io->position = io->length;
}

static void on_read_stdin (struct io *io, void *aux)
{
    struct io *o = (struct io *)aux;

    io_write (o, io->buffer + io->position, io->length - io->position);
    io->position = io->length;
}

static void on_close_stdin (struct io *io, void *aux)
{
    multiplex_del_io  (stdout);
    multiplex_del_d9r (d9io);
}

static void on_close (struct io *io, void *aux)
{
    multiplex_del_io  (stdout);
//    multiplex_del_d9r (d9io);
    cexit(0);
}

static void on_connect (struct d9r_io *io, void *aux)
{
    struct io *n;

    d9io = io;

    switch (i_op)
    {
        case op_cat:
            n = io_open_read_9p  (io, i_path);
            multiplex_add_io (n, on_read, on_close, (void *)io);
            break;
        case op_ls:
        case op_lsd:
            n = io_open_read_9p  (io, i_path);
            multiplex_add_io (n, on_read_ls, on_close, (void *)io);
            break;
        case op_write:
            n = io_open_write_9p (io, i_path);
            multiplex_add_io (stdin, on_read_stdin, on_close_stdin, (void *)n);
            break;
        case op_create:
            n = io_open_create_9p (io, i_path, i_file, 0666);
            multiplex_add_io (stdin, on_read_stdin, on_close_stdin, (void *)n);
            break;
        default:
            cexit (4);
    }
}

static void on_error (struct d9r_io *io, const char *error, void *aux)
{
    sx_write (stdio, make_string (error));

    cexit (3);
}

static void on_d9_close (struct d9r_io *io, void *aux)
{
    cexit (5);
}

int cmain()
{
    char *i_socket = (char *)0;

    stdin          = io_open (0);
    stdin->type    = iot_read;
    stdout         = io_open (1);
    stdout->type   = iot_write;

    stdio          = sx_open_io (stdin, stdout);

    multiplex_io  ();
    multiplex_d9c ();

    for (int i = 1; curie_argv[i] != (char *)0; i++)
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
                            i_socket = curie_argv[xn];
                            xn++;
                        }
                        break;
                    case 'h':
                    case '-':
                        return print_help ();
                }
            }

            i = xn - 1;
        }
        else if (i_op == op_nop)
        {
            char *op = curie_argv[i];

            switch (op[0])
            {
                case 'l':
                    if (op[1] == 's')
                    {
                        if (op[2] == 0)
                        {
                            i_op = op_ls;
                            break;
                        }
                        else if ((op[2] == 'd') && (op[3] == 0))
                        {
                            i_op = op_lsd;
                            break;
                        }
                    }
                    return 10;
                case 'c':
                    if ((op[1] == 'a') && (op[2] == 't') && (op[3] == 0))
                    {
                        i_op = op_cat;
                        break;
                    }
                    else
                    if ((op[1] == 'r') && (op[2] == 'e') && (op[3] == 'a') &&
                        (op[4] == 't') && (op[5] == 'e') && (op[6] == 0))
                    {
                        i_op = op_create;
                        break;
                    }
                    return 11;
                case 'r':
                    if ((op[1] == 'e') && (op[2] == 'a') && (op[3] == 'd') &&
                         (op[4] == 0))
                    {
                        i_op = op_cat;
                        break;
                    }
                    return 12;
                case 'w':
                    if ((op[1] == 'r') && (op[2] == 'i') && (op[3] == 't') &&
                        (op[4] == 'e') && (op[5] == 0))
                    {
                        i_op = op_write;
                        break;
                    }
                    return 13;
                default:
                    return 15;
            }
        }
        else if (i_path == (char *)0)
        {
            i_path = curie_argv[i];
        }
        else if ((i_op == op_create) && (i_file == (char *)0))
        {
            i_file = curie_argv[i];
        }
    }

    if ((i_socket == (char *)0) || (i_op == op_nop) || (i_path == (char *)0))
    {
        return print_help ();
    }

    multiplex_add_d9c_socket
            (i_socket, on_connect, on_error, on_d9_close, (void *)0);
    multiplex_add_io_no_callback (stdout);

    while (multiplex() == mx_ok);

    return 0;
}
