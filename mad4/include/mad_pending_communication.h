#ifndef MAD_PENDING_COMMUNICATION_H
#define MAD_PENDING_COMMUNICATION_H

#define NB_ENTRIES 32
#define NB_DEST    32

typedef struct s_mad_track_t *p_mad_track_t ;
typedef struct s_mad_track_set_t *p_mad_track_set_t;

typedef struct s_mad_iovec{
    ntbx_process_lrank_t    my_rank;
    ntbx_process_lrank_t    remote_rank;

    p_mad_channel_t         channel; // pour retrouver la
                                     // liste des unpack

    p_mad_connection_t      connection; // pour les unexpected

    sequence_t              sequence;

    size_t                  length;       // ent�tes + donn�es
 
    unsigned int            nb_packs; // nb d'entr�es relatives
                                      // � des donn�es
    unsigned int            total_nb_seg; // donn�es + contr�les

    tbx_bool_t              need_rdv;

    mad_send_mode_t         send_mode;
    mad_receive_mode_t      receive_mode;
    int                     matrix_entrie;

    p_mad_track_t           track;

    struct iovec            data [NB_ENTRIES];

    p_mad_driver_specific_t specific;
} mad_iovec_t, *p_mad_iovec_t;




typedef struct s_mad_port_t{

    p_mad_driver_specific_t specific;

}mad_port_t, *p_mad_port_t;





typedef struct s_mad_track_t{
    uint32_t          id;
    p_mad_track_set_t track_set;

    tbx_bool_t        pre_posted;
    int               nb_dest;

    // ports locaux : [i] -> port � destination de i
    p_mad_port_t     *local_ports;
    // leurs correspondants
    p_mad_port_t    *remote_ports;

    // r�ception en cours
    p_mad_iovec_t    *pending_reception;    //-> � stocker dans chaque port?
    int               nb_pending_reception;

    p_mad_driver_specific_t specific;
}mad_track_t;




typedef struct s_mad_track_set_t{
    tbx_bool_t receiver;

    p_tbx_htable_t tracks_htable;
    p_mad_track_t *tracks_tab;
    uint32_t       nb_track;

    int status; // nothing, progress ou no_progress

    // Pour l'�mission
    p_mad_iovec_t cur;
    p_mad_iovec_t next;

    // Pour la r�ception
    tbx_bool_t *reception_tracks_in_use;
    uint32_t    nb_pending_reception; // nb de pistes
                                      // en attente de r�ception

    // Raccourcis
    p_mad_track_t  cpy_track;
    p_mad_track_t  rdv_track;

    p_mad_driver_specific_t specific;
}mad_track_set_t;

#endif // MAD_PENDING_COMMUNICATION_H
