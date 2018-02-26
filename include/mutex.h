#ifndef _MUTEX_H
#define _MUTEX_H

#include <errno.h>
#include <lib.h>
#include <stdio.h>
#include <minix/endpoint.h>

#define OK	0

#define CS_LOCK		1
#define CS_UNLOCK	2
#define CS_WAIT		3
#define CS_BROADCAST	4

static int get_mcv_endpt(endpoint_t *pt);

int cs_lock(int mutex_id);
int cs_unlock(int mutex_id);
int cs_wait(int cond_var_id, int mutex_id);
int cs_broadcast(int cond_var_id);

#endif
