#include "inc.h"

int identifier = 0x1234;
endpoint_t who_e;
int call_type;
endpoint_t SELF_E;

int debug = 0;

static struct {
	int type;
	int (*func)(message *m);
	int reply;
} call_types[] = {
	{ CS_LOCK , do_prelock, 0},
	{ CS_UNLOCK, do_unlock, 0},
	{ CS_WAIT, do_wait, 0},
	{ CS_BROADCAST, do_broadcast, 0},
	{ CS_TERM, do_term, 0},
	{ CS_SIG, do_sig, 0},
};

struct mutexes{
	int taken;
	int number;
	int user;
} mutexes;
struct mutexes mut[MAX_MUTEXES];
queue *mut_queue[MAX_MUTEXES];

struct cond{
	int taken;
	int id;
	int size;
	int proc[MAX_MUTEXES];
	int mut[MAX_MUTEXES];
} cond;
struct cond condi[MAX_MUTEXES];

static void sef_local_startup(void);
static int sef_cb_init_fresh(int type, sef_init_info_t *info);
static void sef_cb_signal_handler(int signo);



int main(int argc, char *argv[])
{
	message m;
	
	/* SEF local startup. */
	env_setargs(argc, argv);
	sef_local_startup();

	while (TRUE) {
		int r;
		if ((r = sef_receive(ANY, &m)) != OK)
		{	
			printf("sef_receive failed %d.\n", r);
			continue;
		}

		who_e = m.m_source;
		call_type = m.m_type;
                int result;
		 
		if (call_type >= 0 && call_type <= 6) {
			int type = call_type - 1;
			result = call_types[type].func(&m);
		} else {
			// warn and then ignore 
			printf("MCV unknown call type: %d from %d.\n",
				call_type, who_e);
		}
		if (debug == 1)
			printf("sending reply? r:%d u:%d type:%d m:%d\n", result, who_e, call_type, m.m1_i1);
		if (result != EDONTREPLY)
			do_reply(who_e, result);
		

	}

	/* no way to get here */
	return -1;
}

/*===========================================================================*
 *			       sef_local_startup			     *
 *===========================================================================*/
static void sef_local_startup()
{
  /* Register init callbacks. */
  sef_setcb_init_fresh(sef_cb_init_fresh);
  sef_setcb_init_restart(sef_cb_init_fresh);

  /* No live update support for now. */

  /* Register signal callbacks. */
  sef_setcb_signal_handler(sef_cb_signal_handler);

  /* Let SEF perform startup. */
  sef_startup();
}

/*===========================================================================*
 *		            sef_cb_init_fresh                                *
 *===========================================================================*/
static int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *UNUSED(info))
{

  return(OK);
}

/*===========================================================================*
 *		            sef_cb_signal_handler                            *
 *===========================================================================*/
static void sef_cb_signal_handler(int signo)
{

}
/*===========================================================================*
 *                               do_prelock                                  *
 *===========================================================================*/
 int do_prelock(message *m)
 {
	int r = do_lock(m->m1_i1, m->m_source);
	return r;
 }
/*===========================================================================*
 *                                do_lock                                    *
 *===========================================================================*/
int do_lock (int mutex_number, int mutex_user)
{
	if (debug == 1)
		printf("doin server lock m:%d u:%d\n", mutex_number, mutex_user);
	int in_use = 0;
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		//szuka czy mut jest juz zarezerwowany
		if (mut[i].taken == 1 && mut[i].number == mutex_number)
		{
			//jest w posiadaniu mutexu juz
			if (mut[i].user == mutex_user)
			{	
				return -1;
			}
			//jesli jest zarezerwowany ale bez kolejki
			if (mut_queue[i] == NULL)
			{	
				mut_queue[i] = make_queue(mutex_number);
			}		
			enqueue(mut_queue[i], mutex_user);
			in_use = 1;
			if (debug == 1)
			printf("dodano do kolejki m:%d u:%d\n", mutex_number, mutex_user); 
			return EDONTREPLY;
		}
	}
	//jesli nikt go jeszcze nie rezerwuje
	if (in_use == 0)
	{
		for (int i=0; i<MAX_MUTEXES; i++)
		{
			if (mut[i].taken == 0)
			{
				mut[i].taken = 1;
				mut[i].number = mutex_number;
				mut[i].user = mutex_user;
				return OK;
			}
		}
	}
	return OK;
}

/*===========================================================================*
 *                              do_unlock                                    *
 *===========================================================================*/
 int do_unlock(message *m)
 {
	int mutex_number =m->m1_i1;
	int mutex_user = m->m_source;
	if (debug == 1)
		printf ("doin unlock m:%d u:%d\n", mutex_number, mutex_user);
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if (mut[i].taken == 1 && mut[i].number == mutex_number) 
		{
			if (mut[i].user != mutex_user) //jesli nie jest wlascicielem
			{
				return EPERM;
			}
			if (mut_queue[i] == NULL) //jesli nikt nie czeko to zwolnij
			{
				mut[i].taken = 0;
				
			}
			else
			{
				int temp = dequeue(mut_queue[i]);
				mut[i].user = temp;
				if (is_empty(mut_queue[i]) == 1)
				{
					remove_queue(mut_queue[i]);
					mut_queue[i] = NULL;
				}
				do_reply(temp, OK);
			}
			break;
		}
		//nie byl zablokowany
		if (i == MAX_MUTEXES - 1)
		{
			return EPERM;
		}
	}
	return OK;
 }

