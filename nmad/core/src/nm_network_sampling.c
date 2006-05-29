#include <stdint.h>
#include <sys/uio.h>
#include <assert.h>
#include <math.h>
#include <tbx.h>
#include "nm_private.h"

static const int param_nb_samples        = 1000;
static const int param_min_size          = 2;
static const int param_max_size          = 1024*1024*1;


static struct nm_pkt_wrap *
nm_ns_initialize_pw(struct nm_core *p_core,
                    struct nm_drv  *p_drv,
                    uint8_t gate_id){
    struct nm_pkt_wrap * p_pw = NULL;

    nm_pkt_wrap_alloc(p_core, &p_pw, 0, 0);
    nm_iov_alloc(p_core, p_pw, 1);

    unsigned char * data = TBX_MALLOC(param_max_size);
    nm_iov_append_buf(p_core, p_pw, data, param_min_size);


    struct nm_gate *p_gate = p_core->gate_array + gate_id;
    p_pw->p_gate = p_gate;
    p_pw->p_drv  = p_drv;
    p_pw->p_trk  = p_drv->p_track_array[1];

    return p_pw;
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
nm_ns_update_pw(struct nm_pkt_wrap *p_pw, int len){
    p_pw->v[0].iov_len = len;
}

static void
nm_ns_free(struct nm_core *p_core, struct nm_pkt_wrap *p_pw){
    TBX_FREE(p_pw->v[0].iov_base);

    /* free iovec */
    nm_iov_free(p_core, p_pw);

    /* free pkt_wrap */
    nm_pkt_wrap_free(p_core, p_pw);
}


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


static void
nm_ns_ping(struct nm_drv *driver, uint8_t gate_id){
    struct nm_drv_ops drv_ops = driver->ops;
    int nb_samples = 0;
    int size = 0;
    int err = NM_EAGAIN;
    int i = 0;

    double *ns_lat = driver->cap.network_sampling_latency;

    struct nm_pkt_wrap * sending_pw
        = nm_ns_initialize_pw(driver->p_core, driver, gate_id);
    nm_ns_fill_pw_data(sending_pw);
    struct nm_pkt_wrap * receiving_pw
        = nm_ns_initialize_pw(driver->p_core, driver, gate_id);

    double sum = 0.0;
    tbx_tick_t t1, t2;

    for (i = 0, size = param_min_size;
         size <= param_max_size;
         i++, size *= 2) {

        nm_ns_update_pw(sending_pw, size);
        nm_ns_update_pw(receiving_pw, size);

        TBX_GET_TICK(t1);
        while (nb_samples++ < param_nb_samples) {
            err = drv_ops.post_send_iov(sending_pw);
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_send_iov(sending_pw);
            }
            err = NM_EAGAIN;


            err = drv_ops.post_recv_iov(receiving_pw);
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_recv_iov(receiving_pw);
            }
            err = NM_EAGAIN;
            //control_buffer(receiving_pw, size);
        }
        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);

        ns_lat[i] = sum / 2 / param_nb_samples;
        DISP("(size = %d) ns_lat[%d] = %lf\n",
             size, i,  ns_lat[i]);

        nb_samples = 0;
        sum = 0.0;
    }
    nm_ns_free(driver->p_core, sending_pw);
    nm_ns_free(driver->p_core, receiving_pw);
}

static void
nm_ns_pong(struct nm_drv *driver, uint8_t gate_id){
    struct nm_drv_ops drv_ops = driver->ops;
    int nb_samples = 0;
    int size = 0;
    int err = NM_EAGAIN;
    int i = 0;

    double *ns_lat = driver->cap.network_sampling_latency;

    struct nm_pkt_wrap * sending_pw
        = nm_ns_initialize_pw(driver->p_core, driver, gate_id);
    nm_ns_fill_pw_data(sending_pw);
    struct nm_pkt_wrap * receiving_pw
        = nm_ns_initialize_pw(driver->p_core, driver, gate_id);

    double sum = 0.0;
    tbx_tick_t t1, t2;

    for (i = 0, size = param_min_size;
         size <= param_max_size;
         i++, size *= 2) {

        nm_ns_update_pw(sending_pw,   size);
        nm_ns_update_pw(receiving_pw, size);

        TBX_GET_TICK(t1);

        err = drv_ops.post_recv_iov(receiving_pw);
        while(err != NM_ESUCCESS){
            err = drv_ops.poll_recv_iov(receiving_pw);
        }
        err = NM_EAGAIN;
        //control_buffer(receiving_pw, size);

        while (nb_samples++ < param_nb_samples - 1) {
            err = drv_ops.post_send_iov(sending_pw);
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_send_iov(sending_pw);
            }
            err = NM_EAGAIN;

            err = drv_ops.post_recv_iov(receiving_pw);
            while(err != NM_ESUCCESS){
                err = drv_ops.poll_recv_iov(receiving_pw);
            }
            err = NM_EAGAIN;
            //control_buffer(receiving_pw, size);
        }

        err = drv_ops.post_send_iov(sending_pw);
        while(err != NM_ESUCCESS){
            err = drv_ops.poll_send_iov(sending_pw);
        }
        err = NM_EAGAIN;

        TBX_GET_TICK(t2);
        sum = TBX_TIMING_DELAY(t1, t2);
        ns_lat[i] = sum / 2 /param_nb_samples;

        DISP("(size = %d) ns_lat[%d] = %lf\n",
             size, i, ns_lat[i]);

        nb_samples = 0;
        sum = 0.0;
    }

    nm_ns_free(driver->p_core, sending_pw);
    nm_ns_free(driver->p_core, receiving_pw);
}

void
nm_ns_network_sampling(struct nm_drv *driver,
                       uint8_t gate_id,
                       int connect_flag){

    unsigned int n = 1;
    int min = param_min_size;
    while(min <= param_max_size){
        n++;
        min *= 2;
    }

    driver->cap.nb_sampling = n;
    driver->cap.network_sampling_latency =
        TBX_MALLOC(n * sizeof(double));

    if(connect_flag){
        nm_ns_ping(driver, gate_id);
    } else {
        nm_ns_pong(driver, gate_id);
    }
}


//------------------------------------------
double
nm_ns_evaluate(struct nm_drv *driver,
               int length){
    int nb_sampling = driver->cap.nb_sampling;
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

