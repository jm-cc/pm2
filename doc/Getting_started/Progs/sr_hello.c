#include <nm_so_util.h>
#include <nm_so_sendrecv_interface.h>

const char *msg	= "hello, world!";

int main(int argc, char	**argv) {
  struct nm_so_interface *sr_if = NULL;
  nm_so_request s_request, r_request;
  nm_gate_id_t gate_id;

  uint32_t len = 1 + strlen(msg);
  int rank;
  char *buf = NULL;

  nm_so_init(&argc, argv);
  nm_so_get_rank(&rank); /* Here */
  nm_so_get_sr_if(&sr_if); /* Here */

  buf = malloc(len*sizeof(char));
  memset(buf, 0, len);

  if (rank == 0) {
    nm_so_sr_irecv(sr_if, NM_SO_ANY_SRC, 0, buf, len, &r_request); /* Here */
    nm_so_sr_rwait(sr_if, r_request);

    gate_id = nm_so_get_gate_out_id(1);    /* Here */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &s_request); /* Here */
    nm_so_sr_swait(sr_if, s_request);
  }
  else if (rank == 1) {
    strcpy(buf, msg);

    gate_id = nm_so_get_gate_out_id(0); /* Here */
    nm_so_sr_isend(sr_if, gate_id, 0, buf, len, &s_request);
    nm_so_sr_swait(sr_if, s_request);

    gate_id = nm_so_get_gate_in_id(0); /* Here */
    nm_so_sr_irecv(sr_if, gate_id, 0, buf, len, &r_request);
    nm_so_sr_rwait(sr_if, r_request);

    printf("buffer contents: <%s>\n", buf);
  }

  free(buf);
  nm_so_exit();
  exit(0);
}
