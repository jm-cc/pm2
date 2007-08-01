#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>
#include <math.h>
#include <tbx.h>
#include "nm_private.h"

static const int param_nb_samples        = 2;
static const int param_min_size          = 2;
static const int param_max_size          = 1024*1024*1;

static void
nm_ns_print_errno(const char *msg,
                  int err){
    err = (err > 0 ? err : -err);

    switch(err){
    case NM_ESUCCESS :
        DISP("%s - Successful operation", msg);
        break;

    case NM_EUNKNOWN :
	DISP("%s - Unknown error", msg);
        break;

    case NM_ENOTIMPL :
	DISP("%s - Not implemented", msg);
        break;

    case NM_ESCFAILD :
        DISP("%s - Syscall failed, see errno", msg);
        break;

    case NM_EAGAIN :
        DISP("%s - Poll again", msg);
        break;

    case NM_ECLOSED :
        DISP("%s - Connection closed", msg);
        break;

    case NM_EBROKEN :
        DISP("%s - Error condition on connection", msg);
        break;

    case NM_EINVAL :
        DISP("%s - Invalid parameter", msg);
        break;

    case NM_ENOTFOUND :
        DISP("%s - Not found", msg);
        break;

    case NM_ENOMEM :
        DISP("%s - Out of memory", msg);
        break;

    case NM_EALREADY :
        DISP("%s - Already in progress or done", msg);
        break;

    case NM_ETIMEDOUT :
	DISP("%s - Operation timeout", msg);
        break;

        case NM_EINPROGRESS :
            DISP("%s - Operation in progress", msg);
            break;

    case NM_EUNREACH :
	DISP("%s - Destination unreachable", msg);
        break;

    case NM_ECANCELED :
        DISP("%s - Operation canceled", msg);
        break;

    case NM_EABORTED :
        DISP("%s - Operation aborted", msg);
        break;

    default:
        TBX_FAILURE("Sortie d'erreur incorrecte");
    }
}






static int
nm_ns_initialize_pw(struct nm_core *p_core,
                    struct nm_drv  *p_drv,
                    gate_id_t gate_id,
                    unsigned char *ptr,
                    struct nm_pkt_wrap **pp_pw){
    struct nm_pkt_wrap *p_pw = NULL;
    int err;

    err = nm_pkt_wrap_alloc(p_core, &p_pw, 0, 0);
    if(err != NM_ESUCCESS){
        nm_ns_print_errno("nm_ns_initialize_pw - nm_pkt_wrap_alloc",
                          err);
        goto out;
    }


    err = nm_iov_alloc(p_core, p_pw, 1);
    if (err != NM_ESUCCESS) {
        nm_ns_print_errno("nm_ns_initialize_pw - nm_iov_alloc",
                          err);
        nm_pkt_wrap_free(p_core, p_pw);
        goto out;
    }

    err = nm_iov_append_buf(p_core, p_pw, ptr, param_min_size);
    if (err != NM_ESUCCESS) {
        nm_ns_print_errno("nm_ns_initialize_pw - nm_iov_append_buf",
                          err);
        nm_iov_free(p_core, p_pw);
        nm_pkt_wrap_free(p_core, p_pw);
        goto out;
    }

    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    p_pw->p_gate = p_gate;
    p_pw->p_drv  = p_drv;
    p_pw->p_trk  = p_drv->p_track_array[0];

    *pp_pw	= p_pw;

 out:
    return err;
}

static void
nm_ns_fill_pw_data(struct nm_pkt_wrap *p_pw){
    unsigned char * data = p_pw->v[0].iov_base;
    unsigned char c = 0;
    int i = 0, n = 0;

    for (i = 0; i < param_max_size; i++){
        n += 7;
        c = (unsigned char)(n % 256);
        data[i] = c;
    }
}

static void
nm_ns_update_pw(struct nm_pkt_wrap *p_pw, unsigned char *ptr, int len){
    p_pw->v[0].iov_base	= ptr;
    p_pw->v[0].iov_len	= len;

    p_pw->drv_priv   = NULL;
    p_pw->gate_priv  = NULL;
}

static void
nm_ns_free(struct nm_core *p_core, struct nm_pkt_wrap *p_pw){

    /* free iovec */
    nm_iov_free(p_core, p_pw);

    /* free pkt_wrap */
    nm_pkt_wrap_free(p_core, p_pw);
}

