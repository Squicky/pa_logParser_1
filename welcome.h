

//
// File:   welcome.h
//

#ifndef _welcome_H
#define	_welcome_H

struct paket_header {
    int train_id;
    int retransfer_train_id;
    int paket_id;
    
    int count_pakets_in_train;
    
    int recv_data_rate; // Bytes per Sek

    int last_recv_train_id;
    int last_recv_retransfer_train_id;
    int last_recv_paket_id;
    int last_recv_paket_bytes;
    
    int timeout_time_tv_sec;
    int timeout_time_tv_usec;

    double rtt;

    struct timespec recv_time;
    struct timespec send_time;
};

#endif	/* _welcome_H */
