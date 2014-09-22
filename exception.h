/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#ifndef __EXCEPTION_H
#define __EXCEPTION_H

/* We disable exceptions, but some compilers don't like seeing try/catch
 * statements with them disabled. Define them to have no effect. */

#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 4
#define try
#define catch(...) if(0)
#endif

#endif /* __EXCEPTION_H */
