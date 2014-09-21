
// save: 21.09.2014 18:45


#include <iostream>

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "welcome.h"

std::list<char*> DateiListe;

int timespec2str(char *buf, uint len, struct timespec *ts) {
    int ret;
    struct tm t;

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL) {
        return 1;
    }

    ret = strftime(buf, len, "%F %T", &t);
    if (ret == 0) {
        return 2;
    }
    len = len - ret;

    ret = snprintf(&buf[ret], len, ".%09ld", ts->tv_nsec);
    if (ret >= len) {
        return 3;
    }

    return 0;
}

bool istInDateiListe(char *datei) {

    for (std::list<char*>::iterator it = DateiListe.begin(); it != DateiListe.end(); it++) {

        //        printf("*it %s\n", *it);

        if (0 == strcmp(*it, datei)) {
            return true;
        }

    }

    return false;
}

// cvs Datei erstellen, mit allen paketen

void create_csv_datei(char* recvDatei, char* sendDatei, char* csvDatei) {

    int File_Deskriptor_send;
    int File_Deskriptor_recv;
    int File_Deskriptor;

    // O_WRONLY nur zum Schreiben �ffnen
    // O_RDWR zum Lesen und Schreiben �ffnen
    // O_RDONLY nur zum Lesen �ffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei �ffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht ge�ffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zur�ckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben ge�ffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    if ((File_Deskriptor_send = open(sendDatei, O_RDONLY)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor_recv = open(recvDatei, O_RDONLY, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    char firstlines[] = "train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n\n\n";
    int buf_size = strlen(firstlines);
    char buf[buf_size];
    int readed = read(File_Deskriptor_send, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    readed = read(File_Deskriptor_recv, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor = open(csvDatei, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", csvDatei, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    FILE *f = fdopen(File_Deskriptor, "w");
    char firstlines2[] = "type;train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n";
    fprintf(f, "%s", firstlines2);

    bool ende = false;

    paket_header ph_recv;
    paket_header ph_send;
    paket_header * php_recv = NULL;
    paket_header * php_send = NULL;

    int paket_header_size = sizeof (paket_header);

    int total_recv = 0;
    int total_send = 0;

    int x = 0;

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;
    char timestr1[timestr_size];
    char timestr2[timestr_size];

    while (ende == false) {

        if (php_recv == NULL) {
            php_recv = &ph_recv;

            readed = read(File_Deskriptor_recv, php_recv, paket_header_size);

            total_recv = total_recv + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, recvDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_recv = NULL;
                }
            } else {
                x = php_recv->train_id;
            }
        }

        if (php_send == NULL) {
            php_send = &ph_send;

            readed = read(File_Deskriptor_send, php_send, paket_header_size);

            total_send = total_send + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, sendDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_send = NULL;
                }
            } else {
                x = php_send->train_id;
            }
        }

        if (php_recv != NULL) {
            if (php_recv->train_id == 12
                    && php_recv->train_send_countid == 0
                    && php_recv->paket_id == 0) {

                php_recv++;
                php_recv--;

            }
        }

        if (php_recv == NULL && php_send == NULL) {
            ende = true;
            continue;
        }

        if (php_recv == NULL || php_send == NULL) {

            if (php_recv != NULL) {

                timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                timespec2str(timestr2, timestr_size, &ph_recv.send_time);

                fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                        ph_recv.train_id,
                        ph_recv.train_send_countid,
                        ph_recv.paket_id,
                        ph_recv.count_pakets_in_train,
                        ph_recv.recv_data_rate,
                        ph_recv.recv_timeout_wait,
                        ph_recv.last_recv_train_id,
                        ph_recv.last_recv_train_send_countid,
                        ph_recv.last_recv_paket_id,
                        timestr1,
                        timestr2
                        );

                fflush(f);

                php_recv = NULL;

            } else {

                timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                timespec2str(timestr2, timestr_size, &ph_send.send_time);

                fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                        ph_send.train_id,
                        ph_send.train_send_countid,
                        ph_send.paket_id,
                        ph_send.count_pakets_in_train,
                        ph_send.recv_data_rate,
                        ph_send.recv_timeout_wait,
                        ph_send.last_recv_train_id,
                        ph_send.last_recv_train_send_countid,
                        ph_send.last_recv_paket_id,
                        timestr1,
                        timestr2
                        );

                fflush(f);

                php_send = NULL;

            }

        } else {
            if (ph_recv.recv_time.tv_sec < ph_send.send_time.tv_sec) {

                timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                timespec2str(timestr2, timestr_size, &ph_recv.send_time);

                fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                        ph_recv.train_id,
                        ph_recv.train_send_countid,
                        ph_recv.paket_id,
                        ph_recv.count_pakets_in_train,
                        ph_recv.recv_data_rate,
                        ph_recv.recv_timeout_wait,
                        ph_recv.last_recv_train_id,
                        ph_recv.last_recv_train_send_countid,
                        ph_recv.last_recv_paket_id,
                        timestr1,
                        timestr2
                        );

                fflush(f);

                php_recv = NULL;

            } else if (ph_send.send_time.tv_sec < ph_recv.recv_time.tv_sec) {

                timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                timespec2str(timestr2, timestr_size, &ph_send.send_time);

                fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                        ph_send.train_id,
                        ph_send.train_send_countid,
                        ph_send.paket_id,
                        ph_send.count_pakets_in_train,
                        ph_send.recv_data_rate,
                        ph_send.recv_timeout_wait,
                        ph_send.last_recv_train_id,
                        ph_send.last_recv_train_send_countid,
                        ph_send.last_recv_paket_id,
                        timestr1,
                        timestr2
                        );

                fflush(f);

                php_send = NULL;

            } else {

                if (ph_recv.recv_time.tv_nsec < ph_send.send_time.tv_nsec) {

                    timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                    timespec2str(timestr2, timestr_size, &ph_recv.send_time);

                    fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                            ph_recv.train_id,
                            ph_recv.train_send_countid,
                            ph_recv.paket_id,
                            ph_recv.count_pakets_in_train,
                            ph_recv.recv_data_rate,
                            ph_recv.recv_timeout_wait,
                            ph_recv.last_recv_train_id,
                            ph_recv.last_recv_train_send_countid,
                            ph_recv.last_recv_paket_id,
                            timestr1,
                            timestr2
                            );

                    fflush(f);

                    php_recv = NULL;

                } else {

                    timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                    timespec2str(timestr2, timestr_size, &ph_send.send_time);

                    fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                            ph_send.train_id,
                            ph_send.train_send_countid,
                            ph_send.paket_id,
                            ph_send.count_pakets_in_train,
                            ph_send.recv_data_rate,
                            ph_send.recv_timeout_wait,
                            ph_send.last_recv_train_id,
                            ph_send.last_recv_train_send_countid,
                            ph_send.last_recv_paket_id,
                            timestr1,
                            timestr2
                            );

                    fflush(f);

                    php_send = NULL;
                }
            }
        }
    }
}

