/*
** Copyright 2002-2006 KVASER AB, Sweden.  All rights reserved.
*/

/* Kvaser Linux Canlib */

#include "canlib_data.h"

extern CANOps vCanOps;

HandleData * findHandle (CanHandle hnd);
HandleData * removeHandle (CanHandle hnd);
CanHandle insertHandle (HandleData *hData);
