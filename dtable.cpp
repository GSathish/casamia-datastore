/* This file is part of the Casa Mia Datastore Project at UBC.It is distributed under the terms of
 * version 2 of the GNU GPL. See the file LICENSE for details. */

#include "dtable.h"

/* that's it, that's all this file is here for */
atomic<abortable_tx> dtable::atx_handle(NO_ABORTABLE_TX);