// cvs Datei erstellen (zusammenfassung), nur mit erstem und letztem Paket vom Train

void create_csv_datei_zus(char* recvDatei, char* sendDatei, char* csvDatei) {

    int File_Deskriptor_send;
    int File_Deskriptor_recv;
    int File_Deskriptor;

    // O_WRONLY nur zum Schreiben �ffnen
    // O_RDWR zum Lesen und Schreiben �ffnen
    // O_RDONLY nur zum Lesen �ffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei �ffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht ge�ffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zur�ckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben ge�ffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    if ((File_Deskriptor_send = open(sendDatei, O_RDONLY)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor_recv = open(recvDatei, O_RDONLY, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    }

    char firstlines[] = "train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n\n\n";
    int buf_size = strlen(firstlines);
    char buf[buf_size];
    int readed = read(File_Deskriptor_send, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", sendDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    readed = read(File_Deskriptor_recv, buf, buf_size);
    if (0 != strncmp(firstlines, buf, buf_size)) {
        printf("ERROR:\n  firstlines von der Datei \"%s\" \n(%s)\n ", recvDatei, strerror(errno));
        printf("\n %s \n %s \n", firstlines, buf);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if ((File_Deskriptor = open(csvDatei, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n ", csvDatei, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    FILE *f = fdopen(File_Deskriptor, "w");
    char firstlines2[] = "type;train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n";
    fprintf(f, "%s", firstlines2);

    bool ende = false;

    paket_header ph_recv;
    paket_header ph_send;
    paket_header * php_recv = NULL;
    paket_header * php_send = NULL;

    int paket_header_size = sizeof (paket_header);

    int total_recv = 0;
    int total_send = 0;

    int x = 0;

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;
    char timestr1[timestr_size];
    char timestr2[timestr_size];

    struct paket_header last_ph;
    last_ph.train_id = -1;
    char *last_recv_send;
    char last_recv[] = "recv";
    char last_send[] = "send";

    while (ende == false) {

        if (php_recv == NULL) {
            php_recv = &ph_recv;

            readed = read(File_Deskriptor_recv, php_recv, paket_header_size);

            total_recv = total_recv + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, recvDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_recv = NULL;
                }
            } else {
                x = php_recv->train_id;
            }
        }

        if (php_send == NULL) {
            php_send = &ph_send;

            readed = read(File_Deskriptor_send, php_send, paket_header_size);

            total_send = total_send + readed;

            if (paket_header_size != readed) {
                if (readed != 0) {
                    printf("ERROR:\n  Fehler beim lesen (%d) der Datei \"%s\" \n(%s)\n ", readed, sendDatei, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    php_send = NULL;
                }
            } else {
                x = php_send->train_id;
            }
        }

        if (php_recv == NULL && php_send == NULL) {
            ende = true;
            continue;
        }

        if (php_recv == NULL || php_send == NULL) {

            if (php_recv != NULL) {

                {
                    // recv speichern
                    if (last_ph.train_id == -1) {

                        timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                        timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                        fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                ph_recv.train_id,
                                ph_recv.train_send_countid,
                                ph_recv.paket_id,
                                ph_recv.count_pakets_in_train,
                                ph_recv.recv_data_rate,
                                ph_recv.recv_timeout_wait,
                                ph_recv.last_recv_train_id,
                                ph_recv.last_recv_train_send_countid,
                                ph_recv.last_recv_paket_id,
                                timestr1,
                                timestr2
                                );
                        fflush(f);

                    } else {

                        if (last_ph.train_id == ph_recv.train_id
                                && last_ph.train_send_countid == ph_recv.train_send_countid
                                && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                            timespec2str(timestr2, timestr_size, &last_ph.send_time);
                            fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    last_recv_send,
                                    last_ph.train_id,
                                    last_ph.train_send_countid,
                                    last_ph.paket_id,
                                    last_ph.count_pakets_in_train,
                                    last_ph.recv_data_rate,
                                    last_ph.recv_timeout_wait,
                                    last_ph.last_recv_train_id,
                                    last_ph.last_recv_train_send_countid,
                                    last_ph.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                            fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_recv.train_id,
                                    ph_recv.train_send_countid,
                                    ph_recv.paket_id,
                                    ph_recv.count_pakets_in_train,
                                    ph_recv.recv_data_rate,
                                    ph_recv.recv_timeout_wait,
                                    ph_recv.last_recv_train_id,
                                    ph_recv.last_recv_train_send_countid,
                                    ph_recv.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);
                        }
                    }

                    php_recv = NULL;
                    last_ph = ph_recv;
                    last_recv_send = last_recv;
                    // recv speichern
                }

            } else {

                {
                    // send speichern
                    if (last_ph.train_id == -1) {

                        timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                        timespec2str(timestr2, timestr_size, &ph_send.send_time);
                        fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                ph_send.train_id,
                                ph_send.train_send_countid,
                                ph_send.paket_id,
                                ph_send.count_pakets_in_train,
                                ph_send.recv_data_rate,
                                ph_send.recv_timeout_wait,
                                ph_send.last_recv_train_id,
                                ph_send.last_recv_train_send_countid,
                                ph_send.last_recv_paket_id,
                                timestr1,
                                timestr2
                                );
                        fflush(f);

                    } else {

                        if (last_ph.train_id == ph_send.train_id
                                && last_ph.train_send_countid == ph_send.train_send_countid
                                && last_ph.paket_id == (ph_send.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                            timespec2str(timestr2, timestr_size, &last_ph.send_time);
                            fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    last_recv_send,
                                    last_ph.train_id,
                                    last_ph.train_send_countid,
                                    last_ph.paket_id,
                                    last_ph.count_pakets_in_train,
                                    last_ph.recv_data_rate,
                                    last_ph.recv_timeout_wait,
                                    last_ph.last_recv_train_id,
                                    last_ph.last_recv_train_send_countid,
                                    last_ph.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_send.send_time);
                            fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_send.train_id,
                                    ph_send.train_send_countid,
                                    ph_send.paket_id,
                                    ph_send.count_pakets_in_train,
                                    ph_send.recv_data_rate,
                                    ph_send.recv_timeout_wait,
                                    ph_send.last_recv_train_id,
                                    ph_send.last_recv_train_send_countid,
                                    ph_send.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);
                        }
                    }

                    php_send = NULL;
                    last_ph = ph_send;
                    last_recv_send = last_send;
                    // send speichern
                }

            }

        } else {

            if (ph_recv.recv_time.tv_sec < ph_send.send_time.tv_sec) {

                {
                    // recv speichern
                    if (last_ph.train_id == -1) {

                        timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                        timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                        fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                ph_recv.train_id,
                                ph_recv.train_send_countid,
                                ph_recv.paket_id,
                                ph_recv.count_pakets_in_train,
                                ph_recv.recv_data_rate,
                                ph_recv.recv_timeout_wait,
                                ph_recv.last_recv_train_id,
                                ph_recv.last_recv_train_send_countid,
                                ph_recv.last_recv_paket_id,
                                timestr1,
                                timestr2
                                );
                        fflush(f);

                    } else {

                        if (last_ph.train_id == ph_recv.train_id
                                && last_ph.train_send_countid == ph_recv.train_send_countid
                                && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                            timespec2str(timestr2, timestr_size, &last_ph.send_time);
                            fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    last_recv_send,
                                    last_ph.train_id,
                                    last_ph.train_send_countid,
                                    last_ph.paket_id,
                                    last_ph.count_pakets_in_train,
                                    last_ph.recv_data_rate,
                                    last_ph.recv_timeout_wait,
                                    last_ph.last_recv_train_id,
                                    last_ph.last_recv_train_send_countid,
                                    last_ph.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                            fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_recv.train_id,
                                    ph_recv.train_send_countid,
                                    ph_recv.paket_id,
                                    ph_recv.count_pakets_in_train,
                                    ph_recv.recv_data_rate,
                                    ph_recv.recv_timeout_wait,
                                    ph_recv.last_recv_train_id,
                                    ph_recv.last_recv_train_send_countid,
                                    ph_recv.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);
                        }
                    }

                    php_recv = NULL;
                    last_ph = ph_recv;
                    last_recv_send = last_recv;
                    // recv speichern
                }

            } else if (ph_send.send_time.tv_sec < ph_recv.recv_time.tv_sec) {

                {
                    // send speichern
                    if (last_ph.train_id == -1) {

                        timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                        timespec2str(timestr2, timestr_size, &ph_send.send_time);
                        fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                ph_send.train_id,
                                ph_send.train_send_countid,
                                ph_send.paket_id,
                                ph_send.count_pakets_in_train,
                                ph_send.recv_data_rate,
                                ph_send.recv_timeout_wait,
                                ph_send.last_recv_train_id,
                                ph_send.last_recv_train_send_countid,
                                ph_send.last_recv_paket_id,
                                timestr1,
                                timestr2
                                );
                        fflush(f);

                    } else {

                        if (last_ph.train_id == ph_send.train_id
                                && last_ph.train_send_countid == ph_send.train_send_countid
                                && last_ph.paket_id == (ph_send.paket_id - 1)) {

                        } else {

                            fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                            timespec2str(timestr2, timestr_size, &last_ph.send_time);
                            fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    last_recv_send,
                                    last_ph.train_id,
                                    last_ph.train_send_countid,
                                    last_ph.paket_id,
                                    last_ph.count_pakets_in_train,
                                    last_ph.recv_data_rate,
                                    last_ph.recv_timeout_wait,
                                    last_ph.last_recv_train_id,
                                    last_ph.last_recv_train_send_countid,
                                    last_ph.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                            timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_send.send_time);
                            fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_send.train_id,
                                    ph_send.train_send_countid,
                                    ph_send.paket_id,
                                    ph_send.count_pakets_in_train,
                                    ph_send.recv_data_rate,
                                    ph_send.recv_timeout_wait,
                                    ph_send.last_recv_train_id,
                                    ph_send.last_recv_train_send_countid,
                                    ph_send.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);
                        }
                    }

                    php_send = NULL;
                    last_ph = ph_send;
                    last_recv_send = last_send;
                    // send speichern
                }

            } else {

                if (ph_recv.recv_time.tv_nsec < ph_send.send_time.tv_nsec) {

                    {
                        // recv speichern
                        if (last_ph.train_id == -1) {

                            timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                            fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_recv.train_id,
                                    ph_recv.train_send_countid,
                                    ph_recv.paket_id,
                                    ph_recv.count_pakets_in_train,
                                    ph_recv.recv_data_rate,
                                    ph_recv.recv_timeout_wait,
                                    ph_recv.last_recv_train_id,
                                    ph_recv.last_recv_train_send_countid,
                                    ph_recv.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                        } else {

                            if (last_ph.train_id == ph_recv.train_id
                                    && last_ph.train_send_countid == ph_recv.train_send_countid
                                    && last_ph.paket_id == (ph_recv.paket_id - 1)) {

                            } else {

                                fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                                fflush(f);

                                timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                                timespec2str(timestr2, timestr_size, &last_ph.send_time);
                                fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                        last_recv_send,
                                        last_ph.train_id,
                                        last_ph.train_send_countid,
                                        last_ph.paket_id,
                                        last_ph.count_pakets_in_train,
                                        last_ph.recv_data_rate,
                                        last_ph.recv_timeout_wait,
                                        last_ph.last_recv_train_id,
                                        last_ph.last_recv_train_send_countid,
                                        last_ph.last_recv_paket_id,
                                        timestr1,
                                        timestr2
                                        );
                                fflush(f);

                                timespec2str(timestr1, timestr_size, &ph_recv.recv_time);
                                timespec2str(timestr2, timestr_size, &ph_recv.send_time);
                                fprintf(f, "recv;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                        ph_recv.train_id,
                                        ph_recv.train_send_countid,
                                        ph_recv.paket_id,
                                        ph_recv.count_pakets_in_train,
                                        ph_recv.recv_data_rate,
                                        ph_recv.recv_timeout_wait,
                                        ph_recv.last_recv_train_id,
                                        ph_recv.last_recv_train_send_countid,
                                        ph_recv.last_recv_paket_id,
                                        timestr1,
                                        timestr2
                                        );
                                fflush(f);
                            }
                        }

                        php_recv = NULL;
                        last_ph = ph_recv;
                        last_recv_send = last_recv;
                        // recv speichern
                    }

                } else {

                    {
                        // send speichern
                        if (last_ph.train_id == -1) {

                            timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                            timespec2str(timestr2, timestr_size, &ph_send.send_time);
                            fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                    ph_send.train_id,
                                    ph_send.train_send_countid,
                                    ph_send.paket_id,
                                    ph_send.count_pakets_in_train,
                                    ph_send.recv_data_rate,
                                    ph_send.recv_timeout_wait,
                                    ph_send.last_recv_train_id,
                                    ph_send.last_recv_train_send_countid,
                                    ph_send.last_recv_paket_id,
                                    timestr1,
                                    timestr2
                                    );
                            fflush(f);

                        } else {

                            if (last_ph.train_id == ph_send.train_id
                                    && last_ph.train_send_countid == ph_send.train_send_countid
                                    && last_ph.paket_id == (ph_send.paket_id - 1)) {

                            } else {

                                fprintf(f, "...;...;...;...;...;...;...;...;...;...;...;...\n");
                                fflush(f);

                                timespec2str(timestr1, timestr_size, &last_ph.recv_time);
                                timespec2str(timestr2, timestr_size, &last_ph.send_time);
                                fprintf(f, "%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                        last_recv_send,
                                        last_ph.train_id,
                                        last_ph.train_send_countid,
                                        last_ph.paket_id,
                                        last_ph.count_pakets_in_train,
                                        last_ph.recv_data_rate,
                                        last_ph.recv_timeout_wait,
                                        last_ph.last_recv_train_id,
                                        last_ph.last_recv_train_send_countid,
                                        last_ph.last_recv_paket_id,
                                        timestr1,
                                        timestr2
                                        );
                                fflush(f);

                                timespec2str(timestr1, timestr_size, &ph_send.recv_time);
                                timespec2str(timestr2, timestr_size, &ph_send.send_time);
                                fprintf(f, "send;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s\n",
                                        ph_send.train_id,
                                        ph_send.train_send_countid,
                                        ph_send.paket_id,
                                        ph_send.count_pakets_in_train,
                                        ph_send.recv_data_rate,
                                        ph_send.recv_timeout_wait,
                                        ph_send.last_recv_train_id,
                                        ph_send.last_recv_train_send_countid,
                                        ph_send.last_recv_paket_id,
                                        timestr1,
                                        timestr2
                                        );
                                fflush(f);
                            }
                        }

                        php_send = NULL;
                        last_ph = ph_send;
                        last_recv_send = last_send;
                        // send speichern
                    }

                }
            }
        }
    }
}