/*===========================================================================*
 *                              do_wait                                      *
 *===========================================================================*/
 int do_wait(message *m)
 {
	int mutex_number = m->m1_i1;
	int cond_var_id = m->m1_i2;
	int mutex_user = m->m_source;
	for (int i=0; i<MAX_MUTEXES; i++) //szukaj mutexu 
	{
		if (mut[i].taken == 1 && mut[i].number == mutex_number)
		{
			if (mut[i].user != mutex_user) //pr nie jest w pos
			{
				return EINVAL;
			}
			
			//zwolnic mutex
			do_unlock(m);		
			
			//zawies proces
			int in_use = 0;
			for (int j=0; j<MAX_MUTEXES; j++)
			{
				if (condi[j].taken == 1 && condi[j].id == cond_var_id)
				{
					in_use = 1;
					condi[j].mut[condi[j].size] = mutex_number;
					condi[j].proc[condi[j].size] = mutex_user;
					condi[j].size++;
					return EDONTREPLY;
				}
			}
			if (in_use == 0) //nikt nie czeka na to cv
			{
				//znajdz wolne miejsce
				for (int j=0; j<MAX_MUTEXES; j++)
				{
					if (condi[j].taken == 0)
					{
						condi[j].taken = 1;
						condi[j].id = cond_var_id;
						condi[j].mut[0] = mutex_number;
						condi[j].proc[0] = mutex_user;
						condi[j].size = 1;
						return EDONTREPLY;
					}
				}
			}

		}
	}
	return OK;
 }

/*===========================================================================*
 *                                do_reply                                   *
 *===========================================================================*/
 void do_reply(int who_e, int mess)
 {
	message reply_message;
	reply_message.m_type = mess;
	if (mess == 0)	
		reply_message.m1_i1 = 0;
	else
		reply_message.m1_i1 = -1; 
	int s = send(who_e, &reply_message);
 }
/*===========================================================================*
 *                            do_broadcast                                   *
 *===========================================================================*/
 int do_broadcast(message *m)
 {
	int cond_var_id = m->m1_i1;
	int mutex_user = m->m_source;
	//znajdz odpowiednie cv
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if(condi[i].taken == 1 && condi[i].id == cond_var_id)
		{
			for (int j=0; j<condi[i].size; j++)
			{
				int d =do_lock(condi[i].mut[j], condi[i].proc[j]);
				if (d == OK)
					do_reply(condi[i].proc[j], OK);
			}
			condi[i].taken = 0;
			condi[i].size = 0;
			break;
		}
	}
	
	return OK;
 }
/*===========================================================================*
 *                               do_term                                     *
 *===========================================================================*/
 int do_term(message *m)
 {
	//return 	EDONTREPLY;
	int proc_number = m->PM_PROC;
	if (debug == 2)
		printf ("do term p:%d\n", proc_number);
	//odblokuj jego mut
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if (mut[i].taken == 1 && mut[i].user == proc_number)
		{
			message mes;
			mes.m_source = proc_number;
			mes.m1_i1 = mut[i].number;
			do_unlock(&mes);
		}
	}

	//usun z kolejki
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if (mut[i].taken == 1 && mut_queue[i] != NULL)
		{
			remove_pr(mut_queue[i], proc_number);
			if (is_empty(mut_queue[i]) == 1)
			{
				remove_queue(mut_queue[i]);
				mut_queue[i] = NULL;
			}
		}
	}

	//usun z oczekiwania na zdarzenie
	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if (condi[i].taken == 1 )
		{
			for (int j=0; j<condi[i].size; j++)
			{
				//usun i przesun tablice
				if (condi[i].proc[j] == proc_number)
				{
					for (int k=j; k<condi[i].size-1; k++)
					{	
						condi[i].proc[k] = condi[i].proc[k+1];
						condi[i].mut[k] = condi[i].mut[k+1];

					}
					condi[i].size--; 
				}
			}
			if (condi[i].size == 0)
				condi[i].taken = 0;

		}
	}


	return EDONTREPLY;

 }
/*===========================================================================*
 *                                  do_sig                                   *
 *===========================================================================*/
 int do_sig(message *m)
 {
	int proc_number = m->PM_PROC;
	if (debug == 2)
		printf ("do sig p:%d\n", proc_number);
	for (int i = 0; i<MAX_MUTEXES; i++)
	{
		if (mut[i].taken == 1 && mut_queue[i] != NULL)
		{
			int x = remove_pr(mut_queue[i], proc_number);
			if (is_empty(mut_queue[i]) == 1)
			{
				remove_queue(mut_queue[i]);
				mut_queue[i] = NULL;
			}
			if (x == 1)
				do_reply(proc_number, EINTR);
}
}

	//printf ("elo\n");

	for (int i=0; i<MAX_MUTEXES; i++)
	{
		if (condi[i].taken == 1)
		{
			for (int j=0; j<condi[i].size; j++)
			{
				if (condi[i].proc[j] == proc_number)
				{
					do_reply(proc_number, EINTR);
					for (int k=j; k<condi[i].size-1; k++)
					{
						condi[i].proc[k] = condi[i].proc[k+1];
						condi[i].mut[k] = condi[i].mut[k+1];
					}
					condi[i].size--;
				}
			}

		}
	}

	return EDONTREPLY;
 }
