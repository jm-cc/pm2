  /* The communication channel */ /* Here! */
  p_mad_channel_t channel = NULL;          
  /* The 'local' rank of the process in the current channel *//* Here! */
  ntbx_process_lrank_t my_local_rank = -1;
  /* The set of processes in the current channel *//* Here! */
  p_ntbx_process_container_t pc = NULL;
  /* The globally unique process rank  *//* Here! */
  ntbx_process_grank_t process_rank;

  /* Retrieve the Madeleine object *//* Here! */
  p_mad_madeleine_t madeleine = mad_get_madeleine();

  /* Get a reference to the corresponding channel structure *//* Here! */
  channel = tbx_htable_get(madeleine->channel_htable, CHANNEL_NAME);
  /* If that fails, it means that our process does not *//* Here! */
  /* belong to the channel *//* Here! */
  if (!channel) {
    DISP("I don't belong to this channel");
    return;
  }

  /* Get the set of processes in the channel *//* Here! */
  pc = channel->pc;

  /* Convert the globally unique process rank to its *//* Here! */
  /* _local_ rank in the current channel *//* Here! */
  process_rank  = madeleine->session->process_rank;
  my_local_rank = ntbx_pc_global_to_local(pc, process_rank);
