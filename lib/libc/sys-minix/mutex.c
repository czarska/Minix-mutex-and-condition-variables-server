#include <mutex.h>
int debug = 0;

static int get_mcv_endpt(endpoint_t *pt)
{
	return minix_rs_lookup("mcv", pt);
}

int cs_lock(int mutex_id) 
{
	if (debug == 1)
	    printf("doing lock %d\n", mutex_id);
	message m;
	endpoint_t mcv_pt;
	int r;

	if (get_mcv_endpt(&mcv_pt) != OK) {
		errno = ENOSYS;
		return -1;
	}
	while (1)
	{
		m.m1_i1 = mutex_id;
		r = _syscall(mcv_pt, CS_LOCK, &m);
		if (r == 0) break;
		if (errno == EINTR)
		{
			m.m1_i1 = mutex_id;
			continue;
		}
	}
	return m.m1_i1;

}
int cs_unlock(int mutex_id)
{
	if (debug == 1)
		printf("doing unlock %d\n", mutex_id);
	message m;
	endpoint_t mcv_pt;
	int r;

	if (get_mcv_endpt(&mcv_pt) != OK) {
		errno = ENOSYS;
		return -1;
	}

	m.m1_i1 = mutex_id;
	r = _syscall(mcv_pt, CS_UNLOCK, &m);
	if (debug == 1)
		printf("after unlock %d with %d\n", mutex_id, r);
	return m.m1_i1;
}
int cs_wait(int cond_var_id, int mutex_id)
{
	if (debug == 1)
		printf("doing wait c:%d m:%d\n", cond_var_id, mutex_id);
	message m;
	endpoint_t mcv_pt;
	int r;

	if (get_mcv_endpt(&mcv_pt) != OK) {
		errno = EINTR;
		return -1;
	}
	m.m1_i1 = mutex_id;
	m.m1_i2 = cond_var_id;
	r = _syscall(mcv_pt, CS_WAIT, &m);
	if (r == -1)
	{
		if (errno == EINTR)
		{	
			int ll = cs_lock(mutex_id);
			return ll;
		}
		else
			return -1;
	}
	return m.m1_i1;

}
int cs_broadcast(int cond_var_id)
{
	if (debug == 1)
		printf("doin brodcast c:%d\n", cond_var_id);
	message m;
	endpoint_t mcv_pt;
	int r;

	if (get_mcv_endpt(&mcv_pt) != OK) {
		errno = EINTR;
		return -1;
	}
	m.m1_i1 = cond_var_id;
	r = _syscall(mcv_pt, CS_BROADCAST, &m);
	return m.m1_i1;
}

