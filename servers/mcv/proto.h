/* FUNCTION PROTOTYPES */

/* main.c */
int main(int argc, char **argv);

/* functions */
int do_prelock(message *m);
int do_lock(int mutex_number, int mutex_user);
int do_unlock(message *m);
int do_wait(message *m);
int do_broadcast(message *m);
int do_term(message *m);
int do_sig(message *m);
void do_reply(int who_e, int message);
