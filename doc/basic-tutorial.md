Using libsqon
=============

This is a short tutorial for using `libsqon`. You can find complete API
documentation [on Delwink's site][1] or by [generating it with Doxygen][2].

The following tutorial provides a basic usage demonstration for this
library. It relies on "fallback" behavior in the library for connectivity with
your database engine; namely, `libsqon` will connect to the database
automatically when querying and then disconnect. This is fine in situations
where only one query is needed, but it is very slow if queries are done in
succession. It would be advisable to call `sqon_connect ()`, do your queries,
then `sqon_close ()`, so that the socket is only opened once. This tutorial
will not be doing that, since it demonstrates a simple, single-query use-case.

The following is a complete program that will print the contents of a
table. Below it, we'll go into detail about each line.

``` c
#include <stdio.h>
#include <sqon.h>

int
main (int argc, char *argv[])
{
  int rc = 0;
  sqon_dbsrv mydb;
  char *output;

  sqon_init ();

  mydb = sqon_new_connection (SQON_DBCONN_MYSQL, "localhost",
			      "myuser", "mypasswd", "mydb");

  rc = sqon_query (&mydb, "SELECT * FROM MyTable", &output, NULL);
  if (rc)
    {
      fprintf (stderr, "Error %d\n", rc);
      return rc;
    }

  printf ("%s\n", output);
  sqon_free (output);

  return 0;
}
```

This example uses a MySQL database, but `libsqon` is designed to be identical
between databases, the only difference being what value is passed as the first
argument to `sqon_new_connection ()`.

Now, let's step through the program.

``` c
#include <stdio.h>
#include <sqon.h>
```

We must include these header files in order to make calls to `libsqon` and give
output to the console.

``` c
int
main (int argc, char *argv[])
{
  ...
}
```

This is the typical declaration of the `main ()` entry point into our
program. Although our program does not take arguments from the command line, it
may need to in the future, and it doesn't hurt to declare the standard `(int,
char *)` parameters.

``` c
int rc = 0;
sqon_dbsrv mydb;
char *output;
```

Here, we declare all the variables we'll need for our program: a place to store
the return value of `int` functions, a `libsqon` database connection object,
and a buffer for the database output.

``` c
sqon_init ();
```

`libsqon` needs to be initialized using this function. Without this, the
library will not be able to properly allocate memory internally.

``` c
mydb = sqon_new_connection (SQON_DBCONN_MYSQL, "localhost",
			    "myuser", "mypasswd", "mydb");
```

Here, we initialize a database connection object. This object will be
referenced when calling the database. Its purpose is to make query function
calls shorter and less error-prone.

The parameters are the type of database we're connecting to, the host of the
database server, the username, the password, and the database name. These
fields are optional based on context, but only the database is optional for
MySQL.

``` c
rc = sqon_query (&mydb, "SELECT * FROM MyTable", &output, NULL);
if (rc)
  {
    fprintf (stderr, "Error %d\n", rc);
    return rc;
  }
```

Finally, we call the database. The parameters are the address of your database
connection object, the SQL query, the output buffer (could be `NULL` if, say,
we were inserting or updating data which does not return anything from the
database), and a string representation of the return value's "primary key". I
use quotes, because this doesn't have to be the actual primary key of the
table. If the primary key value matches a column in the return value, the
output buffer will be organized by that key. Keep in mind that whatever key you
choose must be unique in the return value, or `libsqon` will throw an error.

In this example, we use `NULL` as the primary key, which will simply return
each row of the table as elements of an array, not organized by a particular
key.

The `if` block here catches any error during the query or output formatting
process, prints the error code to the screen, and exits the program with an
error code.

``` c
printf ("%s\n", output);
sqon_free (output);

return 0;
```

We complete our task by printing the return value from the database, freeing
the memory that `libsqon` allocated for the output buffer, and exiting with a
success code.

[1]: http://delwink.com/software/apidocs/libsqon
[2]: generating-api-documentation.md