#if 0
static void
control_buffer(struct nm_pkt_wrap *p_pw, int len) {
    unsigned char *main_buffer = p_pw->v[0].iov_base;
    int ok = 1;
    unsigned int n  = 0;
    int i  = 0;

    for (i = 0; i < len; i++) {
        unsigned char c = 0;

        n += 7;
        c = (unsigned char)(n % 256);

        if (main_buffer[i] != c) {
            int v1 = 0;
            int v2 = 0;

            v1 = c;
            v2 = main_buffer[i];
            printf("Bad data at byte %X: expected %X, received %X\n", i, v1, v2);
            ok = 0;
        }
    }

    if (!ok) {
        printf("%d bytes reception failed", len);
    } else {
        printf("ok");
    }
}
#endif

static int
nm_ns_ping(struct nm_drv *driver, gate_id_t gate_id){
    struct nm_drv_ops drv_ops = driver->ops;
    struct nm_pkt_wrap * sending_pw = NULL;
    struct nm_pkt_wrap * receiving_pw = NULL;
    unsigned char *data_send	= NULL;
    unsigned char *data_recv 	= NULL;
    int nb_samples = 0;
    int size = 0;
    int err;
    int i = 0;

    double *ns_lat = driver->cap.network_sampling_latency;

    data_send = TBX_MALLOC(param_max_size);
    err = nm_ns_initialize_pw(driver->p_core, driver, gate_id, data_send,
                              &sending_pw);
    if(err != NM_ESUCCESS){
        nm_ns_print_errno("nm_ns_ping - nm_ns_initialize_sending_pw",
                          err);
        goto out;
    }

    nm_ns_fill_pw_data(sending_pw);

    data_recv = TBX_MALLOC(param_max_size);
    err = nm_ns_initialize_pw(driver->p_core, driver, gate_id, data_recv,
                              &receiving_pw);
    if(err != NM_ESUCCESS){
        nm_ns_print_errno("nm_ns_ping - nm_ns_initialize_receiving_pw",
                          err);
        goto out;
    }


    double sum = 0.0;
    tbx_tick_t t1, t2;

    for (i = 0, size = param_min_size;
         size <= param_max_size;
         i++, size *= 2) {

        nm_ns_update_pw(sending_pw, data_send, size);
        nm_ns_update_pw(receiving_pw, data_recv, size);

        TBX_GET_TICK(t1);
        while (nb_samples++ < param_nb_samples) {

            err = drv_ops.post_send_iov(sending_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_ping - post_send_iov",
                                  err);
                goto out;
            }

            while(err != NM_ESUCCESS){
                err = drv_ops.poll_send_iov(sending_pw);

                if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                    nm_ns_print_errno("nm_ns_ping - poll_send_iov",
                                      err);

                    goto out;
                }
            }
            nm_ns_update_pw(sending_pw,  data_send, size);


            err = drv_ops.post_recv_iov(receiving_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_ping - post_recv_iov",
                                  err);
                goto out;
            }

            while(err != NM_ESUCCESS){
                err = drv_ops.poll_recv_iov(receiving_pw);
                if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                    nm_ns_print_errno("nm_ns_ping - poll_recv_iov",
                                      err);
                    goto out;
                }
            }
            nm_ns_update_pw(receiving_pw, data_recv, size);
            //control_buffer(receiving_pw, size);

        }
        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);

        ns_lat[i] = sum / 2 / param_nb_samples;
        //DISP("(size = %d) ns_lat[%d] = %lf\n",
        //     size, i,  ns_lat[i]);

        nb_samples = 0;
        sum = 0.0;
    }
    nm_ns_free(driver->p_core, sending_pw);
    nm_ns_free(driver->p_core, receiving_pw);

    err = NM_ESUCCESS;

 out:
    return err;
}

