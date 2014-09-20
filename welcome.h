

//
// File:   welcome.h
//

#ifndef _welcome_H
#define	_welcome_H

struct paket_header {
    //    int token;
    int train_id;
    int train_send_countid;
    int paket_id;
    int count_pakets_in_train;
    int recv_data_rate; // Bytes per Sek

    int recv_timeout_wait;

    int last_recv_train_id;
    int last_recv_train_send_countid;
    int last_recv_paket_id;

    struct timespec recv_time;
    struct timespec send_time;
};

#endif	/* _welcome_H */