int main(int argc, char**argv) {

    DIR *dir;
    struct dirent *dirzeiger;

    /* das Verzeichnis öffnen */
    char pfad[] = "../../log_data/";
    int pfad_len = strlen(pfad);
    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }

    /* das komplette Verzeichnis auslesen */
    while ((dirzeiger = readdir(dir)) != NULL) {
        //        printf("%s\n", (*dirzeiger).d_name);
        char *c = (char*) malloc(strlen((*dirzeiger).d_name));
        strncpy(c, (*dirzeiger).d_name, strlen((*dirzeiger).d_name));
        DateiListe.push_back(c);
    }

    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }
    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }

    /* Dateien parsen */
    while ((dirzeiger = readdir(dir)) != NULL) {
        char *DateiName = (*dirzeiger).d_name;

        int DateiName_len = strlen(DateiName);
        char DateiNameEnde[] = "_recv.b";
        int DateiNameEnde_len = strlen(DateiNameEnde);

        if ((DateiNameEnde_len + 5) < DateiName_len) {
            if (0 == strcmp(DateiName + DateiName_len - DateiNameEnde_len, DateiNameEnde)) {

                char DateiName2[1024];
                strncpy(DateiName2, DateiName, DateiName_len - DateiNameEnde_len);

                char DateiNameEnde2[] = "_send.b";
                int DateiNameEnde2_len = strlen(DateiNameEnde2);

                strncpy(DateiName2 + DateiName_len - DateiNameEnde_len, DateiNameEnde2, DateiNameEnde2_len);

                DateiName2[DateiName_len - DateiNameEnde_len + DateiNameEnde2_len] = 0;

                if (istInDateiListe(DateiName2)) {
                    char DateiName3[1024];
                    strncpy(DateiName3, DateiName, DateiName_len - DateiNameEnde_len);

                    char DateiNameEnde3[] = ".csv";
                    int DateiNameEnde3_len = strlen(DateiNameEnde3);

                    strncpy(DateiName3 + DateiName_len - DateiNameEnde_len, DateiNameEnde3, DateiNameEnde3_len);

                    DateiName3[DateiName_len - DateiNameEnde_len + DateiNameEnde3_len] = 0;

                    if (istInDateiListe(DateiName3) == false) {
                        char d[pfad_len + DateiName_len + DateiNameEnde_len];
                        char d2[pfad_len + DateiName_len + DateiNameEnde2_len];
                        char d3[pfad_len + DateiName_len + DateiNameEnde3_len];

                        d[0] = 0;
                        d2[0] = 0;
                        d3[0] = 0;

                        char *p = d;
                        char *p2 = d2;
                        char *p3 = d3;

                        strncat(d, pfad, pfad_len);
                        strncat(d, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d, DateiNameEnde, DateiNameEnde_len);

                        strncat(d2, pfad, pfad_len);
                        strncat(d2, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d2, DateiNameEnde2, DateiNameEnde2_len);

                        strncat(d3, pfad, pfad_len);
                        strncat(d3, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d3, DateiNameEnde3, DateiNameEnde3_len);

                        create_csv_datei(d, d2, d3);

                        printf("Datei erstellt: %s \n", d3);
                    }
                }

            }
        }

    }

    /* Lesezeiger wieder schließen */
    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }

    if ((dir = opendir(pfad)) == NULL) {
        fprintf(stderr, "Fehler bei opendir ...\n");
        return EXIT_FAILURE;
    }
    /* Dateien parsen zus*/
    while ((dirzeiger = readdir(dir)) != NULL) {
        char *DateiName = (*dirzeiger).d_name;

        int DateiName_len = strlen(DateiName);
        char DateiNameEnde[] = "_recv.b";
        int DateiNameEnde_len = strlen(DateiNameEnde);

        if ((DateiNameEnde_len + 5) < DateiName_len) {
            if (0 == strcmp(DateiName + DateiName_len - DateiNameEnde_len, DateiNameEnde)) {

                char DateiName2[1024];
                strncpy(DateiName2, DateiName, DateiName_len - DateiNameEnde_len);

                char DateiNameEnde2[] = "_send.b";
                int DateiNameEnde2_len = strlen(DateiNameEnde2);

                strncpy(DateiName2 + DateiName_len - DateiNameEnde_len, DateiNameEnde2, DateiNameEnde2_len);

                DateiName2[DateiName_len - DateiNameEnde_len + DateiNameEnde2_len] = 0;

                if (istInDateiListe(DateiName2)) {
                    char DateiName3[1024];
                    strncpy(DateiName3, DateiName, DateiName_len - DateiNameEnde_len);

                    char DateiNameEnde3[] = "_zus.csv";
                    int DateiNameEnde3_len = strlen(DateiNameEnde3);

                    strncpy(DateiName3 + DateiName_len - DateiNameEnde_len, DateiNameEnde3, DateiNameEnde3_len);

                    DateiName3[DateiName_len - DateiNameEnde_len + DateiNameEnde3_len] = 0;

                    if (istInDateiListe(DateiName3) == false) {
                        char d[pfad_len + DateiName_len + DateiNameEnde_len];
                        char d2[pfad_len + DateiName_len + DateiNameEnde2_len];
                        char d3[pfad_len + DateiName_len + DateiNameEnde3_len];

                        d[0] = 0;
                        d2[0] = 0;
                        d3[0] = 0;

                        char *p = d;
                        char *p2 = d2;
                        char *p3 = d3;

                        strncat(d, pfad, pfad_len);
                        strncat(d, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d, DateiNameEnde, DateiNameEnde_len);

                        strncat(d2, pfad, pfad_len);
                        strncat(d2, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d2, DateiNameEnde2, DateiNameEnde2_len);

                        strncat(d3, pfad, pfad_len);
                        strncat(d3, DateiName, DateiName_len - DateiNameEnde_len);
                        strncat(d3, DateiNameEnde3, DateiNameEnde3_len);

                        create_csv_datei_zus(d, d2, d3);

                        printf("Datei erstellt: %s \n", d3);
                    }
                }

            }
        }

    }

    /* Lesezeiger wieder schließen */
    if (closedir(dir) == -1) {
        printf("Fehler beim Schließen von %s\n", argv[1]);
    }

    return EXIT_SUCCESS;


}