static int
nm_ns_pong(struct nm_drv *driver, gate_id_t gate_id){
    struct nm_drv_ops drv_ops = driver->ops;
    struct nm_pkt_wrap * sending_pw = NULL;
    struct nm_pkt_wrap * receiving_pw = NULL;
    unsigned char *data_send          = NULL;
    unsigned char *data_recv          = NULL;
    int nb_samples = 0;
    int size = 0;
    int err;
    int i = 0;

    double *ns_lat = driver->cap.network_sampling_latency;

    data_send = TBX_MALLOC(param_max_size);
    err = nm_ns_initialize_pw(driver->p_core, driver, gate_id, data_send,
                              &sending_pw);
    if(err != NM_ESUCCESS){
        nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_sending_pw",
                          err);
        goto out;
    }

    nm_ns_fill_pw_data(sending_pw);

    data_recv = TBX_MALLOC(param_max_size);
    err = nm_ns_initialize_pw(driver->p_core, driver, gate_id, data_recv,
                              &receiving_pw);
    if(err != NM_ESUCCESS){
        nm_ns_print_errno("nm_ns_pong - nm_ns_initialize_receiving_pw",
                          err);
        goto out;
    }

    double sum = 0.0;
    tbx_tick_t t1, t2;

    for (i = 0, size = param_min_size;
         size <= param_max_size;
         i++, size *= 2) {

        nm_ns_update_pw(sending_pw,    data_send, size);
        nm_ns_update_pw(receiving_pw, data_recv, size);

        TBX_GET_TICK(t1);

        err = drv_ops.post_recv_iov(receiving_pw);
        if(err != NM_ESUCCESS && err != -NM_EAGAIN){
            nm_ns_print_errno("nm_ns_pong - post_recv_iov",
                              err);
            goto out;
        }
        while(err != NM_ESUCCESS){
            err = drv_ops.poll_recv_iov(receiving_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_pong - poll_recv_iov",
                                  err);
            }
        }
        nm_ns_update_pw(receiving_pw, data_recv, size);
        //control_buffer(receiving_pw, size);

        while (nb_samples++ < param_nb_samples - 1) {
            err = drv_ops.post_send_iov(sending_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_pong - post_send_iov",
                                  err);
                goto out;
            }
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_send_iov(sending_pw);
                if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                    nm_ns_print_errno("nm_ns_pong - poll_send_iov",
                                      err);
                    goto out;
                }
            }
            nm_ns_update_pw(sending_pw,  data_send, size);

            err = drv_ops.post_recv_iov(receiving_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_pong - post_recv_iov",
                                  err);
                goto out;
            }
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_recv_iov(receiving_pw);
                if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                    nm_ns_print_errno("nm_ns_pong - poll_recv_iov",
                                      err);
                    goto out;
                }
            }
            nm_ns_update_pw(receiving_pw, data_recv, size);
            //control_buffer(receiving_pw, size);
        }

        err = drv_ops.post_send_iov(sending_pw);
        if(err != NM_ESUCCESS && err != -NM_EAGAIN){
            nm_ns_print_errno("nm_ns_pong - post_send_iov",
                              err);
            goto out;
        }
        while(err != NM_ESUCCESS){
            err = drv_ops.poll_send_iov(sending_pw);
            if(err != NM_ESUCCESS && err != -NM_EAGAIN){
                nm_ns_print_errno("nm_ns_pong - poll_send_iov",
                                  err);
                goto out;
            }
        }
        nm_ns_update_pw(sending_pw,  data_send, size);


        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);
        ns_lat[i] = sum / 2 /param_nb_samples;

        //DISP("(size = %d) ns_lat[%d] = %lf\n",
        //     size, i, ns_lat[i]);

        nb_samples = 0;
        sum = 0.0;
    }

    nm_ns_free(driver->p_core, sending_pw);
    nm_ns_free(driver->p_core, receiving_pw);
    TBX_FREE(data_send);
    TBX_FREE(data_recv);

    err = NM_ESUCCESS;

 out:

    return err;
}

int
nm_ns_network_sampling(struct nm_drv *driver,
                       gate_id_t gate_id,
                       int connect_flag){
    int err;
    unsigned int n = 1;
    int min = param_min_size;

    DISP("- Network sampling ---------------");
    while(min <= param_max_size){
        n++;
        min *= 2;
    }

    driver->cap.nb_sampling = n;
    driver->cap.network_sampling_latency =
        TBX_MALLOC(n * sizeof(double));

    if(connect_flag){
        err = nm_ns_ping(driver, gate_id);
    } else {
        err = nm_ns_pong(driver, gate_id);
    }

    DISP("----------------------------------");
    DISP("----------------------------------");
    return err;
}


//------------------------------------------
double
nm_ns_evaluate(struct nm_drv *driver,
               int length){
    double * samplings = driver->cap.network_sampling_latency;
    int sampling_id = 0;
    double coef = 0;
    int sampling_start_id = 0;

    frexp(param_min_size, &sampling_start_id);
    coef = frexp(length, &sampling_id);

    sampling_id -= sampling_start_id;

    return samplings[sampling_id]
        + coef * (samplings[sampling_id]
                  - samplings[sampling_id - 1]);

}

