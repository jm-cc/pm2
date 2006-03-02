#include "pm2_common.h"

void do_main_code(p_mad_madeleine_t madeleine);

int main(int argc, char *argv[]) {
  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);

  do_main_code(mad_get_madeleine());
  common_exit(NULL);
  exit(EXIT_SUCCESS);
}

void do_main_code(p_mad_madeleine_t madeleine) {
  p_mad_channel_t channel = NULL;
  ntbx_process_lrank_t my_local_rank = -1;

  channel = tbx_htable_get(madeleine->channel_htable, "pm2");
  my_local_rank = ntbx_pc_global_to_local(channel->pc,
				 madeleine->session->process_rank);

  if (my_local_rank == 0) {
    char hostname[100];
    char data[50] = "";
    p_mad_connection_t out = NULL;
    int len = 0;

    strcpy(data, "Hello World from ");   /* Prepare the data to be sent */
    gethostname(hostname, 100);
    strcat(data, hostname);
    len = strlen(data);

    out = mad_begin_packing(channel, 1); /* Open the out connection */
    mad_pack(out, &len, sizeof(len), mad_send_SAFER, mad_receive_EXPRESS);
    mad_pack(out, data, len, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_packing(out);                /* End of the sent */
  }
  else if (my_local_rank == 1) {
    p_mad_connection_t in = NULL;
    int len = 0;
    char *data = NULL;

    in = mad_begin_unpacking(channel);   /* Starts the reception */
    mad_unpack(in, &len, sizeof(len), mad_send_SAFER, mad_receive_EXPRESS);
    data = TBX_CALLOC(1, len);    
    mad_unpack(in, data, len, mad_send_CHEAPER, mad_receive_CHEAPER);
    mad_end_unpacking(in);               /* Ends the reception. */

    printf("%s\n", data);                /* Print the string */
  }
}
